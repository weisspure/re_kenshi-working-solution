// StatModification_Extension.cpp
//
// KenshiLib plugin: StatModification_Extension
//
// Hooks Dialogue::_doActions to support custom FCS dialogue actions that
// adjust or set character skill stats at runtime.
//
// Supported item types:
//   ADJUST_SKILL_LEVEL (3000) - adjusts a stat by a delta value
//   SET_SKILL_LEVEL    (3001) - sets a stat to a specific value
//   STAT_DEFINITION    (3002) - identifies which stat to target
//   CLAMP_PROFILE      (3003) - reusable clamping policy
//
// Supported action keys (all read from dialogLine->getGameData()->objectReferences):
//
//   Adjust : "add skill levels to {speaker|target|owner}"
//            "remove skill levels from {speaker|target|owner}"
//   Set    : "set skill level for {speaker|target|owner}"
//
// Target semantics:
//   speaker = dlg->getSpeaker(dialogLine->speaker, dialogLine, false)
//   target  = dlg->getConversationTarget().getCharacter()
//   owner   = dlg->me
//
// All actions read their numeric value from GameDataReference::values[0].
// Stat identity is read from a referenced STAT_DEFINITION record (idata["enum value"]).
// Clamping policy is read from an optional CLAMP_PROFILE reference.
// There is no stringID fallback, no preset/default amount, no randomisation.

#include <core/Functions.h>
#include <Debug.h>

#include <kenshi/Dialogue.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/GameData.h>
#include <kenshi/Enums.h>

#include <string>
#include <sstream>


// ============================================================
// SECTION 1: Custom item type IDs
// ============================================================

/**
 * @brief Custom item type IDs used by this plugin's FCS data items.
 *        These must match the item type values defined in your FCS mod.
 */
enum ItemTypeExtended
{
	ADJUST_SKILL_LEVEL = 3000,  ///< FCS item type for stat-adjust actions
	SET_SKILL_LEVEL    = 3001,  ///< FCS item type for stat-set actions
	STAT_DEFINITION    = 3002,  ///< FCS item type for stat definition records
	CLAMP_PROFILE      = 3003   ///< FCS item type for reusable clamp policy records
};

/**
 * @brief Extended dialogue condition type IDs for stat checks.
 *
 * DC_STAT_UNMODIFIED checks the character's raw base stat value,
 * ignoring any equipment or modifier bonuses. Use this for trainer
 * checks ("does this NPC actually know this skill?").
 *
 * DC_STAT_MODIFIED checks the effective stat value including all bonuses.
 *
 * These must match the values in your fcs.def DialogConditionEnum extension.
 */
enum ExtendedDialogConditionEnum
{
	DC_STAT_UNMODIFIED = 3004,  ///< Checks base stat value (ignores equipment bonuses)
	DC_STAT_MODIFIED   = 3005   ///< Checks effective stat value (includes equipment bonuses)
};


// ============================================================
// SECTION 2: String utilities
// ============================================================

/** @brief Converts an int to its decimal string representation. */
static std::string IntToString(int value)
{
	std::ostringstream ss;
	ss << value;
	return ss.str();
}

/** @brief Converts a float to its string representation. */
static std::string FloatToString(float value)
{
	std::ostringstream ss;
	ss << value;
	return ss.str();
}


// ============================================================
// SECTION 3: Safe GameData field readers
//
// These helpers guard against null pointers and missing keys.
// They always return a safe fallback value rather than crashing.
// ============================================================

/**
 * @brief Reads an integer field from GameData::idata.
 *
 * @param data      The GameData to read from. May be null.
 * @param key       The field name to look up.
 * @param fallback  Returned when data is null or key is missing.
 */
static int GetIntField(
	GameData*          data,
	const std::string& key,
	int                fallback)
{
	if (data == 0)
		return fallback;

	// auto asks the compiler to work out the iterator type from the right-hand
	// side so we don't have to write it ourselves. The full type here would be
	// ogre_unordered_map<std::string, int>::type::iterator, which is an
	// engine-specific typedef that is noisy and tells the reader nothing useful.
	// auto hides that noise while keeping identical behaviour.
	auto it = data->idata.find(key);

	if (it != data->idata.end())
		return it->second;

	return fallback;
}

