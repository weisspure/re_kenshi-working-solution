#include "Actions.h"

#include "Clamp.h"
#include "Constants.h"
#include "FcsData.h"
#include "Logging.h"
#include "Targets.h"

#include <kenshi/CharStats.h>
#include <kenshi/Character.h>

#include <vector>

/**
 * @brief Adds a signed integer delta to a character's stat, then applies optional clamping.
 *
 * This mutates CharStats directly through getStatRef. It does not grant XP,
 * trigger training, or use Kenshi's normal progression path.
 */
static void AdjustStatForCharacter(Character* character, StatsEnumerated stat, int delta, const ClampConfig& clamp, const std::string& actionKey)
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

	LogDebug(
		"adjusted " + CharStats::getStatName(stat) +
		" | action=" + actionKey +
		" | character={" + DescribeCharacter(character) + "}" +
		" | delta=" + IntToString(delta) +
		" | before=" + FloatToString(before) +
		" | after=" + FloatToString(after));
}

/**
 * @brief Sets a character's stat to an exact integer value, then applies optional clamping.
 *
 * Like adjustment, this directly writes the stored stat value and does not grant XP.
 */
static void SetStatForCharacter(Character* character, StatsEnumerated stat, int value, const ClampConfig& clamp, const std::string& actionKey)
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

	LogDebug(
		"set " + CharStats::getStatName(stat) +
		" | action=" + actionKey +
		" | character={" + DescribeCharacter(character) + "}" +
		" | to=" + IntToString(value) +
		" | before=" + FloatToString(before) +
		" | after=" + FloatToString(after));
}

/**
 * @brief Applies one ADJUST_SKILL_LEVEL reference from an FCS dialogue line.
 *
 * ref.ptr is the custom action record; ref.values[0] is the authored amount.
 * The action record must reference a STAT_DEFINITION record under objectReferences["stat"].
 * Untrain actions always subtract: positive authored values are negated, while already
 * negative values are kept and logged as warnings so authors can spot odd data.
 */
static void ApplyAdjustRef(Character* character, const GameDataReference& ref, const std::string& actionKey, bool isRemove)
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
		LogWarning("train action has negative value | action=" + actionKey + " | delta=" + IntToString(delta));
	if (isRemove && delta < 0)
		LogWarning("untrain action already has negative value | action=" + actionKey + " | delta=" + IntToString(delta));

	// Untrain actions always subtract. If the author stored a positive value, negate it.
	// If it is already negative or zero, leave it as-is.
	if (isRemove && delta > 0)
		delta = -delta;

	ClampConfig clamp = ReadClampProfile(ref.ptr, actionKey);
	AdjustStatForCharacter(character, stat, delta, clamp, actionKey);
}

/**
 * @brief Applies one SET_SKILL_LEVEL reference from an FCS dialogue line.
 *
 * ref.ptr is the custom set record; ref.values[0] is the authored target value.
 * The set record must reference a STAT_DEFINITION record under objectReferences["stat"].
 */
static void ApplySetRef(Character* character, const GameDataReference& ref, const std::string& actionKey)
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
	SetStatForCharacter(character, stat, value, clamp, actionKey);
}

/**
 * @brief Looks up one adjust action key and applies every referenced record.
 *
 * Absent keys are expected and silently skipped. Present keys with malformed
 * records log clear errors and skip only the bad entry.
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

	std::vector<Character*> characters = ResolveTargets(dlg, dialogLine, role);
	if (characters.empty())
	{
		LogError("could not resolve any characters | action=" + actionKey + " | role=" + TargetRoleToString(role));
		return;
	}

	for (auto it = refs->begin(); it != refs->end(); ++it)
	{
		for (auto characterIt = characters.begin(); characterIt != characters.end(); ++characterIt)
		{
			ApplyAdjustRef(*characterIt, *it, actionKey, isRemove);
		}
	}
}

/**
 * @brief Looks up one set action key and applies every referenced record.
 *
 * This mirrors TryApplyAdjustAction so the dispatcher stays explicit and readable.
 */
static void TryApplySetAction(Dialogue* dlg, DialogLineData* dialogLine, GameData* lineData, const std::string& actionKey, TargetRole role)
{
	Ogre::vector<GameDataReference>::type* refs = FindReferences(lineData, actionKey);
	if (refs == 0)
		return;

	std::vector<Character*> characters = ResolveTargets(dlg, dialogLine, role);
	if (characters.empty())
	{
		LogError("could not resolve any characters | action=" + actionKey + " | role=" + TargetRoleToString(role));
		return;
	}

	for (auto it = refs->begin(); it != refs->end(); ++it)
	{
		for (auto characterIt = characters.begin(); characterIt != characters.end(); ++characterIt)
		{
			ApplySetRef(*characterIt, *it, actionKey);
		}
	}
}

/** @brief Returns true if a dialogue line contains at least one StatModification action key. */
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
 * @brief Applies all supported stat actions found on one dialogue line.
 *
 * Each known FCS objectReferences key is checked independently. This is deliberately
 * explicit rather than a dynamic registry so beginner plugin authors can see the
 * full action surface in one place.
 */
void DispatchStatActions(Dialogue* dlg, DialogLineData* dialogLine)
{
	if (dlg == 0 || dialogLine == 0)
		return;

	GameData* lineData = dialogLine->getGameData();
	if (lineData == 0)
		return;

	if (!HasAnyStatAction(lineData))
		return;

	// Train/untrain actions run first (delta = values[0]).
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_TRAIN_SKILL_LEVELS, ROLE_SPEAKER, false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_UNTRAIN_SKILL_LEVELS, ROLE_SPEAKER, true);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_TRAIN_OTHER_SKILL_LEVELS, ROLE_OTHER, false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_UNTRAIN_OTHER_SKILL_LEVELS, ROLE_OTHER, true);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_TRAIN_SQUAD_SKILL_LEVELS, ROLE_SPEAKER_SQUAD, false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_UNTRAIN_SQUAD_SKILL_LEVELS, ROLE_SPEAKER_SQUAD, true);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_TRAIN_OTHER_SQUAD_SKILL_LEVELS, ROLE_OTHER_SQUAD, false);
	TryApplyAdjustAction(dlg, dialogLine, lineData, ACTION_UNTRAIN_OTHER_SQUAD_SKILL_LEVELS, ROLE_OTHER_SQUAD, true);

	// Until actions run second (value = values[0]). If a stat was adjusted above, until wins.
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_TRAIN_SKILL_LEVELS_UNTIL, ROLE_SPEAKER);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_TRAIN_OTHER_SKILL_LEVELS_UNTIL, ROLE_OTHER);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_UNTRAIN_SKILL_LEVELS_UNTIL, ROLE_SPEAKER);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_UNTRAIN_OTHER_SKILL_LEVELS_UNTIL, ROLE_OTHER);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_TRAIN_SQUAD_SKILL_LEVELS_UNTIL, ROLE_SPEAKER_SQUAD);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_TRAIN_OTHER_SQUAD_SKILL_LEVELS_UNTIL, ROLE_OTHER_SQUAD);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_UNTRAIN_SQUAD_SKILL_LEVELS_UNTIL, ROLE_SPEAKER_SQUAD);
	TryApplySetAction(dlg, dialogLine, lineData, ACTION_UNTRAIN_OTHER_SQUAD_SKILL_LEVELS_UNTIL, ROLE_OTHER_SQUAD);
}
