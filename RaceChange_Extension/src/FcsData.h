#pragma once

#include <string>

#include <kenshi/GameData.h>

/** Find the mutable FCS reference list for a field key, or a null pointer when absent. */
Ogre::vector<GameDataReference>::type* FindReferences(GameData* data, const std::string& key);

/** Read a string field from GameData, returning an empty string for null or missing keys. */
std::string GetFcsStringField(GameData* data, const std::string& key);

/** Describe all object reference lists on a record for action-scan diagnostics. */
std::string DescribeFcsObjectReferenceKeys(GameData* data);

/** Describe the first reference for a field key, including values and resolved pointer. */
std::string DescribeFirstFcsReference(GameData* data, const std::string& key);
