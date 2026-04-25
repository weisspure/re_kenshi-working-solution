// StatModification_Extension.cpp
//
// KenshiLib plugin: StatModification_Extension
//
// Hooks Dialogue::_doActions to support custom FCS dialogue actions that
// adjust or set character skill stats at runtime.
//
// Supported item types:
//   STAT_DEFINITION    (3000) - identifies which stat to target
//   CLAMP_PROFILE      (3001) - reusable clamping policy
//   ADJUST_SKILL_LEVEL (3002) - adjusts a stat by a delta value
//   SET_SKILL_LEVEL    (3003) - sets a stat to a specific value
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

// Set true only for short in-game identity spikes. This is intentionally noisy.
static const bool LOG_DIALOGUE_IDENTITY_SPIKE = false;

// ============================================================
// SECTION 1: Custom item type IDs
// ============================================================

/**
 * @brief Custom item type IDs used by this plugin's FCS data items.
 *        These must match the item type values defined in your FCS mod.
 */
enum ItemTypeExtended
{
	STAT_DEFINITION = 3000,	   ///< FCS item type for stat definition records
	CLAMP_PROFILE = 3001,	   ///< FCS item type for reusable clamp policy records
	ADJUST_SKILL_LEVEL = 3002, ///< FCS item type for stat-adjust actions
	SET_SKILL_LEVEL = 3003	   ///< FCS item type for stat-set actions
};

/**
 * @brief Custom dialogue condition IDs used by this plugin.
 */
enum ExtendedDialogConditionEnum
{
	DC_STAT_LEVEL_COMPARE_UNMODIFIED = 3004, ///< Compares two characters' base stat values
	DC_STAT_LEVEL_COMPARE_MODIFIED = 3005  ///< Compares two characters' effective stat values
};

// FCS objectReferences keys for dialogue actions. These must match fcs.def exactly.
static const char* ACTION_ADD_SKILL_LEVELS_TO_SPEAKER = "add skill levels to speaker";
static const char* ACTION_ADD_SKILL_LEVELS_TO_TARGET = "add skill levels to target";
static const char* ACTION_ADD_SKILL_LEVELS_TO_OWNER = "add skill levels to owner";
static const char* ACTION_REMOVE_SKILL_LEVELS_FROM_SPEAKER = "remove skill levels from speaker";
static const char* ACTION_REMOVE_SKILL_LEVELS_FROM_TARGET = "remove skill levels from target";
static const char* ACTION_REMOVE_SKILL_LEVELS_FROM_OWNER = "remove skill levels from owner";
static const char* ACTION_SET_SKILL_LEVEL_FOR_SPEAKER = "set skill level for speaker";
static const char* ACTION_SET_SKILL_LEVEL_FOR_TARGET = "set skill level for target";
static const char* ACTION_SET_SKILL_LEVEL_FOR_OWNER = "set skill level for owner";

// FCS field and objectReferences keys. These must match fcs.def / FCS condition data.
static const char* REF_STAT = "stat";
static const char* REF_CLAMP_PROFILE = "clamp profile";
static const char* REF_CONDITIONS = "conditions";
static const char* FIELD_ENUM_VALUE = "enum value";
static const char* FIELD_CLAMP_MIN = "clamp min";
static const char* FIELD_CLAMP_MAX = "clamp max";
static const char* FIELD_CONDITION_NAME = "condition name";
static const char* FIELD_COMPARE_BY = "compare by";
static const char* FIELD_WHO = "who";
static const char* FIELD_TAG = "tag";

static const char* STAT_ACTION_KEYS[] = {
	ACTION_ADD_SKILL_LEVELS_TO_SPEAKER,
	ACTION_ADD_SKILL_LEVELS_TO_TARGET,
	ACTION_ADD_SKILL_LEVELS_TO_OWNER,
	ACTION_REMOVE_SKILL_LEVELS_FROM_SPEAKER,
	ACTION_REMOVE_SKILL_LEVELS_FROM_TARGET,
	ACTION_REMOVE_SKILL_LEVELS_FROM_OWNER,
	ACTION_SET_SKILL_LEVEL_FOR_SPEAKER,
	ACTION_SET_SKILL_LEVEL_FOR_TARGET,
	ACTION_SET_SKILL_LEVEL_FOR_OWNER
};

