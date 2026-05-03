#include "RaceActions.h"

#include "ActionCore.h"
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
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RaceData.h>

#include <cstdlib>
#include <vector>

static const bool ENABLE_ACTION_SCAN_LOGS = true;

static GameData* GetRaceGameData(Character* character)
{
	if (character == 0)
		return 0;

	RaceData* race = character->getRace();
	if (race == 0)
		return 0;

	return race->data;
}

static std::string GetStringField(GameData* data, const std::string& key)
{
	if (data == 0)
		return "";

	auto it = data->sdata.find(key);
	if (it == data->sdata.end())
		return "";

	return it->second;
}

static std::string DescribeObjectReferenceKeys(GameData* lineData)
{
	if (lineData == 0)
		return "lineData=null";

	if (lineData->objectReferences.empty())
		return "none";

	std::string result;
	for (auto it = lineData->objectReferences.begin(); it != lineData->objectReferences.end(); ++it)
	{
		if (!result.empty())
			result += "; ";

		result += it->first + "[" + IntToString((int)it->second.size()) + "]";

		if (!it->second.empty())
		{
			const GameDataReference& first = it->second[0];
			result += " firstSid=\"" + first.sid + "\"";
			result += " firstVal0=" + IntToString(first.values[0]);
			result += " firstPtr={" + DescribeGameData(first.ptr) + "}";
		}
	}

	return result;
}

static std::string DescribeFirstReference(GameData* data, const std::string& key)
{
	if (data == 0)
		return "data=null";

	auto it = data->objectReferences.find(key);
	if (it == data->objectReferences.end())
		return "missing";

	if (it->second.empty())
		return "empty";

	const GameDataReference& first = it->second[0];
	return
		"sid=\"" + first.sid + "\"" +
		" | values=(" + IntToString(first.values[0]) + "," + IntToString(first.values[1]) + "," + IntToString(first.values[2]) + ")" +
		" | ptr={" + DescribeGameData(first.ptr) + "}";
}

static void SetSingleRaceReference(GameData* appearance, GameData* targetRace)
{
	if (appearance == 0 || targetRace == 0)
		return;

	appearance->clearList("race");
	appearance->addToList("race", targetRace->stringID, 0, 0, 0);

	GameDataReference* ref = appearance->getGameDataReferenceObject("race", targetRace->stringID);
	if (ref != 0)
		ref->ptr = targetRace;
}

static void LogActionScan(DialogLineData* dialogLine, GameData* lineData, bool hasRaceChangeAction)
{
	if (!ENABLE_ACTION_SCAN_LOGS)
		return;

	LogInfo(
		"action scan"
		" | hasRaceChangeAction=" + std::string(hasRaceChangeAction ? "true" : "false") +
		" | line={" + DescribeGameData(lineData) + "}" +
		" | lineName=\"" + (dialogLine != 0 ? dialogLine->getName() : "null") + "\"" +
		" | text0=\"" + GetStringField(lineData, "text0") + "\"" +
		" | speaker=" + IntToString(dialogLine != 0 ? (int)dialogLine->speaker : -1) +
		" | objectReferences=" + DescribeObjectReferenceKeys(lineData));
}

static void LogRaceDiagnostics(GameData* targetRace)
{
	RaceData* raceData = RaceData::getRaceData(targetRace);
	if (raceData == 0)
	{
		LogWarning("RaceData::getRaceData returned null | targetRace={" + DescribeGameData(targetRace) + "}");
		return;
	}

	if (raceData->raceGroup == 0)
		LogWarning("target race has no runtime raceGroup; it may be absent from the vanilla editor race list | targetRace={" + DescribeGameData(targetRace) + "}");
}

static GameData* FindAnimalTemplateForRace(GameData* targetRace)
{
	if (targetRace == 0 || ou == 0)
		return 0;

	lektor<GameData*> matches;
	ou->gamedata.findAllDataThatReferencesThis(matches, targetRace, ANIMAL_CHARACTER, "race");

	if (matches.size() == 0)
	{
		if (matches.stuff != 0)
			free(matches.stuff);
		return 0;
	}

	GameData* animalTemplate = matches[0];
	if (matches.size() > 1)
	{
		LogWarning(
			"multiple animal templates reference target race; using first match"
			" | count=" + IntToString((int)matches.size()) +
			" | targetRace={" + DescribeGameData(targetRace) + "}" +
			" | firstAnimalTemplate={" + DescribeGameData(animalTemplate) + "}");
	}

	if (matches.stuff != 0)
		free(matches.stuff);

	return animalTemplate;
}

