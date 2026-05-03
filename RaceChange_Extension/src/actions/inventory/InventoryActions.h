#pragma once

#include <string>
#include <vector>

class Character;
class Item;

/** Remove every item found in inventory sections before animal replacement or full-inventory pivot paths. */
std::vector<Item*> RemoveAllInventoryItemsBeforeRaceChange(Character* character);

/** Restore previously removed inventory items with dropOnFail=true and destroyOnFail=false. */
void RestoreRemovedInventoryItemsAfterRaceChange(Character* character, const std::vector<Item*>& removedItems);

/** Remove equipped armour before humanoid in-place race mutation, preserving item ownership. */
std::vector<Item*> RemoveArmourBeforeRaceChange(Character* character);

/** Restore armour removed by RemoveArmourBeforeRaceChange after inventory sections are validated. */
void RestoreRemovedArmourAfterRaceChange(Character* character, const std::vector<Item*>& removedItems);

/** Drop evacuated items to the ground for animal replacement or animal fallback policy. */
void DropEvacuatedInventoryItems(Character* character, const std::vector<Item*>& items);