// ============================================================
// SECTION 4: Stat definition resolution
// ============================================================

/**
 * @brief Returns the STAT_DEFINITION GameData referenced by an action record.
 *
 * Looks up objectReferences["stat"] on the action record and returns the
 * GameData of the first referenced STAT_DEFINITION record.
 *
 * Returns null if the action record is null, the reference key is absent,
 * the reference list is empty, or the referenced ptr is null.
 *
 * @param actionRecord  The ADJUST_SKILL_LEVEL or SET_SKILL_LEVEL GameData.
 * @return The STAT_DEFINITION GameData, or null if unresolvable.
 */
static GameData* GetStatDefinitionRecord(GameData* actionRecord)
{
	if (actionRecord == 0)
		return 0;

	auto it = actionRecord->objectReferences.find("stat");
	if (it == actionRecord->objectReferences.end() || it->second.empty())
		return 0;

	GameData* ptr = it->second[0].ptr;
	if (ptr == 0)
		return 0;

	if ((int)ptr->type != STAT_DEFINITION)
	{
		ErrorLog(
			"StatModification: wrong item type for stat reference"
			" | expected=" + IntToString(STAT_DEFINITION) +
			" | got=" + IntToString((int)ptr->type)
		);
		return 0;
	}

	return ptr;
}

/**
 * @brief Reads the StatsEnumerated value from a STAT_DEFINITION record.
 *
 * The int stored in idata["enum value"] is cast directly to StatsEnumerated.
 * No whitelist is applied — the value is used as-is.
 *
 * Extension contract:
 *   This plugin makes no assumptions about which integers are valid beyond
 *   rejecting STAT_NONE (0). If a third-party mod adds custom stats via its
 *   own RE_Kenshi C++ plugin, that mod's author should publish the integer
 *   values of their StatsEnumerated entries. A user then creates a
 *   STAT_DEFINITION record with "enum value" set to that integer, and this
 *   plugin will apply it without requiring any changes here.
 *
 *   Vanilla StatsEnumerated integers are fixed in the Kenshi binary and
 *   will not change.
 *
 * Returns STAT_NONE if the record is null or the field is absent.
 *
 * @param statDef  A STAT_DEFINITION GameData record. May be null.
 * @return The StatsEnumerated value, or STAT_NONE if unresolvable.
 */
static StatsEnumerated ReadStatEnum(GameData* statDef)
{
	if (statDef == 0)
		return STAT_NONE;

	return (StatsEnumerated)GetIntField(statDef, "enum value", (int)STAT_NONE);
}


// ============================================================
// SECTION 5: Target role and resolution
// ============================================================

/**
 * @brief Identifies which dialogue participant to apply a stat action to.
 *
 * ROLE_SPEAKER - the character speaking the current line
 *                (resolved via Dialogue::getSpeaker using dialogLine->speaker,
 *                 which handles T_ME, T_TARGET, T_INTERJECTOR1/2/3, etc.)
 * ROLE_TARGET  - the conversation partner
 *                (resolved via Dialogue::getConversationTarget().getCharacter())
 * ROLE_OWNER   - the dialogue/package owner, resolved via Dialogue::me.
 *                This is the character who holds the dialogue package,
 *                regardless of who is speaking the current line. In a
 *                typical player conversation this is the NPC side.
 */
enum TargetRole
{
	ROLE_SPEAKER,
	ROLE_TARGET,
	ROLE_OWNER
};

/**
 * @brief Resolves a Character* from a Dialogue using a TargetRole.
 *
 * Returns null if the Dialogue is null, the DialogLineData is null (for
 * ROLE_SPEAKER), or if the resolved character pointer is null at runtime
 * (e.g. a conversation target that has left the scene).
 *
 * @param dlg        The active Dialogue. May be null.
 * @param dialogLine The active dialogue line (needed for ROLE_SPEAKER). May be null.
 * @param role       Which participant to resolve.
 * @return The resolved Character*, or null if unavailable.
 */