static const int STAT_ACTION_KEY_COUNT = 9;

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

/** @brief Converts a pointer value to a compact string for diagnostics. */
static std::string PointerToString(const void* ptr)
{
	std::ostringstream ss;
	ss << ptr;
	return ss.str();
}

/** @brief Writes a namespaced info message to the RE_KENSHI debug log. */
static void LogInfo(const std::string& message)
{
	DebugLog("StatModification: " + message);
}

/** @brief Writes a namespaced warning message to the RE_KENSHI debug log. */
static void LogWarning(const std::string& message)
{
	DebugLog("StatModification WARNING: " + message);
}

/** @brief Writes a namespaced error message to the RE_KENSHI error log. */
static void LogError(const std::string& message)
{
	ErrorLog("StatModification: " + message);
}

/** @brief Describes a GameData record without relying on string IDs for behavior. */
static std::string DescribeGameData(GameData* data)
{
	if (data == 0)
		return "null";

	return "name=\"" + data->name +
		"\" | stringID=\"" + data->stringID +
		"\" | id=" + IntToString(data->id) +
		" | type=" + IntToString((int)data->type);
}

/** @brief Describes a character pointer for temporary identity diagnostics. */
static std::string DescribeCharacter(Character* character)
{
	if (character == 0)
		return "null";

	return "ptr=" + PointerToString(character) +
		" | name=\"" + character->getName() +
		"\" | data={" + DescribeGameData(character->getGameData()) + "}";
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
	GameData* data,
	const std::string& key,
	int fallback)
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

/**
 * @brief Returns a non-empty GameData reference list by FCS key.
 *
 * Missing keys and empty lists both return null. Callers decide whether that
 * is a no-op, an optional unset field, or an error.
 */
static Ogre::vector<GameDataReference>::type* FindReferences(
	GameData* data,
	const std::string& key)
{
	if (data == 0)
		return 0;

	auto it = data->objectReferences.find(key);
	if (it == data->objectReferences.end() || it->second.empty())
		return 0;

	return &it->second;
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
static GameData* GetStatDefinitionRecord(
	GameData* actionRecord,
	const std::string& context)
{
	if (actionRecord == 0)
		return 0;

	Ogre::vector<GameDataReference>::type* refs = FindReferences(actionRecord, REF_STAT);
	if (refs == 0)
		return 0;

	GameData* ptr = (*refs)[0].ptr;
	if (ptr == 0)
		return 0;

	if ((int)ptr->type != STAT_DEFINITION)
	{
		LogError(
			"wrong item type for stat reference"
			" | context=" +
			context +
			" | expected=" +
			IntToString(STAT_DEFINITION) +
			" | got=" + IntToString((int)ptr->type));
		return 0;
	}

	return ptr;
}

/**
 * @brief Reads the StatsEnumerated value from a STAT_DEFINITION record.
 *
 * The int stored in idata["enum value"] is cast directly to StatsEnumerated.
 * No whitelist is applied - the value is used as-is.
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

	return (StatsEnumerated)GetIntField(statDef, FIELD_ENUM_VALUE, (int)STAT_NONE);
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
 *                typical "Talk to me" conversation this is the NPC side.
 */
enum TargetRole
{
	ROLE_SPEAKER,
	ROLE_TARGET,
	ROLE_OWNER
};

/** @brief Converts a TargetRole to the FCS-facing role name. */
static std::string TargetRoleToString(TargetRole role)
{
	switch (role)
	{
	case ROLE_SPEAKER:
		return "speaker";

	case ROLE_TARGET:
		return "target";

	case ROLE_OWNER:
		return "owner";
	}

	return "unknown";
}

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
	Dialogue* dlg,
	DialogLineData* dialogLine,
	TargetRole role)
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

