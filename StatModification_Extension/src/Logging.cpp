#include "Logging.h"

#include <Debug.h>

#include <kenshi/Character.h>
#include <kenshi/GameData.h>

#include <sstream>

/**
 * Toggle noisy success-path diagnostics.
 *
 * Release builds are what most RE_Kenshi users run, so we keep this explicit
 * instead of relying on compiler configuration. Leave failures/warnings
 * always-on; use this only for "it worked" details like before/after stat values.
 */
static const bool ENABLE_DEBUG_LOGS = false;

std::string IntToString(int value)
{
	std::ostringstream ss;
	ss << value;
	return ss.str();
}

std::string FloatToString(float value)
{
	std::ostringstream ss;
	ss << value;
	return ss.str();
}

std::string PointerToString(const void* ptr)
{
	std::ostringstream ss;
	ss << ptr;
	return ss.str();
}

void LogInfo(const std::string& message)
{
	DebugLog("StatModification: " + message);
}

void LogDebug(const std::string& message)
{
	if (!ENABLE_DEBUG_LOGS)
		return;

	DebugLog("StatModification DEBUG: " + message);
}

void LogWarning(const std::string& message)
{
	DebugLog("StatModification WARNING: " + message);
}

void LogError(const std::string& message)
{
	ErrorLog("StatModification: " + message);
}

std::string DescribeGameData(GameData* data)
{
	if (data == 0)
		return "null";

	return "name=\"" + data->name +
		"\" | stringID=\"" + data->stringID +
		"\" | id=" + IntToString(data->id) +
		" | type=" + IntToString((int)data->type);
}

std::string DescribeCharacter(Character* character)
{
	if (character == 0)
		return "null";

	return "ptr=" + PointerToString(character) +
		" | name=\"" + character->getName() +
		"\" | data={" + DescribeGameData(character->getGameData()) + "}";
}
