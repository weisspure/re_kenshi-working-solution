#pragma once

#include <string>

/** Runtime target selected by a RaceChange dialogue action key. */
enum RaceChangeTargetRole
{
	RACE_CHANGE_ROLE_UNKNOWN = 0,
	RACE_CHANGE_ROLE_SPEAKER,
	RACE_CHANGE_ROLE_OTHER
};

/** Transform path requested by the FCS action reference value. */
enum RaceChangeIntent
{
	RACE_CHANGE_INTENT_HUMANOID = 0,
	RACE_CHANGE_INTENT_ANIMAL = 1,
	RACE_CHANGE_INTENT_UNSUPPORTED
};

/** Public FCS action key that targets the resolved dialogue speaker. */
static const char* ACTION_CHANGE_RACE = "change race";

/** Public FCS action key that targets the other main dialogue side. */
static const char* ACTION_CHANGE_OTHER_RACE = "change other race";

/** Map an FCS action key to the target role it represents. */
RaceChangeTargetRole GetRaceChangeActionRole(const std::string& actionKey);

/** Check whether an action key belongs to the RaceChange public surface. */
bool IsRaceChangeActionKey(const std::string& actionKey);

/** Validate the minimum data required before applying a race-change reference. */
bool CanApplyRaceChangeAction(bool hasCharacter, bool hasTargetRace, bool referencedRecordIsRaceType);

/** Interpret the public `value[0]` intent flag from a dialogue action reference. */
RaceChangeIntent GetRaceChangeIntent(int value);

/** Convert an intent enum to a stable lowercase diagnostic string. */
const char* RaceChangeIntentToString(RaceChangeIntent intent);
