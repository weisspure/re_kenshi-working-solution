#pragma once

struct ClampConfig
{
	bool doClamp;	///< True only when an explicit CLAMP_PROFILE was referenced.
	float minValue; ///< Inclusive lower bound. Ignored when doClamp is false.
	float maxValue; ///< Inclusive upper bound. Ignored when doClamp is false.
};

/**
 * @brief Applies a ClampConfig to a value.
 *
 * @return The original value when clamping is disabled.
 */
float ApplyClamp(float value, const ClampConfig& clamp);