static Character* ResolveTarget(
	Dialogue*       dlg,
	DialogLineData* dialogLine,
	TargetRole      role)
{
	if (dlg == 0)
		return 0;

	switch (role)
	{
		case ROLE_SPEAKER:
			if (dialogLine == 0)
				return 0;
			return dlg->getSpeaker(dialogLine->speaker, dialogLine, false);

		case ROLE_TARGET:
			return dlg->getConversationTarget().getCharacter();

		case ROLE_OWNER:
			// dlg->me is the dialogue/package owner (typically the NPC side of TALK_TO_ME).
			return dlg->me;
	}

	return 0;
}


// ============================================================
// SECTION 6: Clamp configuration
// ============================================================

/**
 * @brief Holds the clamping policy read from a custom item's GameData.
 *
 * doClamp            - whether any clamping is applied at all
 * minValue/maxValue  - the inclusive clamp range (only used when doClamp is true)
 */
struct ClampConfig
{
	bool  doClamp;
	float minValue;
	float maxValue;
};

/**
 * @brief Reads the ClampConfig from an optional CLAMP_PROFILE reference.
 *
 * Looks up objectReferences["clamp profile"] on the action record.
 * If no reference is set, clamping is disabled entirely — the stat value
 * is written as-is. If a CLAMP_PROFILE record is referenced, its fields
 * drive the policy.
 *
 * Absence of a profile means unclamped. Presence means the author has
 * explicitly opted into a clamping policy.
 *
 * @param actionRecord  The ADJUST_SKILL_LEVEL or SET_SKILL_LEVEL GameData. May be null.
 * @return A ClampConfig describing the clamping policy.
 */
static ClampConfig ReadClampProfile(GameData* actionRecord)
{
	ClampConfig result;
	result.doClamp  = false;  // Default: no profile = no clamping
	result.minValue = 0.0f;
	result.maxValue = 100.0f;

	if (actionRecord == 0)
		return result;

	auto it = actionRecord->objectReferences.find("clamp profile");
	if (it == actionRecord->objectReferences.end() || it->second.empty())
		return result;  // No reference — unclamped

	GameData* profile = it->second[0].ptr;
	if (profile == 0)
		return result;

	result.doClamp  = true;
	result.minValue = (float)GetIntField(profile, "clamp min", 0);
	result.maxValue = (float)GetIntField(profile, "clamp max", 100);
	return result;
}

/**
 * @brief Clamps a float value according to a ClampConfig.
 *
 * @param value  The value to clamp.
 * @param clamp  The policy to apply.
 * @return The clamped value, or the original value if clamping is disabled.
 */
static float ApplyClamp(float value, const ClampConfig& clamp)
{
	if (!clamp.doClamp)
		return value;

	if (value < clamp.minValue)
		return clamp.minValue;

	if (value > clamp.maxValue)
		return clamp.maxValue;

	return value;
}


// ============================================================
// SECTION 7: Stat mutation helpers
// ============================================================

/**
 * @brief Adds a signed integer delta to a character's stat, then clamps.
 *
 * The stat is mutated directly via CharStats::getStatRef. This does not
 * grant XP - it sets the stored base value immediately.
 *
 * @param character  The character to modify. May be null (no-op).
 * @param stat       The stat to adjust.
 * @param delta      The signed amount to add. Negative values subtract.
 * @param clamp      The clamping policy to apply after adjustment.
 */
static void AdjustStatForCharacter(
	Character*         character,
	StatsEnumerated    stat,
	int                delta,
	const ClampConfig& clamp)
{
	if (character == 0)
		return;

	CharStats* stats = character->getStats();
	if (stats == 0)
	{
		ErrorLog("StatModification: character->getStats() returned null during adjust");
		return;
	}

	float& statRef = stats->getStatRef(stat);
	float  before  = statRef;
	float  after   = ApplyClamp(before + (float)delta, clamp);
	statRef = after;

	DebugLog(
		"StatModification: adjusted " + CharStats::getStatName(stat) +
		" | delta=" + IntToString(delta) +
		" | before=" + FloatToString(before) +
		" | after=" + FloatToString(after)
	);
}

