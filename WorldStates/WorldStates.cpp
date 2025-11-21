
#include <kenshi/WorldEventStateQuery.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Dialogue.h>
#include <kenshi/Faction.h>
#include <core/Functions.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <Debug.h>

enum itemTypeExtended
{
	VARIABLE = 1000
};

// WorldEventStateQuery objects don't store a ref to their gamedata so we need this to get it in WorldEventStateQuery::isTrue
// I'm currently not doing garbage collection on this but that should just cause a small memory leak when reloading, not enough to care about
boost::unordered_map<WorldEventStateQuery*, GameData*> queryMap;
// probably unnecessary  but I don't want to take the risk
boost::mutex mapLock;

WorldEventStateQuery* (*getFromData_orig)(GameData* d);
WorldEventStateQuery* getFromData_hook(GameData* d)
{
	WorldEventStateQuery* query = getFromData_orig(d);

	// lock + add to map
	boost::lock_guard<boost::mutex> lock(mapLock);
	queryMap.emplace(query, d);

	return query;
}

// hook for dialogue checks
// I don't think this is the ideal function to hook, but it has access to the GameData and seems to do the job
bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target);
bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target)
{
	bool returnVal = checkTags_orig(thisptr, me, target);

	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = thisptr->getGameData()->objectReferences.find("variable equals");
	if (iter != thisptr->getGameData()->objectReferences.end())
	{
		for (Ogre::vector<GameDataReference>::type::iterator variableIter = iter->second.begin(); variableIter != iter->second.end(); ++variableIter)
		{
			ogre_unordered_map<std::string, int>::type::iterator valueIter = variableIter->ptr->idata.find("value");
			if (valueIter != variableIter->ptr->idata.end())
			{
				// apply condition
				returnVal = returnVal && valueIter->second == variableIter->values[0];
			}
		}
	}

	iter = thisptr->getGameData()->objectReferences.find("variable less than");
	if (iter != thisptr->getGameData()->objectReferences.end())
	{
		for (Ogre::vector<GameDataReference>::type::iterator variableIter = iter->second.begin(); variableIter != iter->second.end(); ++variableIter)
		{
			ogre_unordered_map<std::string, int>::type::iterator valueIter = variableIter->ptr->idata.find("value");
			if (valueIter != variableIter->ptr->idata.end())
			{
				// apply condition
				returnVal = returnVal && valueIter->second < variableIter->values[0];
			}
		}
	}

	iter = thisptr->getGameData()->objectReferences.find("variable greater than");
	if (iter != thisptr->getGameData()->objectReferences.end())
	{
		for (Ogre::vector<GameDataReference>::type::iterator variableIter = iter->second.begin(); variableIter != iter->second.end(); ++variableIter)
		{
			ogre_unordered_map<std::string, int>::type::iterator valueIter = variableIter->ptr->idata.find("value");
			if (valueIter != variableIter->ptr->idata.end())
			{
				// apply condition
				returnVal = returnVal && valueIter->second > variableIter->values[0] ;
			}
		}
	}

	return returnVal;
}

bool (*isTrue_orig)(WorldEventStateQuery* thisptr);
bool isTrue_hook(WorldEventStateQuery* thisptr)
{
	// regular conditions
	bool state = isTrue_orig(thisptr);

	GameData* gameData;
	// prevent race conditions
	{
		boost::lock_guard<boost::mutex> lock(mapLock);
		gameData = queryMap[thisptr];
	}

	// our new conditions
	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = gameData->objectReferences.find("variable equals");
	if(iter != gameData->objectReferences.end())
	{
		for (Ogre::vector<GameDataReference>::type::iterator variableIter = iter->second.begin(); variableIter != iter->second.end(); ++variableIter)
		{
			int targetVal = variableIter->values[0];
			ogre_unordered_map<std::string, int>::type::iterator valueIter = variableIter->ptr->idata.find("value");

			if (valueIter != variableIter->ptr->idata.end())
			{
				state = state && (targetVal == valueIter->second);
			}
			else
			{
				ErrorLog("WorldStates: Variable is missing value");
			}
		}
	}

	iter = gameData->objectReferences.find("variable less than");
	if (iter != gameData->objectReferences.end())
	{
		for (Ogre::vector<GameDataReference>::type::iterator variableIter = iter->second.begin(); variableIter != iter->second.end(); ++variableIter)
		{
			int targetVal = variableIter->values[0];
			ogre_unordered_map<std::string, int>::type::iterator valueIter = variableIter->ptr->idata.find("value");

			if (valueIter != variableIter->ptr->idata.end())
			{
				state = state && (targetVal < valueIter->second);
			}
			else
			{
				ErrorLog("WorldStates: Variable is missing value");
			}
		}
	}

	iter = gameData->objectReferences.find("variable greater than");
	if (iter != gameData->objectReferences.end())
	{
		for (Ogre::vector<GameDataReference>::type::iterator variableIter = iter->second.begin(); variableIter != iter->second.end(); ++variableIter)
		{
			int targetVal = variableIter->values[0];
			ogre_unordered_map<std::string, int>::type::iterator valueIter = variableIter->ptr->idata.find("value");

			if (valueIter != variableIter->ptr->idata.end())
			{
				state = state && (targetVal > valueIter->second);
			}
			else
			{
				ErrorLog("WorldStates: Variable is missing value");
			}
		}
	}

	return state;
}