/** @brief Logs current dialogue identities for short in-game test spikes. */
static void LogDialogueIdentitySpike(
	const std::string& phase,
	Dialogue* dlg,
	DialogLineData* dialogLine)
{
	if (!LOG_DIALOGUE_IDENTITY_SPIKE)
		return;

	if (dlg == 0 || dialogLine == 0)
	{
		LogInfo("identity spike | phase=" + phase + " | dlg or dialogLine is null");
		return;
	}

	GameData* lineData = dialogLine->getGameData();
	Character* speaker = ResolveTarget(dlg, dialogLine, ROLE_SPEAKER);
	Character* target = ResolveTarget(dlg, dialogLine, ROLE_TARGET);
	Character* owner = ResolveTarget(dlg, dialogLine, ROLE_OWNER);

	LogInfo(
		"identity spike"
		" | phase=" +
		phase +
		" | line={" + DescribeGameData(lineData) +
		"} | dialogLine->speaker=" + IntToString((int)dialogLine->speaker) +
		" | speaker={" + DescribeCharacter(speaker) +
		"} | target={" + DescribeCharacter(target) +
		"} | owner={" + DescribeCharacter(owner) + "}");
}

/** @brief Logs condition-side identities for short in-game test spikes. */
static void LogConditionIdentitySpike(
	const std::string& phase,
	DialogLineData* dialogLine,
	Character* me,
	Character* target)
{
	if (!LOG_DIALOGUE_IDENTITY_SPIKE)
		return;

	GameData* lineData = (dialogLine == 0) ? 0 : dialogLine->getGameData();
	LogInfo(
		"identity spike"
		" | phase=" +
		phase +
		" | line={" + DescribeGameData(lineData) +
		"} | me={" + DescribeCharacter(me) +
		"} | target={" + DescribeCharacter(target) + "}");
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
	bool doClamp;
	float minValue;
	float maxValue;
};

/**
 * @brief Reads the ClampConfig from an optional CLAMP_PROFILE reference.
 *
 * Looks up objectReferences["clamp profile"] on the action record.
 * If no reference is set, clamping is disabled entirely - the stat value
 * is written as-is. If a CLAMP_PROFILE record is referenced, its fields
 * drive the policy.
 *
 * Absence of a profile means unclamped. Presence means the author has
 * explicitly opted into a clamping policy.
 *
 * @param actionRecord  The ADJUST_SKILL_LEVEL or SET_SKILL_LEVEL GameData. May be null.
 * @return A ClampConfig describing the clamping policy.
 */
static ClampConfig ReadClampProfile(
	GameData* actionRecord,
	const std::string& context)
{
	ClampConfig result;
	result.doClamp = false; // Default: no profile = no clamping
	result.minValue = 0.0f;
	result.maxValue = 0.0f; // Unused unless doClamp is true

	if (actionRecord == 0)
		return result;

	Ogre::vector<GameDataReference>::type* refs = FindReferences(actionRecord, REF_CLAMP_PROFILE);
	if (refs == 0)
		return result; // No reference - unclamped

	GameData* profile = (*refs)[0].ptr;
	if (profile == 0)
	{
		LogError("clamp profile reference is null | context=" + context + " | continuing unclamped");
		return result;
	}

	if ((int)profile->type != CLAMP_PROFILE)
	{
		LogError(
			"wrong item type for clamp profile"
			" | context=" +
			context +
			" | expected=" +
			IntToString(CLAMP_PROFILE) +
			" | got=" + IntToString((int)profile->type) +
			" | continuing unclamped");
		return result;
	}

	auto minIt = profile->idata.find(FIELD_CLAMP_MIN);
	auto maxIt = profile->idata.find(FIELD_CLAMP_MAX);
	if (minIt == profile->idata.end() || maxIt == profile->idata.end())
	{
		LogError("clamp profile is missing clamp min or clamp max | context=" + context + " | continuing unclamped");
		return result;
	}

	float minValue = (float)minIt->second;
	float maxValue = (float)maxIt->second;
	if (minValue > maxValue)
	{
		LogError(
			"clamp profile min is greater than max"
			" | context=" +
			context +
			" | min=" +
			FloatToString(minValue) +
			" | max=" + FloatToString(maxValue) +
			" | continuing unclamped");
		return result;
	}

	result.doClamp = true;
	result.minValue = minValue;
	result.maxValue = maxValue;
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
	Character* character,
	StatsEnumerated stat,
	int delta,
	const ClampConfig& clamp)
{
	if (character == 0)
		return;

	CharStats* stats = character->getStats();
	if (stats == 0)
	{
		LogError("character->getStats() returned null during adjust");
		return;
	}

	float& statRef = stats->getStatRef(stat);
	float before = statRef;
	float after = ApplyClamp(before + (float)delta, clamp);
	statRef = after;

	LogInfo(
		"adjusted " + CharStats::getStatName(stat) +
		" | delta=" + IntToString(delta) +
		" | before=" + FloatToString(before) +
		" | after=" + FloatToString(after));
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
	Character* character,
	StatsEnumerated stat,
	int value,
	const ClampConfig& clamp)
{
	if (character == 0)
		return;

	CharStats* stats = character->getStats();
	if (stats == 0)
	{
		LogError("character->getStats() returned null during set");
		return;
	}

	float& statRef = stats->getStatRef(stat);
	float before = statRef;
	float after = ApplyClamp((float)value, clamp);
	statRef = after;

	LogInfo(
		"set " + CharStats::getStatName(stat) +
		" | to=" + IntToString(value) +
		" | before=" + FloatToString(before) +
		" | after=" + FloatToString(after));
}

