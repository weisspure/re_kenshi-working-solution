#include "FcsData.h"

Ogre::vector<GameDataReference>::type* FindReferences(GameData* data, const std::string& key)
{
	if (data == 0)
		return 0;

	auto it = data->objectReferences.find(key);
	if (it == data->objectReferences.end() || it->second.empty())
		return 0;

	return &it->second;
}