/**
 * @brief Sets a character's stat to a specific value, then clamps.
 *
 * The stat is mutated directly via CharStats::getStatRef. This does not
 * grant XP - it sets the stored base value immediately.
 *
 * @param character  The character to modify. May be null (no-op).
 * @param stat       The stat to set.
 * @param value      The new value to assign.
 * @param clamp      The clamping policy to apply after assignment.
 */
static void SetStatForCharacter(
	Character*         character,
	StatsEnumerated    stat,
	int                value,
	const ClampConfig& clamp)
{
	if (character == 0)
		return;

	CharStats* stats = character->getStats();
	if (stats == 0)
	{
		ErrorLog("StatModification: character->getStats() returned null during set");
		return;
	}

	float& statRef = stats->getStatRef(stat);
	float  before  = statRef;
	float  after   = ApplyClamp((float)value, clamp);
	statRef = after;

	DebugLog(
		"StatModification: set " + CharStats::getStatName(stat) +
		" | to=" + IntToString(value) +
		" | before=" + FloatToString(before) +
		" | after=" + FloatToString(after)
	);
}


// ============================================================
// SECTION 8: Per-reference action application
// ============================================================

/**
 * @brief Applies one adjust GameDataReference to a character.
 *
 * Validates that ref.ptr is non-null and has type ADJUST_SKILL_LEVEL (3000).
 * Navigates to the referenced STAT_DEFINITION record to read the stat enum value.
 * Reads the delta from ref.values[0], applies the remove sign if needed,
 * reads the optional CLAMP_PROFILE reference, and calls AdjustStatForCharacter.
 *
 * @param character  The character to modify. May be null (no-op).
 * @param ref        The GameDataReference for this item entry.
 * @param isRemove   If true, the adjustment is forced to be non-positive so the stat is reduced.
 */
static void ApplyAdjustRef(
	Character*               character,
	const GameDataReference& ref,
	bool                     isRemove)
{
	if (ref.ptr == 0)
	{
		ErrorLog("StatModification: ref.ptr is null in adjust action, skipping entry");
		return;
	}

	if ((int)ref.ptr->type != ADJUST_SKILL_LEVEL)
	{
		ErrorLog(
			"StatModification: wrong item type in adjust action"
			" | expected=" + IntToString(ADJUST_SKILL_LEVEL) +
			" | got=" + IntToString((int)ref.ptr->type)
		);
		return;
	}

	GameData* statDef = GetStatDefinitionRecord(ref.ptr);
	if (statDef == 0)
	{
		ErrorLog("StatModification: no STAT_DEFINITION reference set in adjust action");
		return;
	}

	StatsEnumerated stat = ReadStatEnum(statDef);
	if (stat == STAT_NONE)
	{
		ErrorLog("StatModification: STAT_DEFINITION enum value is STAT_NONE in adjust action");
		return;
	}

	int delta = ref.values[0];

	// Remove actions always subtract: ensure delta is non-positive.
	// If the author stored a positive value, negate it.
	// If it is already negative or zero, leave it as-is.
	if (isRemove && delta > 0)
		delta = -delta;

	ClampConfig clamp = ReadClampProfile(ref.ptr);
	AdjustStatForCharacter(character, stat, delta, clamp);
}

/**
 * @brief Applies one set GameDataReference to a character.
 *
 * Validates that ref.ptr is non-null and has type SET_SKILL_LEVEL (3001).
 * Navigates to the referenced STAT_DEFINITION record to read the stat enum value.
 * Reads the target value from ref.values[0], reads the optional CLAMP_PROFILE
 * reference, and calls SetStatForCharacter.
 *
 * @param character  The character to modify. May be null (no-op).
 * @param ref        The GameDataReference for this item entry.
 */
