#pragma once

#include <string>

class Character;
class GameData;

/** @brief Converts an int to its decimal string representation for log messages. */
std::string IntToString(int value);

/** @brief Converts a float to its string representation for log messages. */
std::string FloatToString(float value);

/** @brief Converts a pointer value to a compact diagnostic string. */
std::string PointerToString(const void* ptr);

/** @brief Writes a namespaced info message to the RE_KENSHI debug log. */
void LogInfo(const std::string& message);

/** @brief Writes a namespaced debug message when verbose logging is enabled. */
void LogDebug(const std::string& message);

/** @brief Writes a namespaced warning message to the RE_KENSHI debug log. */
void LogWarning(const std::string& message);

/** @brief Writes a namespaced error message to the RE_KENSHI error log. */
void LogError(const std::string& message);

/** @brief Describes a GameData record for diagnostics without relying on string IDs for behavior. */
std::string DescribeGameData(GameData* data);

/** @brief Describes a Character pointer and its GameData for temporary identity diagnostics. */
std::string DescribeCharacter(Character* character);
