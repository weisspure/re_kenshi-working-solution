#pragma once

#include <string>

#include <kenshi/GameData.h>

Ogre::vector<GameDataReference>::type* FindReferences(GameData* data, const std::string& key);

