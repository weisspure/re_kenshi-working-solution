#pragma once

class Character;
class GameData;

/** Reset borrowed Kenshi appearance data for the new race. Returns false when runtime services or inputs are missing. */
bool ResetAppearanceDataForRace(Character* character, GameData* targetRace);

/** Rebuild race-derived inventory sections after a live race change. Null character is a no-op. */
void RefreshRaceDerivedInventory(Character* character);

/** Open the vanilla character editor for the final live character after race data and inventory sections are repaired. */
void OpenCharacterEditor(Character* character);
