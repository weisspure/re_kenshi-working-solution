#pragma once

class Character;
class GameData;
class RootObject;

/** Return the current runtime race GameData for a character, or null when character/race data is missing. */
GameData* GetCharacterRaceGameData(Character* character);

/** Find the first ANIMAL_CHARACTER template referencing targetRace. Returns null for missing runtime data or no match. */
GameData* FindAnimalTemplateForRace(GameData* targetRace);

/** Spawn an animal from a template at the source character's position/faction/platoon. Returns null on runtime failure. */
RootObject* SpawnAnimalFromTemplate(Character* character, GameData* animalTemplate);

/** Transfer only the verified supported source state: name, stats, and common runtime fields. */
void TransferSupportedStateToSpawnedAnimal(Character* source, Character* dest);

/** Destroy the source character after replacement succeeds, logging null runtime failures. */
void DestroySourceAfterAnimalReplacement(Character* source, Character* spawnedCharacter);