// ============================================================
// SECTION 8: Per-reference action application
// ============================================================

/**
 * @brief Applies one adjust GameDataReference to a character.
 *
 * Validates that ref.ptr is non-null and has type ADJUST_SKILL_LEVEL (3002).
 * Navigates to the referenced STAT_DEFINITION record to read the stat enum value.
 * Reads the delta from ref.values[0], applies the remove sign if needed,
 * reads the optional CLAMP_PROFILE reference, and calls AdjustStatForCharacter.
 *
 * @param character  The character to modify. May be null (no-op).
 * @param ref        The GameDataReference for this item entry.
 * @param isRemove   If true, the adjustment is forced to be non-positive so the stat is reduced.
 */
static void ApplyAdjustRef(
	Character* character,
	const GameDataReference& ref,
	const std::string& actionKey,
	bool isRemove)
{
	if (ref.ptr == 0)
	{
		LogError("ref.ptr is null in adjust action | action=" + actionKey + " | skipping entry");
		return;
	}

	if ((int)ref.ptr->type != ADJUST_SKILL_LEVEL)
	{
		LogError(
			"wrong item type in adjust action"
			" | action=" +
			actionKey +
			" | expected=" +
			IntToString(ADJUST_SKILL_LEVEL) +
			" | got=" + IntToString((int)ref.ptr->type));
		return;
	}

	GameData* statDef = GetStatDefinitionRecord(ref.ptr, actionKey);
	if (statDef == 0)
	{
		LogError("no STAT_DEFINITION reference set in adjust action | action=" + actionKey);
		return;
	}

	StatsEnumerated stat = ReadStatEnum(statDef);
	if (stat == STAT_NONE)
	{
		LogError("STAT_DEFINITION enum value is STAT_NONE in adjust action | action=" + actionKey);
		return;
	}

	int delta = ref.values[0];
	if (!isRemove && delta < 0)
	{
		LogWarning(
			"add action has negative value"
			" | action=" +
			actionKey +
			" | delta=" + IntToString(delta));
	}
	if (isRemove && delta < 0)
	{
		LogWarning(
			"remove action already has negative value"
			" | action=" +
			actionKey +
			" | delta=" + IntToString(delta));
	}

	// Remove actions always subtract: ensure delta is non-positive.
	// If the author stored a positive value, negate it.
	// If it is already negative or zero, leave it as-is.
	if (isRemove && delta > 0)
		delta = -delta;

	ClampConfig clamp = ReadClampProfile(ref.ptr, actionKey);
	AdjustStatForCharacter(character, stat, delta, clamp);
}

/**
 * @brief Applies one set GameDataReference to a character.
 *
 * Validates that ref.ptr is non-null and has type SET_SKILL_LEVEL (3003).
 * Navigates to the referenced STAT_DEFINITION record to read the stat enum value.
 * Reads the target value from ref.values[0], reads the optional CLAMP_PROFILE
 * reference, and calls SetStatForCharacter.
 *
 * @param character  The character to modify. May be null (no-op).
 * @param ref        The GameDataReference for this item entry.
 */
