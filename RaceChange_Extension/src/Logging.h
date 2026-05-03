#pragma once

#include <cctype>
#include <string>

class Character;
class GameData;

/** Severity threshold for RaceChange logging. Lower values are more severe. */
enum RaceChangeLogLevel
{
	RACE_CHANGE_LOG_ERROR = 0,
	RACE_CHANGE_LOG_WARNING = 1,
	RACE_CHANGE_LOG_INFO = 2,
	RACE_CHANGE_LOG_DEBUG = 3,
	RACE_CHANGE_LOG_TRACE = 4
};

/** Default logging threshold used until runtime config loading is added. */
static const RaceChangeLogLevel RACE_CHANGE_LOG_DEFAULT = RACE_CHANGE_LOG_INFO;

/** Parse a case-insensitive log-level name, falling back to the default. */
inline RaceChangeLogLevel ParseRaceChangeLogLevel(const std::string& value)
{
	std::string lowered;
	for (std::string::const_iterator it = value.begin(); it != value.end(); ++it)
		lowered += (char)std::tolower((unsigned char)*it);

	if (lowered == "error")
		return RACE_CHANGE_LOG_ERROR;
	if (lowered == "warning" || lowered == "warn")
		return RACE_CHANGE_LOG_WARNING;
	if (lowered == "info")
		return RACE_CHANGE_LOG_INFO;
	if (lowered == "debug")
		return RACE_CHANGE_LOG_DEBUG;
	if (lowered == "trace")
		return RACE_CHANGE_LOG_TRACE;

	return RACE_CHANGE_LOG_DEFAULT;
}

/** Decide whether a message should be emitted for the configured threshold. */
inline bool ShouldLogRaceChangeMessage(RaceChangeLogLevel configuredLevel, RaceChangeLogLevel messageLevel)
{
	return messageLevel <= configuredLevel;
}

/** Convert an integer to a string using tooling compatible with VC++ 2010. */
std::string IntToString(int value);

/** Format a pointer for diagnostics. */
std::string PointerToString(const void* ptr);

/** Describe a GameData record for support logs. */
std::string DescribeGameData(GameData* data);

/** Describe a runtime character for support logs. */
std::string DescribeCharacter(Character* character);

/** Write a high-level RaceChange lifecycle message. */
void LogInfo(const std::string& message);

/** Write a detailed RaceChange development diagnostic when debug logging is enabled. */
void LogDebug(const std::string& message);

/** Write a noisy RaceChange diagnostic when trace logging is enabled. */
void LogTrace(const std::string& message);

/** Write a recoverable RaceChange warning. */
void LogWarning(const std::string& message);

/** Write a RaceChange error for an action that cannot complete safely. */
void LogError(const std::string& message);