static void ApplySetRef(
	Character*               character,
	const GameDataReference& ref)
{
	if (ref.ptr == 0)
	{
		ErrorLog("StatModification: ref.ptr is null in set action, skipping entry");
		return;
	}

	if ((int)ref.ptr->type != SET_SKILL_LEVEL)
	{
		ErrorLog(
			"StatModification: wrong item type in set action"
			" | expected=" + IntToString(SET_SKILL_LEVEL) +
			" | got=" + IntToString((int)ref.ptr->type)
		);
		return;
	}

	GameData* statDef = GetStatDefinitionRecord(ref.ptr);
	if (statDef == 0)
	{
		ErrorLog("StatModification: no STAT_DEFINITION reference set in set action");
		return;
	}

	StatsEnumerated stat = ReadStatEnum(statDef);
	if (stat == STAT_NONE)
	{
		ErrorLog("StatModification: STAT_DEFINITION enum value is STAT_NONE in set action");
		return;
	}

	int value = ref.values[0];
	ClampConfig clamp = ReadClampProfile(ref.ptr);
	SetStatForCharacter(character, stat, value, clamp);
}


// ============================================================
// SECTION 9: Action dispatchers
// ============================================================

/**
 * @brief Looks up one adjust action key and applies it if present.
 *
 * This is intentionally a no-op when the key is absent, so the dispatcher
 * can call it unconditionally for every known key without prior existence checks.
 *
 * @param dlg        The active Dialogue. May be null.
 * @param dialogLine The active dialogue line. May be null.
 * @param lineData   GameData for the dialogue line. Must not be null.
 * @param actionKey  The objectReferences key to look up.
 * @param role       Which character receives the adjustment.
 * @param isRemove   If true, the adjustment is forced to be non-positive so the stat is reduced.
 */
static void TryApplyAdjustAction(
	Dialogue*          dlg,
	DialogLineData*    dialogLine,
	GameData*          lineData,
	const std::string& actionKey,
	TargetRole         role,
	bool               isRemove)
{
	auto iter = lineData->objectReferences.find(actionKey);

	if (iter == lineData->objectReferences.end())
		return;

	Character* character = ResolveTarget(dlg, dialogLine, role);
	if (character == 0)
	{
		ErrorLog("StatModification: could not resolve character for '" + actionKey + "'");
		return;
	}

	Ogre::vector<GameDataReference>::type& refs = iter->second;
	for (auto it = refs.begin(); it != refs.end(); ++it)
	{
		ApplyAdjustRef(character, *it, isRemove);
	}
}

/**
 * @brief Looks up one set action key and applies it if present.
 *
 * This is intentionally a no-op when the key is absent.
 *
 * @param dlg        The active Dialogue. May be null.
 * @param dialogLine The active dialogue line. May be null.
 * @param lineData   GameData for the dialogue line. Must not be null.
 * @param actionKey  The objectReferences key to look up.
 * @param role       Which character receives the set.
 */
static void TryApplySetAction(
	Dialogue*          dlg,
	DialogLineData*    dialogLine,
	GameData*          lineData,
	const std::string& actionKey,
	TargetRole         role)
{
	auto iter = lineData->objectReferences.find(actionKey);

	if (iter == lineData->objectReferences.end())
		return;

	Character* character = ResolveTarget(dlg, dialogLine, role);
	if (character == 0)
	{
		ErrorLog("StatModification: could not resolve character for '" + actionKey + "'");
		return;
	}

	Ogre::vector<GameDataReference>::type& refs = iter->second;
	for (auto it = refs.begin(); it != refs.end(); ++it)
	{
		ApplySetRef(character, *it);
	}
}

/**
 * @brief Dispatches all supported stat actions found on a dialogue line.
 *
 * Each of the 9 known action keys is checked independently against
 * objectReferences. Keys not present are silently skipped. Multiple
 * keys on the same line are all applied in the order listed below.
 *
 * Ordering is intentional: adjust actions always run before set actions.
 * If the same stat on the same character is targeted by both an adjust and
 * a set action on the same line, the set action wins because it runs last
 * and overwrites the adjusted value.
 *
 * Within each group, speaker actions come before target, which come before owner.
 *
 * @param dlg        The active Dialogue. May be null (no-op).
 * @param dialogLine The active dialogue line. May be null (no-op).
 */
