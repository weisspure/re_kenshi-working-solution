#include "ClampCore.h"

float ApplyClamp(float value, const ClampConfig& clamp)
{
	if (!clamp.doClamp)
		return value;

	if (value < clamp.minValue)
		return clamp.minValue;

	if (value > clamp.maxValue)
		return clamp.maxValue;

	return value;
}
