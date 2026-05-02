#include "Logging.h"

#include <Debug.h>

#include <kenshi/Character.h>
#include <kenshi/GameData.h>

#include <sstream>

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
	DebugLog("RaceChange: " + message);
}

void LogWarning(const std::string& message)
{
	DebugLog("RaceChange WARNING: " + message);
}

void LogError(const std::string& message)
{
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

