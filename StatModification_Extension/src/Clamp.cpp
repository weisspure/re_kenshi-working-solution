#include "Clamp.h"

#include "Constants.h"
#include "FcsData.h"
#include "Logging.h"

/**
 * @brief Reads the ClampConfig from an optional CLAMP_PROFILE reference.
 *
 * Absence of a profile means unclamped. Presence means the author opted into
 * a policy, so malformed profiles are logged loudly rather than guessed.
 */
ClampConfig ReadClampProfile(GameData* actionRecord, const std::string& context)
{
	ClampConfig result;
	result.doClamp = false; // Default: no profile = no clamping
	result.minValue = 0.0f;
	result.maxValue = 0.0f; // Unused unless doClamp is true

	if (actionRecord == 0)
		return result;

	Ogre::vector<GameDataReference>::type* refs = FindReferences(actionRecord, REF_CLAMP_PROFILE);
	if (refs == 0)
		return result; // No reference - unclamped

	GameData* profile = (*refs)[0].ptr;
	if (profile == 0)
	{
		LogError("clamp profile reference is null | context=" + context + " | continuing unclamped");
		return result;
	}

	if ((int)profile->type != CLAMP_PROFILE)
	{
		LogError(
			"wrong item type for clamp profile"
			" | context=" +
			context +
			" | expected=" +
			IntToString(CLAMP_PROFILE) +
			" | got=" + IntToString((int)profile->type) +
			" | continuing unclamped");
		return result;
	}

	auto minIt = profile->idata.find(FIELD_CLAMP_MIN);
	auto maxIt = profile->idata.find(FIELD_CLAMP_MAX);
	if (minIt == profile->idata.end() || maxIt == profile->idata.end())
	{
		LogError("clamp profile is missing clamp min or clamp max | context=" + context + " | continuing unclamped");
		return result;
	}

	float minValue = (float)minIt->second;
	float maxValue = (float)maxIt->second;
	if (minValue > maxValue)
	{
		LogError(
			"clamp profile min is greater than max"
			" | context=" +
			context +
			" | min=" +
			FloatToString(minValue) +
			" | max=" + FloatToString(maxValue) +
			" | continuing unclamped");
		return result;
	}

	result.doClamp = true;
	result.minValue = minValue;
	result.maxValue = maxValue;
	return result;
}
