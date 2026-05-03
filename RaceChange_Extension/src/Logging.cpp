#include "Logging.h"

#include <Debug.h>

#include <kenshi/Character.h>
#include <kenshi/GameData.h>

#include <sstream>

/**
 * Keep the plugin-facing log levels independent from the native RE_Kenshi sink names.
 * This KenshiLib build exposes DebugLog and ErrorLog only, so warning/info/debug/trace
 * all route through DebugLog after our own filtering. Call sites should still use the
 * semantic RaceChange level that describes why the message exists.
 */
static RaceChangeLogLevel GetConfiguredLogLevel()
{
	return RACE_CHANGE_LOG_DEFAULT;
}

static bool ShouldWriteLog(RaceChangeLogLevel level)
{
	return ShouldLogRaceChangeMessage(GetConfiguredLogLevel(), level);
}

std::string IntToString(int value)
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
	if (!ShouldWriteLog(RACE_CHANGE_LOG_INFO))
		return;

	DebugLog("RaceChange: " + message);
}

void LogDebug(const std::string& message)
{
	if (!ShouldWriteLog(RACE_CHANGE_LOG_DEBUG))
		return;

	DebugLog("RaceChange DEBUG: " + message);
}

void LogTrace(const std::string& message)
{
	if (!ShouldWriteLog(RACE_CHANGE_LOG_TRACE))
		return;

	DebugLog("RaceChange TRACE: " + message);
}

void LogWarning(const std::string& message)
{
	if (!ShouldWriteLog(RACE_CHANGE_LOG_WARNING))
		return;

	DebugLog("RaceChange WARNING: " + message);
}

void LogError(const std::string& message)
{
	if (!ShouldWriteLog(RACE_CHANGE_LOG_ERROR))
		return;

	ErrorLog("RaceChange: " + message);
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
		"\" | player=" + std::string(character->isPlayerCharacter() ? "true" : "false") +
		" | human=" + std::string(character->isHuman() != 0 ? "true" : "false") +
		" | data={" + DescribeGameData(character->getGameData()) + "}";
}

