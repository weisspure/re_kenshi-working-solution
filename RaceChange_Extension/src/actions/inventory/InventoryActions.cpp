#include "InventoryActions.h"

#include "../../Logging.h"

#include <kenshi/Character.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>

#include <cstdlib>

namespace
{

std::string DescribeItemState(Item *item, Inventory *inventory)
{
	if (item == 0)
		return "item=null";

	return "item={" + DescribeGameData(item->data) + "}" +
		   " | ptr=" + PointerToString(item) +
		   " | section=\"" + item->inventorySection + "\"" +
		   " | equipped=" + std::string(item->isEquipped ? "true" : "false") +
		   " | inInventory=" + std::string(item->isInInventory ? "true" : "false") +
		   " | inventoryHasItem=" + std::string(inventory != 0 && inventory->hasItem(item) ? "true" : "false");
}

}

std::vector<Item *> RemoveAllInventoryItemsBeforeRaceChange(Character *character)
{
	std::vector<Item *> removedItems;
	if (character == 0 || character->getInventory() == 0)
		return removedItems;

	Inventory *inventory = character->getInventory();

	// getAllItems() only returns items in the flat backpack list (_allItems).
	// Equipped items live exclusively in their InventorySection slot and are not
	// present in _allItems.  Iterate all sections to capture everything.
	std::vector<Item *> snapshot;
	lektor<InventorySection *> &sections = inventory->getAllSections();
	for (int s = 0; s < (int)sections.size(); ++s)
	{
		InventorySection *section = sections[s];
		if (section == 0)
			continue;

		const Ogre::vector<InventorySection::SectionItem>::type &sItems = section->getItems();
		for (size_t i = 0; i < sItems.size(); ++i)
		{
			Item *item = sItems[i].item;
			if (item != 0)
				snapshot.push_back(item);
		}
	}

	LogInfo(
		"removing all inventory items before race change (section scan)"
		" | character={" +
		DescribeCharacter(character) + "}"
									   " | count=" +
		IntToString((int)snapshot.size()));

	for (std::vector<Item *>::const_iterator it = snapshot.begin(); it != snapshot.end(); ++it)
	{
		Item *item = *it;
		if (item == 0)
			continue;

		if (item->isEquipped)
			character->unequipItem(item->inventorySection, item);

		Item *removed = inventory->removeItemDontDestroy_returnsItem(item, item->quantity, false);
		if (removed == 0)
		{
			LogWarning(
				"could not remove inventory item without destroying it; dropping to ground"
				" | " +
				DescribeItemState(item, inventory));
			inventory->dropItem(item);
			continue;
		}

		removedItems.push_back(removed);
	}

	inventory->refreshGui();
	return removedItems;
}

void RestoreRemovedInventoryItemsAfterRaceChange(Character *character, const std::vector<Item *> &removedItems)
{
	if (character == 0 || character->getInventory() == 0 || removedItems.empty())
		return;

	Inventory *inventory = character->getInventory();
	LogInfo(
		"restoring all removed inventory items after race change"
		" | character={" +
		DescribeCharacter(character) + "}"
									   " | count=" +
		IntToString((int)removedItems.size()));

	for (std::vector<Item *>::const_iterator it = removedItems.begin(); it != removedItems.end(); ++it)
	{
		Item *item = *it;
		if (item == 0)
			continue;

		bool added = inventory->addItem(item, item->quantity, true, false);
		if (!added)
		{
			LogWarning(
				"could not restore removed inventory item; dropOnFail=true destroyOnFail=false"
				" | " +
				DescribeItemState(item, inventory));
		}
	}

	inventory->refreshGui();
}

/**
 * Humanoid/playable race changes preserve policy differently from animal replacement:
 * only armour is evacuated before setRace, then returned with destroyOnFail=false after
 * inventory sections have been validated for the new race.
 */
std::vector<Item *> RemoveArmourBeforeRaceChange(Character *character)
{
	std::vector<Item *> removedItems;
	if (character == 0 || character->getInventory() == 0)
		return removedItems;

	lektor<Item *> armour;
	armour.maxSize = 0;
	armour.count = 0;
	armour.stuff = 0;

	character->getInventory()->getEquippedArmour(armour);
	if (armour.size() == 0)
		return removedItems;

	Inventory *inventory = character->getInventory();
	LogInfo("removing equipped armour before race change | character={" + DescribeCharacter(character) + "} | count=" + IntToString((int)armour.size()));

	for (int i = 0; i < (int)armour.size(); ++i)
	{
		Item *item = armour[i];
		if (item == 0)
			continue;

		std::string section = item->inventorySection;
		LogInfo(
			"preparing armour item for race change"
			" | section=\"" +
			section + "\"" +
			" | " + DescribeItemState(item, inventory));

		character->unequipItem(section, item);

		if (item->isEquipped)
			LogWarning(
				"armour item still reports equipped after unequip attempt"
				" | section=\"" +
				section + "\"" +
				" | item={" + DescribeGameData(item->data) + "}");

		Item *removed = inventory->removeItemDontDestroy_returnsItem(item, item->quantity, false);
		if (removed == 0)
		{
			LogWarning(
				"could not remove armour item from inventory without destroying it"
				" | section=\"" +
				section + "\"" +
				" | " + DescribeItemState(item, inventory));
			continue;
		}

		LogInfo(
			"removed armour item without destroying it"
			" | originalSection=\"" +
			section + "\"" +
			" | " + DescribeItemState(removed, inventory));
		removedItems.push_back(removed);
	}

	if (armour.stuff != 0)
		free(armour.stuff);

	inventory->refreshGui();
	return removedItems;
}

void RestoreRemovedArmourAfterRaceChange(Character *character, const std::vector<Item *> &removedItems)
{
	if (character == 0 || character->getInventory() == 0 || removedItems.empty())
		return;

	Inventory *inventory = character->getInventory();
	LogInfo("restoring removed armour after race change | character={" + DescribeCharacter(character) + "} | count=" + IntToString((int)removedItems.size()));

	for (std::vector<Item *>::const_iterator it = removedItems.begin(); it != removedItems.end(); ++it)
	{
		Item *item = *it;
		if (item == 0)
			continue;

		LogInfo("restoring armour item after race change | before={" + DescribeItemState(item, inventory) + "}");

		bool added = inventory->addItem(item, item->quantity, true, false);
		if (!added)
		{
			LogWarning(
				"could not restore armour item to inventory; addItem was called with dropOnFail=true and destroyOnFail=false"
				" | after={" +
				DescribeItemState(item, inventory) + "}");
			continue;
		}

		LogInfo("restored armour item after race change | after={" + DescribeItemState(item, inventory) + "}");
	}

	inventory->refreshGui();
}

/**
 * Animal replacement drops evacuated items instead of restoring them because the source
 * character is about to be destroyed and humanoid equipment slots should not be carried
 * onto the spawned animal object.
 */
void DropEvacuatedInventoryItems(Character *character, const std::vector<Item *> &items)
{
	if (character == 0 || character->getInventory() == 0 || items.empty())
		return;

	Inventory *inventory = character->getInventory();
	LogInfo(
		"dropping all evacuated items to ground after animal race change"
		" | character={" +
		DescribeCharacter(character) + "}"
									   " | count=" +
		IntToString((int)items.size()));

	for (std::vector<Item *>::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		Item *item = *it;
		if (item == 0)
			continue;
		inventory->dropItem(item);
	}

	inventory->refreshGui();
}