static void DispatchStatActions(Dialogue* dlg, DialogLineData* dialogLine)
{
	if (dlg == 0 || dialogLine == 0)
		return;

	GameData* lineData = dialogLine->getGameData();
	if (lineData == 0)
		return;

	// Adjust actions run first (delta = values[0]).
	TryApplyAdjustAction(dlg, dialogLine, lineData, "add skill levels to speaker",      ROLE_SPEAKER, false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, "add skill levels to target",       ROLE_TARGET,  false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, "add skill levels to owner",        ROLE_OWNER,   false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, "remove skill levels from speaker", ROLE_SPEAKER, true);
	TryApplyAdjustAction(dlg, dialogLine, lineData, "remove skill levels from target",  ROLE_TARGET,  true);
	TryApplyAdjustAction(dlg, dialogLine, lineData, "remove skill levels from owner",   ROLE_OWNER,   true);

	// Set actions run second (value = values[0]).
	// If a stat was already adjusted above, the set value wins.
	TryApplySetAction(dlg, dialogLine, lineData, "set skill level for speaker", ROLE_SPEAKER);
	TryApplySetAction(dlg, dialogLine, lineData, "set skill level for target",  ROLE_TARGET);
	TryApplySetAction(dlg, dialogLine, lineData, "set skill level for owner",   ROLE_OWNER);
}


// ============================================================
// SECTION 10: Stat condition evaluation
// ============================================================

/**
 * @brief Compares two integers using a ComparisonEnum operator.
 *
 * @param value      The character's stat value (left-hand side).
 * @param threshold  The value to compare against (right-hand side).
 * @param compareBy  The comparison operator.
 * @return True if the comparison holds.
 */
static bool StatDialogCompare(int value, int threshold, ComparisonEnum compareBy)
{
	if (compareBy == CE_EQUALS   && value == threshold) return true;
	if (compareBy == CE_LESS_THAN && value <  threshold) return true;
	if (compareBy == CE_MORE_THAN && value >  threshold) return true;
	return false;
}

/**
 * @brief Evaluates a single stat condition against a character.
 *
 * @param conditionType  DC_STAT_UNMODIFIED or DC_STAT_MODIFIED.
 * @param character      The character to check. May be null (returns false).
 * @param compareBy      The comparison operator.
 * @param stat           The stat to read.
 * @param threshold      The value to compare against.
 * @return True if the condition holds.
 */
static bool EvaluateStatCondition(
	ExtendedDialogConditionEnum conditionType,
	Character*                  character,
	ComparisonEnum              compareBy,
	StatsEnumerated             stat,
	int                         threshold)
{
	if (character == 0)
		return false;

	CharStats* stats = character->getStats();
	if (stats == 0)
		return false;

	bool unmodified = (conditionType == DC_STAT_UNMODIFIED);
	float statValue = stats->getStat(stat, unmodified);
	return StatDialogCompare((int)statValue, threshold, compareBy);
}

/** @brief Saved pointer to the original DialogLineData::checkTags. */
static bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target) = 0;

/**
 * @brief Hook for DialogLineData::checkTags.
 *
 * Evaluates DC_STAT_UNMODIFIED and DC_STAT_MODIFIED conditions.
 * Only enters the condition scan if at least one condition key on this
 * dialogue line matches a supported extended type, then processes all
 * custom conditions from objectReferences["conditions"].
 * Conditions from other plugins are ignored (returns true for them).
 * The original is always called last so built-in conditions still evaluate.
 *
 * Note: T_WHOLE_SQUAD is not supported. T_ME checks the dialogue owner;
 * all other TalkerEnum values check the conversation target.
 *
 * @param thisptr  The dialogue line being evaluated.
 * @param me       The dialogue owner (typically the NPC).
 * @param target   The conversation target (typically the player).
 */
static bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target)
{
	bool hasExtendedCondition = false;
	for (int i = 0; i < (int)thisptr->conditions.size(); ++i)
	{
		int key = thisptr->conditions[i]->key;
		if (key == DC_STAT_UNMODIFIED || key == DC_STAT_MODIFIED)
		{
			hasExtendedCondition = true;
			break;
		}
	}

	if (hasExtendedCondition)
	{
		GameData* lineData = thisptr->getGameData();
		if (lineData != 0)
		{
			auto iter = lineData->objectReferences.find("conditions");
			if (iter != lineData->objectReferences.end())
			{
				for (int i = 0; i < (int)iter->second.size(); ++i)
				{
					GameData* cond = iter->second[i].ptr;
					if (cond == 0)
						continue;

					auto condNameIt  = cond->idata.find("condition name");
					auto compareByIt = cond->idata.find("compare by");
					auto whoIt       = cond->idata.find("who");
					auto tagIt       = cond->idata.find("tag");

					if (condNameIt  == cond->idata.end() ||
						compareByIt == cond->idata.end() ||
						whoIt       == cond->idata.end() ||
						tagIt       == cond->idata.end())
					{
						continue;  // Missing fields — not our condition, skip
					}

					int conditionInt = condNameIt->second;
					if (conditionInt != DC_STAT_UNMODIFIED && conditionInt != DC_STAT_MODIFIED)
						continue;  // Different plugin's condition, skip

					ExtendedDialogConditionEnum conditionType =
						(ExtendedDialogConditionEnum)conditionInt;
					ComparisonEnum  compareBy = (ComparisonEnum)compareByIt->second;
					TalkerEnum      who       = (TalkerEnum)whoIt->second;
					StatsEnumerated stat      = (StatsEnumerated)tagIt->second;
					int             threshold = iter->second[i].values[0];

					Character* conditionCheck = (who == T_ME) ? me : target;

					if (!EvaluateStatCondition(conditionType, conditionCheck, compareBy, stat, threshold))
						return false;
				}
			}
		}
	}

	if (checkTags_orig == 0)
	{
		ErrorLog("StatModification: checkTags_orig is null, cannot call original");
		return true;
	}

	return checkTags_orig(thisptr, me, target);
}


// ============================================================
// SECTION 11: Hook and plugin entry
// ============================================================

/** @brief Saved pointer to the original Dialogue::_doActions, set during hook installation. */
static void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine) = 0;

/**
 * @brief Hook for Dialogue::_doActions.
 *
 * Called by the engine once per dialogue line execution, before the engine's
 * own action handler runs. Custom stat actions are dispatched here first,
 * then the original function is called so built-in actions still work.
 *
 * If _doActions_orig is null (hook installation failed silently), an error
 * is logged and the call is skipped rather than crashing.
 *
 * @param thisptr    The Dialogue executing the line. May be null in edge cases.
 * @param dialogLine The line whose actions are being executed. May be null.
 */
static void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
	if (thisptr != 0 && dialogLine != 0)
		DispatchStatActions(thisptr, dialogLine);

	if (_doActions_orig == 0)
	{
		ErrorLog("StatModification: _doActions_orig is null, cannot call original");
		return;
	}

	_doActions_orig(thisptr, dialogLine);
}

/**
 * @brief DLL entry point called by KenshiLib when the plugin is loaded.
 *
 * Installs both hooks. Logs an error and returns early on any failure
 * so the plugin fails visibly rather than silently.
 */
__declspec(dllexport) void startPlugin()
{
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(
			KenshiLib::GetRealAddress(&Dialogue::_doActions),
			&_doActions_hook,
			&_doActions_orig))
	{
		ErrorLog("StatModification_Extension: failed to hook Dialogue::_doActions");
		return;
	}

	if (KenshiLib::SUCCESS != KenshiLib::AddHook(
			KenshiLib::GetRealAddress(&DialogLineData::checkTags),
			&checkTags_hook,
			&checkTags_orig))
	{
		ErrorLog("StatModification_Extension: failed to hook DialogLineData::checkTags");
		return;
	}

	DebugLog("StatModification_Extension: loaded and hooks installed");
}
