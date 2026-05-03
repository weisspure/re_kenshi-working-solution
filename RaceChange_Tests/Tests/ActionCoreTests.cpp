#define BOOST_TEST_MODULE RaceChangeTests
#include <boost/test/included/unit_test.hpp>

#include "ActionCore.h"
#include "Logging.h"

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

BOOST_AUTO_TEST_CASE(race_change_reference_value_selects_transform_intent)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_INTENT_HUMANOID, GetRaceChangeIntent(0));
	BOOST_CHECK_EQUAL(RACE_CHANGE_INTENT_ANIMAL, GetRaceChangeIntent(1));
}

BOOST_AUTO_TEST_CASE(unsupported_race_change_reference_values_fail_closed)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_INTENT_UNSUPPORTED, GetRaceChangeIntent(-1));
	BOOST_CHECK_EQUAL(RACE_CHANGE_INTENT_UNSUPPORTED, GetRaceChangeIntent(2));
}

BOOST_AUTO_TEST_CASE(log_level_names_parse_case_insensitively)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_LOG_ERROR, ParseRaceChangeLogLevel("error"));
	BOOST_CHECK_EQUAL(RACE_CHANGE_LOG_WARNING, ParseRaceChangeLogLevel("WARNING"));
	BOOST_CHECK_EQUAL(RACE_CHANGE_LOG_INFO, ParseRaceChangeLogLevel("Info"));
	BOOST_CHECK_EQUAL(RACE_CHANGE_LOG_DEBUG, ParseRaceChangeLogLevel("debug"));
	BOOST_CHECK_EQUAL(RACE_CHANGE_LOG_TRACE, ParseRaceChangeLogLevel("TRACE"));
}

BOOST_AUTO_TEST_CASE(unknown_log_level_names_fall_back_to_default)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_LOG_INFO, ParseRaceChangeLogLevel(""));
	BOOST_CHECK_EQUAL(RACE_CHANGE_LOG_INFO, ParseRaceChangeLogLevel("verbose"));
}

BOOST_AUTO_TEST_CASE(log_level_filtering_emits_only_threshold_and_above)
{
	BOOST_CHECK(ShouldLogRaceChangeMessage(RACE_CHANGE_LOG_INFO, RACE_CHANGE_LOG_ERROR));
	BOOST_CHECK(ShouldLogRaceChangeMessage(RACE_CHANGE_LOG_INFO, RACE_CHANGE_LOG_WARNING));
	BOOST_CHECK(ShouldLogRaceChangeMessage(RACE_CHANGE_LOG_INFO, RACE_CHANGE_LOG_INFO));
	BOOST_CHECK(!ShouldLogRaceChangeMessage(RACE_CHANGE_LOG_INFO, RACE_CHANGE_LOG_DEBUG));
	BOOST_CHECK(!ShouldLogRaceChangeMessage(RACE_CHANGE_LOG_INFO, RACE_CHANGE_LOG_TRACE));

	BOOST_CHECK(ShouldLogRaceChangeMessage(RACE_CHANGE_LOG_ERROR, RACE_CHANGE_LOG_ERROR));
	BOOST_CHECK(!ShouldLogRaceChangeMessage(RACE_CHANGE_LOG_ERROR, RACE_CHANGE_LOG_WARNING));
}
