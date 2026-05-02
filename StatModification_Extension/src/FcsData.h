#pragma once

#include <kenshi/Enums.h>
#include <kenshi/GameData.h>

#include <string>

/**
 * @brief Reads an integer field from GameData::idata.
 *
 * Returns fallback when the record is null or the field is absent.
 */
int GetIntField(GameData* data, const std::string& key, int fallback);

/**
 * @brief Returns a non-empty GameData reference list by FCS key.
 *
 * Missing keys and empty lists both return null. Callers decide whether
 * that means no-op, optional unset field, or malformed data.
 */
Ogre::vector<GameDataReference>::type* FindReferences(GameData* data, const std::string& key);

/**
 * @brief Returns the STAT_DEFINITION record referenced by an action record.
 *
 * The action record must have objectReferences["stat"] pointing at a
 * STAT_DEFINITION item. Wrong item types are logged and rejected.
 */
GameData* GetStatDefinitionRecord(GameData* actionRecord, const std::string& context);

/**
 * @brief Reads idata["enum value"] from a STAT_DEFINITION as StatsEnumerated.
 *
 * No runtime whitelist is applied. STAT_NONE is handled by callers as invalid.
 */
StatsEnumerated ReadStatEnum(GameData* statDef);
