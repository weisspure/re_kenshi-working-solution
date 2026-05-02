#pragma once

#include <string>

enum RaceChangeTargetRole
{
	RACE_CHANGE_ROLE_UNKNOWN = 0,
	RACE_CHANGE_ROLE_SPEAKER,
	RACE_CHANGE_ROLE_OTHER
};

static const char* ACTION_CHANGE_RACE = "change race";
static const char* ACTION_CHANGE_OTHER_RACE = "change other race";

RaceChangeTargetRole GetRaceChangeActionRole(const std::string& actionKey);
bool IsRaceChangeActionKey(const std::string& actionKey);
bool CanApplyRaceChangeAction(bool hasCharacter, bool hasTargetRace, bool referencedRecordIsRaceType);
