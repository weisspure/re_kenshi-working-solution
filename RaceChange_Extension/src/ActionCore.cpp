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
