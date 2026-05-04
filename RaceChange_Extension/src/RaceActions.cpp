#include "RaceActions.h"

#include "ActionCore.h"
#include "actions/animal/AnimalRaceActions.h"
#include "actions/appearance/AppearanceActions.h"
#include "actions/inventory/InventoryActions.h"
#include "FcsData.h"
#include "Logging.h"
#include "Targets.h"

#include <kenshi/Character.h>
#include <kenshi/Dialogue.h>
#include <kenshi/Enums.h>
#include <kenshi/GameData.h>
#include <kenshi/Globals.h>
#include <kenshi/RaceData.h>
#include <kenshi/RootObject.h>

#include <vector>

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

	GameData *beforeRace = GetCharacterRaceGameData(character);
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

			TransferSupportedStateToSpawnedAnimal(character, spawnedCharacter);
			ResetAppearanceDataForRace(spawnedCharacter, targetRace);
			RefreshRaceDerivedInventory(spawnedCharacter);
			OpenCharacterEditor(spawnedCharacter);

			DestroySourceAfterAnimalReplacement(character, spawnedCharacter);

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

	GameData *afterRace = GetCharacterRaceGameData(character);
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
