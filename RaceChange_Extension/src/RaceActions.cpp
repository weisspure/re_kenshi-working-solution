#include "RaceActions.h"

#include "ActionCore.h"
#include "actions/inventory/InventoryActions.h"
#include "FcsData.h"
#include "Logging.h"
#include "Targets.h"

#include <kenshi/Character.h>
#include <kenshi/Dialogue.h>
#include <kenshi/Enums.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/AppearanceManager.h>
#include <kenshi/CharStats.h>
#include <kenshi/Platoon.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RaceData.h>
#include <kenshi/RootObject.h>
#include <kenshi/RootObjectFactory.h>

#include <cstdlib>
#include <vector>

static GameData *GetRaceGameData(Character *character)
{
	if (character == 0)
		return 0;

	RaceData *race = character->getRace();
	if (race == 0)
		return 0;

	return race->data;
}

static void SetSingleRaceReference(GameData *appearance, GameData *targetRace)
{
	if (appearance == 0 || targetRace == 0)
		return;

	appearance->clearList("race");
	appearance->addToList("race", targetRace->stringID, 0, 0, 0);

	GameDataReference *ref = appearance->getGameDataReferenceObject("race", targetRace->stringID);
	if (ref != 0)
		ref->ptr = targetRace;
}

static void LogActionScan(DialogLineData *dialogLine, GameData *lineData, bool hasRaceChangeAction)
{
	// Dialogue lines can carry many object reference lists unrelated to RaceChange.
	// Keep this behind trace logging so support builds can inspect FCS shape without
	// spamming normal players on every dialogue action.
	LogTrace(
		"action scan"
		" | hasRaceChangeAction=" +
		std::string(hasRaceChangeAction ? "true" : "false") +
		" | line={" + DescribeGameData(lineData) + "}" +
		" | lineName=\"" + (dialogLine != 0 ? dialogLine->getName() : "null") + "\"" +
		" | text0=\"" + GetFcsStringField(lineData, "text0") + "\"" +
		" | speaker=" + IntToString(dialogLine != 0 ? (int)dialogLine->speaker : -1) +
		" | objectReferences=" + DescribeFcsObjectReferenceKeys(lineData));
}

