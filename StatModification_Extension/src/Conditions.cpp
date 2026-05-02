#include "Conditions.h"

#include "Constants.h"
#include "FcsData.h"
#include "Logging.h"
#include "Targets.h"

#include <kenshi/CharStats.h>

bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target) = 0;

/** @brief Compares two displayed whole-number stat levels using a Kenshi comparison operator. */
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
 * This is intentionally not the same condition shape as BFrizzle's Dialogue
 * stat threshold checks. Dialogue's checks answer "is this character's stat
 * above/below a fixed value?" These comparison conditions answer "how do two
 * participants compare to each other?"
 *
 * The caller chooses left and right based on the FCS "who" field:
 * T_ME and T_WHOLE_SQUAD mean owner-side OP target; every other value means
 * target OP owner-side. T_WHOLE_SQUAD is not true squad aggregation here:
 * checkTags gives us the current evaluated me/target pair, not a squad collection.
 *
 * Kenshi stores stats as floats, but the player-facing UI shows whole stat
 * levels. Casting to int means two visible level-90 characters compare equal
 * even if their hidden values are 90.1 and 90.5. This is important for mod
 * author expectations: exact float equality would almost never be useful.
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

	return StatDialogCompare((int)leftValue, (int)rightValue, compareBy);
}

/** @brief Returns true when a dialogue condition ID belongs to this plugin. */
static bool IsStatCondition(int conditionType)
{
	return conditionType == DC_STAT_LEVEL_COMPARE_UNMODIFIED ||
		conditionType == DC_STAT_LEVEL_COMPARE_MODIFIED;
}

/** @brief Parsed fields from one StatModification condition record. */
struct StatConditionFields
{
	ExtendedDialogConditionEnum conditionType;
	ComparisonEnum compareBy;
	TalkerEnum who;
	StatsEnumerated stat;
};

/** @brief Reads the fields required by a StatModification comparison condition record. */
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
		LogError("stat condition is missing compare by, who, or tag | condition={" + DescribeGameData(conditionRecord) + "}");
		return false;
	}

	out.conditionType = conditionType;
	out.compareBy = (ComparisonEnum)compareByIt->second;
	out.who = (TalkerEnum)whoIt->second;
	out.stat = (StatsEnumerated)tagIt->second;
	return true;
}

/**
 * @brief Evaluates StatModification comparison conditions and then calls original checkTags.
 *
 * If one of our condition IDs is present but the backing FCS condition data is
 * missing or malformed, this fails closed and logs the reason. Conditions from
 * other plugins are ignored here and left to the original hook chain.
 */
bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target)
{
	if (thisptr == 0)
	{
		LogError("checkTags_hook received null DialogLineData");
		return false;
	}

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
			ExtendedDialogConditionEnum conditionType = (ExtendedDialogConditionEnum)conditionInt;
			if (!ReadStatConditionFields(cond, conditionType, fields))
				return false;

			if (fields.who == T_WHOLE_SQUAD)
				LogWarning("T_WHOLE_SQUAD condition uses current evaluated owner-side character, not whole-squad aggregation | condition={" + DescribeGameData(cond) + "}");

			bool ownerSideIsLeft = (fields.who == T_ME || fields.who == T_WHOLE_SQUAD);
			Character* left = ownerSideIsLeft ? me : target;
			Character* right = ownerSideIsLeft ? target : me;

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

	return checkTags_orig(thisptr, me, target);
}
