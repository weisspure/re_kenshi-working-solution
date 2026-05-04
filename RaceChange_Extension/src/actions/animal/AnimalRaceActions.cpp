#include "AnimalRaceActions.h"

#include "../../Logging.h"

#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/Enums.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Platoon.h>
#include <kenshi/RaceData.h>
#include <kenshi/RootObject.h>
#include <kenshi/RootObjectFactory.h>

#include <cstdlib>
#include <string>

GameData *GetCharacterRaceGameData(Character *character)
{
	if (character == 0)
		return 0;

	RaceData *race = character->getRace();
	if (race == 0)
		return 0;

	return race->data;
}

static void TransferNameToSpawnedAnimal(Character *source, Character *dest)
{
	std::string name = source->getName();
	if (name.empty())
	{
		LogInfo("source character has no name; skipping name transfer");
		return;
	}
	dest->setName(name);
	LogInfo("transferred name to spawned animal | name=\"" + name + "\"");
}

/**
 * Animal replacement intentionally transplants only the modest state we have verified:
 * name, stat values, and small common runtime fields. This is not a full character
 * migration layer; add new transfers only after an in-game check proves the field is
 * safe to read from the source and write to the spawned animal.
 */
static void TransferStatsToSpawnedAnimal(Character *source, Character *dest)
{
	CharStats *srcStats = source->getStats();
	CharStats *dstStats = dest->getStats();
	if (srcStats == 0 || dstStats == 0)
	{
		LogWarning(
			"cannot transfer stats: stats pointer null"
			" | srcStats=" +
			PointerToString(srcStats) +
			" | dstStats=" + PointerToString(dstStats));
		return;
	}

	int transferred = 0;
	for (int i = (int)STAT_STRENGTH; i < (int)STAT_END; ++i)
	{
		StatsEnumerated stat = (StatsEnumerated)i;
		float value = srcStats->getStat(stat, true);
		dstStats->getStatRef(stat) = value;
		++transferred;
	}

	LogInfo("transferred stats to spawned animal | count=" + IntToString(transferred));
}

static void TransferCommonRuntimeStateToSpawnedAnimal(Character *source, Character *dest)
{
	if (source == 0 || dest == 0)
		return;

	float age = source->getAge();
	dest->setAge(age);

	LogInfo("transferred common runtime state to spawned animal | age=" + IntToString((int)age));
}

RootObject *SpawnAnimalFromTemplate(Character *character, GameData *animalTemplate)
{
	if (ou == 0 || ou->theFactory == 0)
	{
		LogError("cannot spawn animal: ou or ou->theFactory is null");
		return 0;
	}

	Ogre::Vector3 position = character->getPosition();
	Faction *faction = character->getFaction();
	ActivePlatoon *platoon = character->getPlatoon();

	LogInfo(
		"spawning animal template for transform"
		" | animalTemplate={" +
		DescribeGameData(animalTemplate) + "}"
										   " | position=(" +
		IntToString((int)position.x) + "," + IntToString((int)position.y) + "," + IntToString((int)position.z) + ")"
																												 " | faction=" +
		PointerToString(faction) +
		" | platoon=" + PointerToString(platoon));

	RootObject *spawned = ou->theFactory->createRandomCharacter(faction, position, platoon, animalTemplate, 0, 1.0f);
	if (spawned == 0)
	{
		LogError("createRandomCharacter returned null | animalTemplate={" + DescribeGameData(animalTemplate) + "}");
		return 0;
	}

	LogInfo("spawned animal | ptr=" + PointerToString(spawned) + " | animalTemplate={" + DescribeGameData(animalTemplate) + "}");
	return spawned;
}

/**
 * Animal intent is authored against a target RACE, but Kenshi needs an ANIMAL_CHARACTER
 * template to create a real animal object. `findAllDataThatReferencesThis` fills a
 * lektor buffer for us, but it does not manage that buffer's lifetime after the call.
 * Always copy out the pointer we need and release `matches.stuff` before returning;
 * otherwise a repeated dialogue action can leak memory every time it scans templates.
 */
GameData *FindAnimalTemplateForRace(GameData *targetRace)
{
	if (targetRace == 0 || ou == 0)
		return 0;

	lektor<GameData *> matches;
	ou->gamedata.findAllDataThatReferencesThis(matches, targetRace, ANIMAL_CHARACTER, "race");

	if (matches.size() == 0)
	{
		if (matches.stuff != 0)
			free(matches.stuff);
		return 0;
	}

	GameData *animalTemplate = matches[0];
	if (matches.size() > 1)
	{
		LogWarning(
			"multiple animal templates reference target race; using first match"
			" | count=" +
			IntToString((int)matches.size()) +
			" | targetRace={" + DescribeGameData(targetRace) + "}" +
			" | firstAnimalTemplate={" + DescribeGameData(animalTemplate) + "}");
	}

	if (matches.stuff != 0)
		free(matches.stuff);

	return animalTemplate;
}

void TransferSupportedStateToSpawnedAnimal(Character *source, Character *dest)
{
	if (source == 0 || dest == 0)
		return;

	TransferNameToSpawnedAnimal(source, dest);
	TransferStatsToSpawnedAnimal(source, dest);
	TransferCommonRuntimeStateToSpawnedAnimal(source, dest);
}

void DestroySourceAfterAnimalReplacement(Character *source, Character *spawnedCharacter)
{
	if (source == 0)
	{
		LogWarning(
			"could not destroy source character after animal replacement because source is null"
			" | spawned={" +
			DescribeCharacter(spawnedCharacter) + "}");
		return;
	}

	if (ou == 0)
	{
		LogWarning(
			"could not destroy source character after animal replacement because ou is null"
			" | source={" +
			DescribeCharacter(source) + "}"
			" | spawned={" +
			DescribeCharacter(spawnedCharacter) + "}");
		return;
	}

	bool destroyed = ou->destroy(static_cast<RootObject *>(source), false, "RaceChange animal replacement");
	LogInfo(
		"destroyed source character after animal replacement"
		" | success=" +
		std::string(destroyed ? "true" : "false") +
		" | source={" + DescribeCharacter(source) + "}"
		" | spawned={" + DescribeCharacter(spawnedCharacter) + "}");
}
