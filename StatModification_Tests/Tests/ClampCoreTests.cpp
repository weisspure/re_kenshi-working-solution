#define BOOST_TEST_MODULE StatModificationTests
#include <boost/test/included/unit_test.hpp>

#include "ClampCore.h"

BOOST_AUTO_TEST_CASE(apply_clamp_returns_original_value_when_disabled)
{
	ClampConfig clamp;
	clamp.doClamp = false;
	clamp.minValue = 10.0f;
	clamp.maxValue = 20.0f;

	BOOST_CHECK_EQUAL(42.0f, ApplyClamp(42.0f, clamp));
}

BOOST_AUTO_TEST_CASE(apply_clamp_limits_values_to_configured_range)
{
	ClampConfig clamp;
	clamp.doClamp = true;
	clamp.minValue = 10.0f;
	clamp.maxValue = 20.0f;

	BOOST_CHECK_EQUAL(10.0f, ApplyClamp(5.0f, clamp));
	BOOST_CHECK_EQUAL(15.0f, ApplyClamp(15.0f, clamp));
	BOOST_CHECK_EQUAL(20.0f, ApplyClamp(25.0f, clamp));
}
