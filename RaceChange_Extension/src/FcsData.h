#pragma once

#include <string>

#include <kenshi/GameData.h>

/** Find the mutable FCS reference list for a field key, or a null pointer when absent. */
Ogre::vector<GameDataReference>::type* FindReferences(GameData* data, const std::string& key);