static void OpenCharacterEditor(Character* character)
{
	if (ou == 0 || ou->player == 0)
	{
		LogError("cannot open character editor because ou/player is null");
		return;
	}

	LogInfo("opening character editor through PlayerInterface::activateCharacterEditMode | character={" + DescribeCharacter(character) + "}");
	ou->player->activateCharacterEditMode(character);
}

static std::string DescribeItemState(Item* item, Inventory* inventory)
{
	if (item == 0)
		return "item=null";

	return
		"item={" + DescribeGameData(item->data) + "}" +
		" | ptr=" + PointerToString(item) +
		" | section=\"" + item->inventorySection + "\"" +
		" | equipped=" + std::string(item->isEquipped ? "true" : "false") +
		" | inInventory=" + std::string(item->isInInventory ? "true" : "false") +
		" | inventoryHasItem=" + std::string(inventory != 0 && inventory->hasItem(item) ? "true" : "false");
}

static std::vector<Item*> RemoveArmourBeforeRaceChange(Character* character)
{
	std::vector<Item*> removedItems;
	if (character == 0 || character->getInventory() == 0)
		return removedItems;

	lektor<Item*> armour;
	armour.maxSize = 0;
	armour.count = 0;
	armour.stuff = 0;

	character->getInventory()->getEquippedArmour(armour);
	if (armour.size() == 0)
		return removedItems;

	Inventory* inventory = character->getInventory();
	LogInfo("removing equipped armour before race change | character={" + DescribeCharacter(character) + "} | count=" + IntToString((int)armour.size()));

	for (int i = 0; i < (int)armour.size(); ++i)
	{
		Item* item = armour[i];
		if (item == 0)
			continue;

		std::string section = item->inventorySection;
		LogInfo(
			"preparing armour item for race change"
			" | section=\"" + section + "\"" +
			" | " + DescribeItemState(item, inventory));

		character->unequipItem(section, item);

		if (item->isEquipped)
			LogWarning(
				"armour item still reports equipped after unequip attempt"
				" | section=\"" + section + "\"" +
				" | item={" + DescribeGameData(item->data) + "}");

		Item* removed = inventory->removeItemDontDestroy_returnsItem(item, item->quantity, false);
		if (removed == 0)
		{
			LogWarning(
				"could not remove armour item from inventory without destroying it"
				" | section=\"" + section + "\"" +
				" | " + DescribeItemState(item, inventory));
			continue;
		}

		LogInfo(
			"removed armour item without destroying it"
			" | originalSection=\"" + section + "\"" +
			" | " + DescribeItemState(removed, inventory));
		removedItems.push_back(removed);
	}

	if (armour.stuff != 0)
		free(armour.stuff);

	inventory->refreshGui();
	return removedItems;
}

static void RestoreRemovedArmourAfterRaceChange(Character* character, const std::vector<Item*>& removedItems)
{
	if (character == 0 || character->getInventory() == 0 || removedItems.empty())
		return;

	Inventory* inventory = character->getInventory();
	LogInfo("restoring removed armour after race change | character={" + DescribeCharacter(character) + "} | count=" + IntToString((int)removedItems.size()));

	for (std::vector<Item*>::const_iterator it = removedItems.begin(); it != removedItems.end(); ++it)
	{
		Item* item = *it;
		if (item == 0)
			continue;

		LogInfo("restoring armour item after race change | before={" + DescribeItemState(item, inventory) + "}");

		bool added = inventory->addItem(item, item->quantity, true, false);
		if (!added)
		{
			LogWarning(
				"could not restore armour item to inventory; addItem was called with dropOnFail=true and destroyOnFail=false"
				" | after={" + DescribeItemState(item, inventory) + "}");
			continue;
		}

		LogInfo("restored armour item after race change | after={" + DescribeItemState(item, inventory) + "}");
	}

	inventory->refreshGui();
}