void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine);
void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = dialogLine->getGameData()->objectReferences.find("set variable");
	if (iter != dialogLine->getGameData()->objectReferences.end())
	{
		if (iter->second.size() == 0)
		{
			ErrorLog("Missing references for \"set variable\"");
		}

		for (Ogre::vector<GameDataReference>::type::iterator variableIter = iter->second.begin(); variableIter != iter->second.end(); ++variableIter)
		{
			ogre_unordered_map<std::string, int>::type::iterator valueIter = variableIter->ptr->idata.find("value");

			if (valueIter != variableIter->ptr->idata.end())
			{
				valueIter->second = variableIter->values[0];
			}
			else
			{
				ErrorLog("Missing parameter: value");
			}
		}
	}

	iter = dialogLine->getGameData()->objectReferences.find("add to variable");
	if (iter != dialogLine->getGameData()->objectReferences.end())
	{
		if (iter->second.size() == 0)
		{
			ErrorLog("Missing references for \"add to variable\"");
		}

		for (Ogre::vector<GameDataReference>::type::iterator variableIter = iter->second.begin(); variableIter != iter->second.end(); ++variableIter)
		{
			ogre_unordered_map<std::string, int>::type::iterator valueIter = variableIter->ptr->idata.find("value");

			if (valueIter != variableIter->ptr->idata.end())
			{
				valueIter->second += variableIter->values[0];
			}
			else
			{
				ErrorLog("Missing parameter: value");
			}
		}
	}
	_doActions_orig(thisptr, dialogLine);
}

// this is a convenient place to hook into the save system
// not 100% sure this is the best way to save data - data here is written to "quick.save"
void (*saveGameState_orig)(FactionManager* thisptr, GameDataContainer* container);
void saveGameState_hook(FactionManager* thisptr, GameDataContainer* container)
{
	// find VARIABLE gamedata
	ogre_unordered_map<std::string, GameData*>::type::iterator iter = ou->gamedata.gamedataSID.begin();
	for (; iter != ou->gamedata.gamedataSID.end(); ++iter)
	{
		if (iter->second->type == (itemType)VARIABLE)
		{
			// create new GameData in save
			GameData* newGameData2 = container->createNewData(iter->second->type, iter->second->stringID, iter->second->name);
			if (newGameData2)
				newGameData2->updateFrom(iter->second, false);
		}
	}
	saveGameState_orig(thisptr, container);
}


// save data is loaded into ou->savedata but the dialogue/world state reference is to the object in ou->gamedata
// so we copy the value over on load
// this probably isn't the ideal function to hook but as far as I can tell it's only called when loading a save game
void (*loadAllPlatoons_orig)(GameWorld* thisptr);
void loadAllPlatoons_hook(GameWorld* thisptr)
{
	loadAllPlatoons_orig(thisptr);

	ogre_unordered_map<std::string, GameData*>::type::iterator iter = ou->savedata.gamedataSID.begin();
	for (; iter != ou->savedata.gamedataSID.end(); ++iter)
	{
		if (iter->second->type == (itemType)VARIABLE)
		{
			ou->gamedata.updateData(iter->second, false);
		}
	}
}

__declspec(dllexport) void startPlugin()
{
	// there's no obvious way to track which GameData is associated with which WorldEventStateQuery/List so we make our own
	// NOTE: there's no garage collection...
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&WorldEventStateQuery::getFromData), &getFromData_hook, &getFromData_orig))
		DebugLog("WorldStates: Could not hook function!");
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&WorldEventStateQuery::isTrue), &isTrue_hook, &isTrue_orig))
		DebugLog("WorldStates: Could not hook function!");
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&DialogLineData::checkTags), &checkTags_hook, &checkTags_orig))
		DebugLog("WorldStates: Could not hook function!");
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Dialogue::_doActions), &_doActions_hook, &_doActions_orig))
		DebugLog("WorldStates: Could not hook function!");
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&FactionManager::saveGameState), &saveGameState_hook, &saveGameState_orig))
		DebugLog("WorldStates: Could not hook function!");
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::loadAllPlatoons), &loadAllPlatoons_hook, &loadAllPlatoons_orig))
		DebugLog("WorldStates: Could not hook function!");
}