static void ApplySetRef(
	Character* character,
	const GameDataReference& ref,
	const std::string& actionKey)
{
	if (ref.ptr == 0)
	{
		LogError("ref.ptr is null in set action | action=" + actionKey + " | skipping entry");
		return;
	}

	if ((int)ref.ptr->type != SET_SKILL_LEVEL)
	{
		LogError(
			"wrong item type in set action"
			" | action=" +
			actionKey +
			" | expected=" +
			IntToString(SET_SKILL_LEVEL) +
			" | got=" + IntToString((int)ref.ptr->type));
		return;
	}

	GameData* statDef = GetStatDefinitionRecord(ref.ptr, actionKey);
	if (statDef == 0)
	{
		LogError("no STAT_DEFINITION reference set in set action | action=" + actionKey);
		return;
	}

	StatsEnumerated stat = ReadStatEnum(statDef);
	if (stat == STAT_NONE)
	{
		LogError("STAT_DEFINITION enum value is STAT_NONE in set action | action=" + actionKey);
		return;
	}

	int value = ref.values[0];
	ClampConfig clamp = ReadClampProfile(ref.ptr, actionKey);
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
	Dialogue* dlg,
	DialogLineData* dialogLine,
	GameData* lineData,
	const std::string& actionKey,
	TargetRole role,
	bool isRemove)
{
	Ogre::vector<GameDataReference>::type* refs = FindReferences(lineData, actionKey);
	if (refs == 0)
		return;

	Character* character = ResolveTarget(dlg, dialogLine, role);
	if (character == 0)
	{
		LogError(
			"could not resolve character"
			" | action=" +
			actionKey +
			" | role=" + TargetRoleToString(role));
		return;
	}

	for (auto it = refs->begin(); it != refs->end(); ++it)
	{
		ApplyAdjustRef(character, *it, actionKey, isRemove);
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
	Dialogue* dlg,
	DialogLineData* dialogLine,
	GameData* lineData,
	const std::string& actionKey,
	TargetRole role)
{
	Ogre::vector<GameDataReference>::type* refs = FindReferences(lineData, actionKey);
	if (refs == 0)
		return;

	Character* character = ResolveTarget(dlg, dialogLine, role);
	if (character == 0)
	{
		LogError(
			"could not resolve character"
			" | action=" +
			actionKey +
			" | role=" + TargetRoleToString(role));
		return;
	}

	for (auto it = refs->begin(); it != refs->end(); ++it)
	{
		ApplySetRef(character, *it, actionKey);
	}
}

/**
 * @brief Returns true if the dialogue line contains any StatModification action key.
 */
static bool HasAnyStatAction(GameData* lineData)
{
	if (lineData == 0)
		return false;

	for (int i = 0; i < STAT_ACTION_KEY_COUNT; ++i)
	{
		if (FindReferences(lineData, STAT_ACTION_KEYS[i]) != 0)
			return true;
	}

	return false;
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

	if (!HasAnyStatAction(lineData))
		return;

	LogDialogueIdentitySpike("before stat action dispatch", dlg, dialogLine);

	// Adjust actions run first (delta = values[0]).
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_ADD_SKILL_LEVELS_TO_SPEAKER, ROLE_SPEAKER, false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_ADD_SKILL_LEVELS_TO_TARGET, ROLE_TARGET, false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_ADD_SKILL_LEVELS_TO_OWNER, ROLE_OWNER, false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_REMOVE_SKILL_LEVELS_FROM_SPEAKER, ROLE_SPEAKER, true);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_REMOVE_SKILL_LEVELS_FROM_TARGET, ROLE_TARGET, true);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_REMOVE_SKILL_LEVELS_FROM_OWNER, ROLE_OWNER, true);

	// Set actions run second (value = values[0]).
	// If a stat was already adjusted above, the set value wins.
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_SET_SKILL_LEVEL_FOR_SPEAKER, ROLE_SPEAKER);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_SET_SKILL_LEVEL_FOR_TARGET, ROLE_TARGET);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_SET_SKILL_LEVEL_FOR_OWNER, ROLE_OWNER);

	LogDialogueIdentitySpike("after stat action dispatch", dlg, dialogLine);
}

// ============================================================
// SECTION 10: Stat condition evaluation
// ============================================================

/**
 * @brief Compares two displayed stat levels using a ComparisonEnum operator.
 *
 * @param value      The character's stat value (left-hand side).
 * @param threshold  The value to compare against (right-hand side).
 * @param compareBy  The comparison operator.
 * @return True if the comparison holds.
 */
