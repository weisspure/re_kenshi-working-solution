#pragma once

#include <string>

class Character;
class GameData;

std::string IntToString(int value);
std::string PointerToString(const void* ptr);
std::string DescribeGameData(GameData* data);
std::string DescribeCharacter(Character* character);

void LogInfo(const std::string& message);
void LogWarning(const std::string& message);
void LogError(const std::string& message);