static bool ResetAppearanceDataForRace(Character* character, GameData* targetRace)
{
	if (character == 0 || targetRace == 0)
		return false;

	AppearanceManager* appearanceManager = AppearanceManager::getInstance();
	if (appearanceManager == 0)
	{
		LogError("AppearanceManager::getInstance returned null; cannot reset appearance data");
		return false;
	}

	GameDataCopyStandalone* beforeAppearance = character->getAppearanceData();
	LogInfo(
		"resetting appearance data"
		" | character={" + DescribeCharacter(character) + "}" +
		" | beforeAppearance={" + DescribeGameData(beforeAppearance) + "}" +
		" | beforeAppearanceRace={" + DescribeFirstReference(beforeAppearance, "race") + "}" +
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	GameDataCopyStandalone* appearance = appearanceManager->createAppearanceData(targetRace);
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
		" | appearance={" + DescribeGameData(appearance) + "}" +
		" | appearanceRace={" + DescribeFirstReference(appearance, "race") + "}" +
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	character->setAppearanceData(appearance);

	LogInfo(
		"reset appearance data"
		" | character={" + DescribeCharacter(character) + "}" +
		" | afterAppearance={" + DescribeGameData(character->getAppearanceData()) + "}" +
		" | afterAppearanceRace={" + DescribeFirstReference(character->getAppearanceData(), "race") + "}" +
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	return true;
}

static void RefreshRaceDerivedInventory(Character* character)
{
	if (character == 0)
		return;

	LogInfo("validating inventory sections after race change | character={" + DescribeCharacter(character) + "}");
	character->validateInventorySections();
}

static void ApplyRaceChangeRef(Dialogue* dlg, DialogLineData* dialogLine, const GameDataReference& ref, const std::string& actionKey)
{
	GameData* targetRace = ref.ptr;
	RaceChangeIntent intent = GetRaceChangeIntent(ref.values[0]);
	RaceChangeTargetRole role = GetRaceChangeActionRole(actionKey);
	if (role == RACE_CHANGE_ROLE_UNKNOWN)
	{
		LogError("unknown race change action key reached dispatcher | action=" + actionKey);
		return;
	}

	Character* character = ResolveRaceChangeTarget(dlg, dialogLine, role);

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
			" | action=" + actionKey +
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
			" | action=" + actionKey +
			" | val0=" + IntToString(ref.values[0]) +
			" | character={" + DescribeCharacter(character) + "}" +
			" | targetRace={" + DescribeGameData(targetRace) + "}");
		return;
	}

	LogRaceDiagnostics(targetRace);

	if (intent == RACE_CHANGE_INTENT_ANIMAL)
	{
		GameData* animalTemplate = FindAnimalTemplateForRace(targetRace);
		if (animalTemplate == 0)
		{
			LogWarning(
				"animal transform intent could not find an ANIMAL_CHARACTER template referencing target race; refusing"
				" | action=" + actionKey +
				" | character={" + DescribeCharacter(character) + "}" +
				" | targetRace={" + DescribeGameData(targetRace) + "}");
			return;
		}

		LogWarning(
			"animal transform intent found ANIMAL_CHARACTER template but spawn-and-migrate is not implemented; refusing"
			" | action=" + actionKey +
			" | character={" + DescribeCharacter(character) + "}" +
			" | targetRace={" + DescribeGameData(targetRace) + "}" +
			" | animalTemplate={" + DescribeGameData(animalTemplate) + "}");
		return;
	}

	GameData* beforeRace = GetRaceGameData(character);
	LogInfo(
		"changing race"
		" | action=" + actionKey +
		" | role=" + RaceChangeRoleToString(role) +
		" | intent=" + RaceChangeIntentToString(intent) +
		" | character={" + DescribeCharacter(character) + "}" +
		" | beforeRace={" + DescribeGameData(beforeRace) + "}" +
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	std::vector<Item*> removedArmour = RemoveArmourBeforeRaceChange(character);
	character->setRace(targetRace);

	GameData* afterRace = GetRaceGameData(character);
	LogInfo(
		"changed race"
		" | action=" + actionKey +
		" | character={" + DescribeCharacter(character) + "}" +
		" | afterRace={" + DescribeGameData(afterRace) + "}");

	ResetAppearanceDataForRace(character, targetRace);
	RefreshRaceDerivedInventory(character);
	RestoreRemovedArmourAfterRaceChange(character, removedArmour);

	OpenCharacterEditor(character);
}

static void TryApplyRaceChangeAction(Dialogue* dlg, DialogLineData* dialogLine, GameData* lineData, const std::string& actionKey)
{
	Ogre::vector<GameDataReference>::type* refs = FindReferences(lineData, actionKey);
	if (refs == 0)
		return;

	for (auto it = refs->begin(); it != refs->end(); ++it)
		ApplyRaceChangeRef(dlg, dialogLine, *it, actionKey);
}

static bool HasAnyRaceChangeAction(GameData* lineData)
{
	return FindReferences(lineData, ACTION_CHANGE_RACE) != 0 || FindReferences(lineData, ACTION_CHANGE_OTHER_RACE) != 0;
}

void DispatchRaceChangeActions(Dialogue* dlg, DialogLineData* dialogLine)
{
	if (dlg == 0 || dialogLine == 0)
		return;

	GameData* lineData = dialogLine->getGameData();
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
