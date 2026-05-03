#include "AppearanceActions.h"

#include "../../FcsData.h"
#include "../../Logging.h"

#include <kenshi/AppearanceManager.h>
#include <kenshi/Character.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObject.h>

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

void OpenCharacterEditor(Character *character)
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

bool ResetAppearanceDataForRace(Character *character, GameData *targetRace)
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

void RefreshRaceDerivedInventory(Character *character)
{
	if (character == 0)
		return;

	// This is not just UI polish. Runtime testing showed stale race-derived slots until
	// Kenshi rebuilt inventory sections, especially around equipment and editor refresh.
	LogInfo("validating inventory sections after race change | character={" + DescribeCharacter(character) + "}");
	character->validateInventorySections();
}