static void LogRaceDiagnostics(GameData *targetRace)
{
	RaceData *raceData = RaceData::getRaceData(targetRace);
	if (raceData == 0)
	{
		LogWarning("RaceData::getRaceData returned null | targetRace={" + DescribeGameData(targetRace) + "}");
		return;
	}

	if (raceData->raceGroup == 0)
		LogWarning("target race has no runtime raceGroup; it may be absent from the vanilla editor race list | targetRace={" + DescribeGameData(targetRace) + "}");
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

static RootObject *SpawnAnimalFromTemplate(Character *character, GameData *animalTemplate)
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
static GameData *FindAnimalTemplateForRace(GameData *targetRace)
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

static void RefreshPlayerSelectionForCharacter(Character *character)
{
	if (character == 0 || ou == 0 || ou->player == 0)
		return;

	RootObject *obj = static_cast<RootObject *>(character);
	if (!ou->player->isObjectSelected(obj))
		return;

	LogInfo("refreshing player selection for changed character | character={" + DescribeCharacter(character) + "}");
	ou->player->unselectPlayerCharacter(obj);
	ou->player->selectObject(obj, false);
	ou->player->activateSelection(obj);
}

static void OpenCharacterEditor(Character *character)
{
	if (ou == 0 || ou->player == 0)
	{
		LogError("cannot open character editor because ou/player is null");
		return;
	}

	if (ou->player->getCharacterEditMode())
	{
		LogInfo("character editor already active; closing before reopen");
		ou->player->setCharacterEditMode(false);
	}

	RefreshPlayerSelectionForCharacter(character);

	LogInfo("opening character editor through PlayerInterface::activateCharacterEditMode | character={" + DescribeCharacter(character) + "}");
	ou->player->activateCharacterEditMode(character);
}

static bool ResetAppearanceDataForRace(Character *character, GameData *targetRace)
{
	if (character == 0 || targetRace == 0)
		return false;

	AppearanceManager *appearanceManager = AppearanceManager::getInstance();
	if (appearanceManager == 0)
	{
		LogError("AppearanceManager::getInstance returned null; cannot reset appearance data");
		return false;
	}

	GameDataCopyStandalone *beforeAppearance = character->getAppearanceData();
	LogInfo(
		"resetting appearance data"
		" | character={" +
		DescribeCharacter(character) + "}" +
		" | beforeAppearance={" + DescribeGameData(beforeAppearance) + "}" +
		" | beforeAppearanceRace={" + DescribeFirstFcsReference(beforeAppearance, "race") + "}" +
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	GameDataCopyStandalone *appearance = appearanceManager->createAppearanceData(targetRace);
	if (appearance == 0)
	{
		LogError("AppearanceManager::createAppearanceData returned null | targetRace={" + DescribeGameData(targetRace) + "}");
		return false;
	}

	SetSingleRaceReference(appearance, targetRace);
	appearanceManager->resetAll(appearance, true);
	appearanceManager->cleanValidateAppearanceData(appearance);
	SetSingleRaceReference(appearance, targetRace);

	LogInfo(
		"prepared replacement appearance data"
		" | appearance={" +
		DescribeGameData(appearance) + "}" +
		" | appearanceRace={" + DescribeFirstFcsReference(appearance, "race") + "}" +
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	character->setAppearanceData(appearance);

	LogInfo(
		"reset appearance data"
		" | character={" +
		DescribeCharacter(character) + "}" +
		" | afterAppearance={" + DescribeGameData(character->getAppearanceData()) + "}" +
		" | afterAppearanceRace={" + DescribeFirstFcsReference(character->getAppearanceData(), "race") + "}" +
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	return true;
}

static void RefreshRaceDerivedInventory(Character *character)
{
	if (character == 0)
		return;

	// This is not just UI polish. Runtime testing showed stale race-derived slots until
	// Kenshi rebuilt inventory sections, especially around equipment and editor refresh.
	LogInfo("validating inventory sections after race change | character={" + DescribeCharacter(character) + "}");
	character->validateInventorySections();
}

static void ApplyRaceChangeRef(Dialogue *dlg, DialogLineData *dialogLine, const GameDataReference &ref, const std::string &actionKey)
{
	GameData *targetRace = ref.ptr;
	// `value[0]` is part of the public FCS authoring contract now:
	// 0 means humanoid/default in-place mutation, 1 requests animal spawn-and-replace.
	// Do not reinterpret additional values without treating it as a compatibility change.
	RaceChangeIntent intent = GetRaceChangeIntent(ref.values[0]);
	RaceChangeTargetRole role = GetRaceChangeActionRole(actionKey);
	if (role == RACE_CHANGE_ROLE_UNKNOWN)
	{
		LogError("unknown race change action key reached dispatcher | action=" + actionKey);
		return;
	}

	Character *character = ResolveRaceChangeTarget(dlg, dialogLine, role);

	if (character == 0)
	{
		LogError("could not resolve target character | action=" + actionKey + " | role=" + RaceChangeRoleToString(role));
		return;
	}

	if (targetRace == 0)
	{
		LogError("race reference is null | action=" + actionKey + " | character={" + DescribeCharacter(character) + "}");
		return;
	}

	if ((int)targetRace->type != (int)RACE)
	{
		LogError(
			"wrong item type for race change action"
			" | action=" +
			actionKey +
			" | expected=" + IntToString((int)RACE) +
			" | got=" + IntToString((int)targetRace->type) +
			" | targetRace={" + DescribeGameData(targetRace) + "}");
		return;
	}

	if (!CanApplyRaceChangeAction(character != 0, targetRace != 0, (int)targetRace->type == (int)RACE))
		return;

	if (intent == RACE_CHANGE_INTENT_UNSUPPORTED)
	{
		LogError(
			"unsupported race change intent value"
			" | action=" +
			actionKey +
			" | val0=" + IntToString(ref.values[0]) +
			" | character={" + DescribeCharacter(character) + "}" +
			" | targetRace={" + DescribeGameData(targetRace) + "}");
		return;
	}

	LogRaceDiagnostics(targetRace);

	bool useFullInventoryPivot = (intent == RACE_CHANGE_INTENT_ANIMAL);
	if (intent == RACE_CHANGE_INTENT_HUMANOID)
	{
		GameData *inventoryPivotProbe = FindAnimalTemplateForRace(targetRace);
		if (inventoryPivotProbe != 0)
		{
			useFullInventoryPivot = true;
			LogInfo(
				"inventory handling pivot enabled for race change"
				" | action=" +
				actionKey +
				" | reason=targetRaceHasAnimalTemplate"
				" | targetRace={" +
				DescribeGameData(targetRace) + "}" +
				" | pivotTemplate={" + DescribeGameData(inventoryPivotProbe) + "}");
		}
	}

	if (intent == RACE_CHANGE_INTENT_ANIMAL)
	{
		LogInfo(
			"animal transform intent uses spawn-and-replace path"
			" | action=" +
			actionKey +
			" | character={" + DescribeCharacter(character) + "}"
															  " | targetRace={" +
			DescribeGameData(targetRace) + "}");
	}

	GameData *beforeRace = GetRaceGameData(character);
	LogInfo(
		"changing race"
		" | action=" +
		actionKey +
		" | role=" + RaceChangeRoleToString(role) +
		" | intent=" + RaceChangeIntentToString(intent) +
		" | character={" + DescribeCharacter(character) + "}" +
		" | beforeRace={" + DescribeGameData(beforeRace) + "}" +
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	if (intent == RACE_CHANGE_INTENT_ANIMAL)
	{
		// The animal path is intentionally separate. It must create the replacement first,
		// move the supported state onto it, open the editor for that final character, and
		// only then destroy the source.
		GameData *animalTemplate = FindAnimalTemplateForRace(targetRace);
		if (animalTemplate == 0)
		{
			LogWarning(
				"animal intent had no ANIMAL_CHARACTER template; falling back to in-place mutation"
				" | action=" +
				actionKey +
				" | character={" + DescribeCharacter(character) + "}"
																  " | targetRace={" +
				DescribeGameData(targetRace) + "}");
		}
		else
		{
			std::vector<Item *> removedInventoryItems = RemoveAllInventoryItemsBeforeRaceChange(character);
			DropEvacuatedInventoryItems(character, removedInventoryItems);

			RootObject *spawned = SpawnAnimalFromTemplate(character, animalTemplate);
			if (spawned == 0)
			{
				LogError(
					"animal spawn failed; aborting animal transform"
					" | action=" +
					actionKey +
					" | character={" + DescribeCharacter(character) + "}"
																	  " | targetRace={" +
					DescribeGameData(targetRace) + "}"
												   " | animalTemplate={" +
					DescribeGameData(animalTemplate) + "}");
				return;
			}

			Character *spawnedCharacter = static_cast<Character *>(spawned);

			TransferNameToSpawnedAnimal(character, spawnedCharacter);
			TransferStatsToSpawnedAnimal(character, spawnedCharacter);
			TransferCommonRuntimeStateToSpawnedAnimal(character, spawnedCharacter);
			ResetAppearanceDataForRace(spawnedCharacter, targetRace);
			RefreshRaceDerivedInventory(spawnedCharacter);
			OpenCharacterEditor(spawnedCharacter);

			if (ou != 0)
			{
				bool destroyed = ou->destroy(static_cast<RootObject *>(character), false, "RaceChange animal replacement");
				LogInfo(
					"destroyed source character after animal replacement"
					" | success=" +
					std::string(destroyed ? "true" : "false") +
					" | source={" + DescribeCharacter(character) + "}"
																   " | spawned={" +
					DescribeCharacter(spawnedCharacter) + "}");
			}
			else
			{
				LogWarning(
					"could not destroy source character after animal replacement because ou is null"
					" | source={" +
					DescribeCharacter(character) + "}"
												   " | spawned={" +
					DescribeCharacter(spawnedCharacter) + "}");
			}

			return;
		}
	}

	std::vector<Item *> removedArmour;
	std::vector<Item *> removedInventoryItems;
	// The fallback in-place path still matters when animal intent has no template.
	// Preserve the distinction between full inventory evacuation and armour-only
	// evacuation; they are different product policies, not interchangeable cleanup.
	if (useFullInventoryPivot)
		removedInventoryItems = RemoveAllInventoryItemsBeforeRaceChange(character);
	else
		removedArmour = RemoveArmourBeforeRaceChange(character);
	character->setRace(targetRace);

	GameData *afterRace = GetRaceGameData(character);
	LogInfo(
		"changed race"
		" | action=" +
		actionKey +
		" | character={" + DescribeCharacter(character) + "}" +
		" | afterRace={" + DescribeGameData(afterRace) + "}");

	ResetAppearanceDataForRace(character, targetRace);
	RefreshRaceDerivedInventory(character);

	if (intent == RACE_CHANGE_INTENT_ANIMAL)
		DropEvacuatedInventoryItems(character, removedInventoryItems);
	else if (useFullInventoryPivot)
		RestoreRemovedInventoryItemsAfterRaceChange(character, removedInventoryItems);
	else
		RestoreRemovedArmourAfterRaceChange(character, removedArmour);

	OpenCharacterEditor(character);
}

static void TryApplyRaceChangeAction(Dialogue *dlg, DialogLineData *dialogLine, GameData *lineData, const std::string &actionKey)
{
	Ogre::vector<GameDataReference>::type *refs = FindReferences(lineData, actionKey);
	if (refs == 0)
		return;

	for (auto it = refs->begin(); it != refs->end(); ++it)
		ApplyRaceChangeRef(dlg, dialogLine, *it, actionKey);
}

static bool HasAnyRaceChangeAction(GameData *lineData)
{
	return FindReferences(lineData, ACTION_CHANGE_RACE) != 0 || FindReferences(lineData, ACTION_CHANGE_OTHER_RACE) != 0;
}

void DispatchRaceChangeActions(Dialogue *dlg, DialogLineData *dialogLine)
{
	if (dlg == 0 || dialogLine == 0)
		return;

	GameData *lineData = dialogLine->getGameData();
	if (lineData == 0)
		return;

	bool hasRaceChangeAction = HasAnyRaceChangeAction(lineData);
	if (!hasRaceChangeAction)
	{
		if (!lineData->objectReferences.empty())
			LogActionScan(dialogLine, lineData, false);
		return;
	}

	LogActionScan(dialogLine, lineData, true);

	TryApplyRaceChangeAction(dlg, dialogLine, lineData, ACTION_CHANGE_RACE);
	TryApplyRaceChangeAction(dlg, dialogLine, lineData, ACTION_CHANGE_OTHER_RACE);
}
