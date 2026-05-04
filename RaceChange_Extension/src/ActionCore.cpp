#include "ActionCore.h"

RaceChangeTargetRole GetRaceChangeActionRole(const std::string& actionKey)
{
	if (actionKey == ACTION_CHANGE_RACE)
		return RACE_CHANGE_ROLE_SPEAKER;

	if (actionKey == ACTION_CHANGE_OTHER_RACE)
		return RACE_CHANGE_ROLE_OTHER;

	return RACE_CHANGE_ROLE_UNKNOWN;
}

bool IsRaceChangeActionKey(const std::string& actionKey)
{
	return GetRaceChangeActionRole(actionKey) != RACE_CHANGE_ROLE_UNKNOWN;
}

bool CanApplyRaceChangeAction(bool hasCharacter, bool hasTargetRace, bool referencedRecordIsRaceType)
{
	return hasCharacter && hasTargetRace && referencedRecordIsRaceType;
}

RaceChangeIntent GetRaceChangeIntent(int value)
{
	if (value == 0)
		return RACE_CHANGE_INTENT_HUMANOID;

	if (value == 1)
		return RACE_CHANGE_INTENT_ANIMAL;

	return RACE_CHANGE_INTENT_UNSUPPORTED;
}

const char* RaceChangeIntentToString(RaceChangeIntent intent)
{
	switch (intent)
	{
	case RACE_CHANGE_INTENT_HUMANOID:
		return "humanoid";
	case RACE_CHANGE_INTENT_ANIMAL:
		return "animal";
	default:
		return "unsupported";
	}
}

RaceChangePath SelectRaceChangePath(RaceChangeIntent intent, bool targetRaceHasAnimalTemplate)
{
	if (intent == RACE_CHANGE_INTENT_UNSUPPORTED)
		return RACE_CHANGE_PATH_NONE;

	if (intent == RACE_CHANGE_INTENT_ANIMAL && targetRaceHasAnimalTemplate)
		return RACE_CHANGE_PATH_ANIMAL_REPLACEMENT;

	if (intent == RACE_CHANGE_INTENT_ANIMAL)
		return RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY;

	if (targetRaceHasAnimalTemplate)
		return RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY;

	return RACE_CHANGE_PATH_IN_PLACE_ARMOUR_ONLY;
}

const char* RaceChangePathToString(RaceChangePath path)
{
	switch (path)
	{
	case RACE_CHANGE_PATH_IN_PLACE_ARMOUR_ONLY:
		return "in-place-armour-only";
	case RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY:
		return "in-place-full-inventory";
	case RACE_CHANGE_PATH_ANIMAL_REPLACEMENT:
		return "animal-replacement";
	default:
		return "none";
	}
}