static bool StatDialogCompare(int value, int threshold, ComparisonEnum compareBy)
{
	if (compareBy == CE_EQUALS && value == threshold)
		return true;
	if (compareBy == CE_LESS_THAN && value < threshold)
		return true;
	if (compareBy == CE_MORE_THAN && value > threshold)
		return true;
	return false;
}

/**
 * @brief Compares the same stat on two characters.
 *
 * left OP right, where the operator is compareBy.
 * Used by DC_STAT_LEVEL_COMPARE_UNMODIFIED and DC_STAT_LEVEL_COMPARE_MODIFIED.
 * The caller determines which character is left and which is right
 * via the who field on the condition (T_ME = owner is left, otherwise
 * target is left).
 *
 * @param conditionType  DC_STAT_LEVEL_COMPARE_UNMODIFIED or DC_STAT_LEVEL_COMPARE_MODIFIED.
 * @param left           The left-hand character. May be null (returns false).
 * @param right          The right-hand character. May be null (returns false).
 * @param compareBy      The comparison operator.
 * @param stat           The stat to read from both characters.
 * @return True if left OP right holds.
 */
static bool EvaluateStatComparison(
	ExtendedDialogConditionEnum conditionType,
	Character* left,
	Character* right,
	ComparisonEnum compareBy,
	StatsEnumerated stat)
{
	if (left == 0 || right == 0)
	{
		LogError("comparison condition character is null");
		return false;
	}

	if (stat == STAT_NONE)
	{
		LogError("comparison condition stat is STAT_NONE");
		return false;
	}

	CharStats* leftStats = left->getStats();
	CharStats* rightStats = right->getStats();
	if (leftStats == 0 || rightStats == 0)
	{
		LogError("character->getStats() returned null during comparison condition check");
		return false;
	}

	bool unmodified = (conditionType == DC_STAT_LEVEL_COMPARE_UNMODIFIED);
	float leftValue = leftStats->getStat(stat, unmodified);
	float rightValue = rightStats->getStat(stat, unmodified);

	// Match the player's visible stat-level granularity: 90.5 and 90.1 both
	// compare as level 90.
	return StatDialogCompare((int)leftValue, (int)rightValue, compareBy);
}

/**
 * @brief Returns true when a condition ID belongs to this plugin.
 */
static bool IsStatCondition(int conditionType)
{
	return conditionType == DC_STAT_LEVEL_COMPARE_UNMODIFIED ||
		conditionType == DC_STAT_LEVEL_COMPARE_MODIFIED;
}

/**
 * @brief Parsed fields from one StatModification condition record.
 */
struct StatConditionFields
{
	ExtendedDialogConditionEnum conditionType;
	ComparisonEnum compareBy;
	TalkerEnum who;
	StatsEnumerated stat;
};

/**
 * @brief Reads the fields required by a StatModification condition record.
 */
static bool ReadStatConditionFields(
	GameData* conditionRecord,
	ExtendedDialogConditionEnum conditionType,
	StatConditionFields& out)
{
	if (conditionRecord == 0)
	{
		LogError("stat condition record is null");
		return false;
	}

	auto compareByIt = conditionRecord->idata.find(FIELD_COMPARE_BY);
	auto whoIt = conditionRecord->idata.find(FIELD_WHO);
	auto tagIt = conditionRecord->idata.find(FIELD_TAG);
	if (compareByIt == conditionRecord->idata.end() ||
		whoIt == conditionRecord->idata.end() ||
		tagIt == conditionRecord->idata.end())
	{
		LogError(
			"stat condition is missing compare by, who, or tag"
			" | condition={" +
			DescribeGameData(conditionRecord) + "}");
		return false;
	}

	out.conditionType = conditionType;
	out.compareBy = (ComparisonEnum)compareByIt->second;
	out.who = (TalkerEnum)whoIt->second;
	out.stat = (StatsEnumerated)tagIt->second;
	return true;
}

/** @brief Saved pointer to the original DialogLineData::checkTags. */
static bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target) = 0;

