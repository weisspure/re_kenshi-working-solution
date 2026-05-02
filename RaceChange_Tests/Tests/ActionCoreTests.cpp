#define BOOST_TEST_MODULE RaceChangeTests
#include <boost/test/included/unit_test.hpp>

#include "ActionCore.h"

BOOST_AUTO_TEST_CASE(change_race_action_targets_resolved_speaker)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_ROLE_SPEAKER, GetRaceChangeActionRole(ACTION_CHANGE_RACE));
}

BOOST_AUTO_TEST_CASE(change_other_race_action_targets_other_dialogue_side)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_ROLE_OTHER, GetRaceChangeActionRole(ACTION_CHANGE_OTHER_RACE));
}

BOOST_AUTO_TEST_CASE(unrecognized_action_key_is_not_treated_as_race_change)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_ROLE_UNKNOWN, GetRaceChangeActionRole("not a race change action"));
}

BOOST_AUTO_TEST_CASE(only_public_race_change_action_keys_are_recognized)
{
	BOOST_CHECK(IsRaceChangeActionKey(ACTION_CHANGE_RACE));
	BOOST_CHECK(IsRaceChangeActionKey(ACTION_CHANGE_OTHER_RACE));
	BOOST_CHECK(!IsRaceChangeActionKey("not a race change action"));
}

BOOST_AUTO_TEST_CASE(race_change_validation_matches_vanilla_editor_entry_shape)
{
	BOOST_CHECK(CanApplyRaceChangeAction(true, true, true));

	BOOST_CHECK(!CanApplyRaceChangeAction(false, true, true));
	BOOST_CHECK(!CanApplyRaceChangeAction(true, false, true));
	BOOST_CHECK(!CanApplyRaceChangeAction(true, true, false));
}