/**
 * @brief Hook for DialogLineData::checkTags.
 *
 * Evaluates StatModification_Extension dialogue conditions.
 * Only enters the condition scan if at least one condition key on this
 * dialogue line matches a supported extended type, then processes all
 * custom conditions from objectReferences["conditions"].
 * Conditions from other plugins are ignored (returns true for them).
 * The original is always called last so built-in conditions still evaluate.
 *
 * Note: T_WHOLE_SQUAD is not supported. T_ME means owner OP target;
 * all other TalkerEnum values mean target OP owner.
 *
 * @param thisptr  The dialogue line being evaluated.
 * @param me       The dialogue owner (typically the NPC).
 * @param target   The conversation target (typically the player).
 */
static bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target)
{
	if (thisptr == 0)
	{
		LogError("checkTags_hook received null DialogLineData");
		return false;
	}

	LogConditionIdentitySpike("checkTags entry", thisptr, me, target);

	bool hasExtendedCondition = false;
	for (int i = 0; i < (int)thisptr->conditions.size(); ++i)
	{
		int key = thisptr->conditions[i]->key;
		if (IsStatCondition(key))
		{
			hasExtendedCondition = true;
			break;
		}
	}

	if (hasExtendedCondition)
	{
		LogConditionIdentitySpike("before stat condition evaluation", thisptr, me, target);

		GameData* lineData = thisptr->getGameData();
		if (lineData == 0)
		{
			LogError("stat condition found but dialogue line GameData is null");
			return false;
		}

		Ogre::vector<GameDataReference>::type* conditionRefs = FindReferences(lineData, REF_CONDITIONS);
		if (conditionRefs == 0)
		{
			LogError("stat condition found but condition references are missing or empty");
			return false;
		}

		bool evaluatedStatCondition = false;
		for (int i = 0; i < (int)conditionRefs->size(); ++i)
		{
			GameData* cond = (*conditionRefs)[i].ptr;
			if (cond == 0)
			{
				LogError("null condition reference while evaluating stat condition");
				continue;
			}

			auto condNameIt = cond->idata.find(FIELD_CONDITION_NAME);
			if (condNameIt == cond->idata.end())
			{
				LogError("condition record is missing condition name while evaluating stat condition | condition={" + DescribeGameData(cond) + "}");
				continue;
			}

			int conditionInt = condNameIt->second;
			if (!IsStatCondition(conditionInt))
				continue;

			evaluatedStatCondition = true;

			StatConditionFields fields;
			ExtendedDialogConditionEnum conditionType =
				(ExtendedDialogConditionEnum)conditionInt;
			if (!ReadStatConditionFields(cond, conditionType, fields))
				return false;

			if (fields.who == T_WHOLE_SQUAD)
			{
				LogError("T_WHOLE_SQUAD is not supported for stat conditions | condition={" + DescribeGameData(cond) + "}");
				return false;
			}

			Character* left = (fields.who == T_ME) ? me : target;
			Character* right = (fields.who == T_ME) ? target : me;

			if (!EvaluateStatComparison(fields.conditionType, left, right, fields.compareBy, fields.stat))
				return false;
		}

		if (!evaluatedStatCondition)
		{
			LogError("stat condition key found but no matching condition record was evaluated | line={" + DescribeGameData(lineData) + "}");
			return false;
		}
	}

	if (checkTags_orig == 0)
	{
		LogError("checkTags_orig is null, cannot call original");
		return true;
	}

	bool result = checkTags_orig(thisptr, me, target);
	if (LOG_DIALOGUE_IDENTITY_SPIKE)
	{
		LogInfo(
			"identity spike"
			" | phase=checkTags original returned"
			" | result=" +
			IntToString(result ? 1 : 0) +
			" | line={" + DescribeGameData(thisptr == 0 ? 0 : thisptr->getGameData()) + "}");
	}

	return result;
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
		LogError("_doActions_orig is null, cannot call original");
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
		LogError("failed to hook Dialogue::_doActions");
		return;
	}

	if (KenshiLib::SUCCESS != KenshiLib::AddHook(
		KenshiLib::GetRealAddress(&DialogLineData::checkTags),
		&checkTags_hook,
		&checkTags_orig))
	{
		LogError("failed to hook DialogLineData::checkTags");
		return;
	}

	LogInfo("loaded and hooks installed");
}

