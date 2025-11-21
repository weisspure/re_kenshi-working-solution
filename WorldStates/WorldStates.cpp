
#include <ogre/OgreMemorySTLAllocator.h>
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

	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator varEquals = thisptr->getGameData()->objectReferences.find("variable equals");
	if (varEquals != thisptr->getGameData()->objectReferences.end())
	{
		//if (thisptr->texts)
		//	DebugLog(thisptr->texts[0]);
		for (Ogre::vector<GameDataReference>::type::iterator iter = varEquals->second.begin(); iter != varEquals->second.end(); ++iter)
		{
			// weird hack - save data doesn't get loaded onto the GameData object referenced here, so we need to load the actual saved value from the save gamedatas
			/*
			ogre_unordered_map<std::string, GameData*>::type::iterator saveDataIter = ou->gamedata.gamedataSID.find(iter->sid);
			if (saveDataIter != ou->gamedata.gamedataSID.end())
			{
				ogre_unordered_map<std::string, int>::type::iterator valueIter = saveDataIter->second->idata.find("value");
				int value = -1;
				if (valueIter != saveDataIter->second->idata.end())
					value = valueIter->second;
			/*
			DebugLog("Ref: " +  iter->ptr->name);
			DebugLog("Val1: " + std::to_string((int64_t)iter->values[0]));
			DebugLog("Val2: " + std::to_string((int64_t)iter->values[1]));
			DebugLog("Val3: " + std::to_string((int64_t)iter->values[2]));
			DebugLog("SID: " + iter->sid);
			*/
			ogre_unordered_map<std::string, int>::type::iterator valueIter = iter->ptr->idata.find("value");
			if (valueIter != iter->ptr->idata.end())
			{
				//DebugLog("Value: " + std::to_string((int64_t)valueIter->second));
				// apply condition
				returnVal = returnVal && iter->values[0] == valueIter->second;
			}
		}
	}
	/*
	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = thisptr->getGameData()->objectReferences.begin();
	for (; iter != thisptr->getGameData()->objectReferences.end(); ++iter)
	{
		DebugLog(iter->first);
		for (Ogre::vector<GameDataReference>::type::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
		{
			DebugLog(iter2->sid);
		}
	}
	*/
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
	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = gameData->objectReferences.find("variable is");
	if(iter != gameData->objectReferences.end())
	{
		//DebugLog("Ref name: " + iter->first);
		for (Ogre::vector<GameDataReference>::type::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
		{
			//DebugLog("Ref val name: " + iter2->sid);
			int targetVal = iter2->values[0];
			ogre_unordered_map<std::string, int>::type::iterator valueIter = iter2->ptr->idata.find("value");

			if (valueIter != iter2->ptr->idata.end())
			{
				//DebugLog("Check " + std::to_string((int64_t)targetVal) + " " + std::to_string((uint64_t)valueIter->second));
				state = state && (targetVal == valueIter->second);
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
	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator setVar = dialogLine->getGameData()->objectReferences.find("set variable");
	if (setVar != dialogLine->getGameData()->objectReferences.end())
	{
		if (setVar->second.size() == 0)
		{
			ErrorLog("Missing references for \"set variable\"");
		}

		for (Ogre::vector<GameDataReference>::type::iterator iter = setVar->second.begin(); iter != setVar->second.end(); ++iter)
		{
			/*
			{
				DebugLog("Finding before: " + iter->sid);
				ogre_unordered_map<std::string, GameData*>::type::iterator saveDataIter = ou->savedata.gamedataSID.find(iter->sid);
				if (saveDataIter != ou->savedata.gamedataSID.end())
				{
					ogre_unordered_map<std::string, int>::type::iterator valueIter = saveDataIter->second->idata.find("value");
					int value = -1;
					if (valueIter != saveDataIter->second->idata.end())
						value = valueIter->second;
					DebugLog(saveDataIter->second->stringID + " " + saveDataIter->second->name + " " + std::to_string((int64_t)saveDataIter->second->type) + " " + std::to_string((int64_t)value));
				}
				else
					DebugLog("Not found!");
				saveDataIter = ou->gamedata.gamedataSID.find(iter->sid);
				if (saveDataIter != ou->gamedata.gamedataSID.end())
				{
					ogre_unordered_map<std::string, int>::type::iterator valueIter = saveDataIter->second->idata.find("value");
					int value = -1;
					if (valueIter != saveDataIter->second->idata.end())
						value = valueIter->second;
					DebugLog(saveDataIter->second->stringID + " " + saveDataIter->second->name + " " + std::to_string((int64_t)saveDataIter->second->type) + " " + std::to_string((int64_t)value));
				}
				else
					DebugLog("Not found!");
			}
			*/
			ogre_unordered_map<std::string, int>::type::iterator valueIter = iter->ptr->idata.find("value");

			if (valueIter != iter->ptr->idata.end())
			{
				// TODO persistant state


				DebugLog("Referenced value: " + std::to_string((int64_t)valueIter->second));
				valueIter->second = iter->values[0];

				/*
				//ou->gamedata.updateData(iter->ptr, true);
				//ou->savedata.addNewData(iter->ptr, iter->ptr->stringID);
				{
					DebugLog("Finding after: " + iter->sid);
					ogre_unordered_map<std::string, GameData*>::type::iterator saveDataIter = ou->savedata.gamedataSID.find(iter->sid);
					if (saveDataIter != ou->savedata.gamedataSID.end())
					{
						ogre_unordered_map<std::string, int>::type::iterator valueIter = saveDataIter->second->idata.find("value");
						int value = -1;
						if (valueIter != saveDataIter->second->idata.end())
							value = valueIter->second;
						DebugLog(saveDataIter->second->stringID + " " + saveDataIter->second->name + " " + std::to_string((int64_t)saveDataIter->second->type) + " " + std::to_string((int64_t)value));
					}
					else
						DebugLog("Not found!");
					saveDataIter = ou->gamedata.gamedataSID.find(iter->sid);
					if (saveDataIter != ou->gamedata.gamedataSID.end())
					{
						ogre_unordered_map<std::string, int>::type::iterator valueIter = saveDataIter->second->idata.find("value");
						int value = -1;
						if (valueIter != saveDataIter->second->idata.end())
							value = valueIter->second;
						DebugLog(saveDataIter->second->stringID + " " + saveDataIter->second->name + " " + std::to_string((int64_t)saveDataIter->second->type) + " " + std::to_string((int64_t)value));
					}
					else
						DebugLog("Not found!");
				}
				*/
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
void (*saveGameState_orig)(FactionManager* thisptr, GameDataContainer* container);
void saveGameState_hook(FactionManager* thisptr, GameDataContainer* container)
{
	//DebugLog("SAVING");
	// get variables
	// this doesn't seem to work
	/*
	lektor<GameData*> gameDataList;
	ou->savedata.getDataOfType(gameDataList, (itemType)VARIABLE);
	for (int i = 0; i < gameDataList.size(); ++i)
		DebugLog(gameDataList[i]->name);
		*/
	//container->readOnly = false;
	//container->isBaseDatafile = !container->isBaseDatafile;

	//DebugLog(std::to_string((uint64_t)container->gamedataSID.size()));
	ogre_unordered_map<std::string, GameData*>::type::iterator iter = ou->gamedata.gamedataSID.begin();
	for (; iter != ou->gamedata.gamedataSID.end(); ++iter)
	{
		//DebugLog(iter->first + " " + std::to_string((uint64_t)iter->second->type));
		if (iter->second->type == (itemType)VARIABLE)
		{
			//container->addNewData(iter->second, iter->second->stringID);
			/*
			lektor<GameData*> tempData;
			GameData* tempPtr = (*iter);
			tempData.count = 1;
			tempData.maxSize = 1;
			tempData.stuff = &tempPtr;
			container->addNewData(tempData);
			*/
			// TODO update other maps?
			//container->mainList.insert((*iter));
			/*
			GameData* newGameData = container->createNewData((itemType)VARIABLE, iter->second->stringID, iter->second->name);
			if (newGameData)
			{
				DebugLog("Added");
				DebugLog(newGameData->name);
				DebugLog(newGameData->stringID);
			}
			else
			{
				DebugLog("Could not create");
			}
			*/
			GameData* newGameData2 = container->createNewData(iter->second->type, iter->second->stringID, iter->second->name);
			if (newGameData2)
				newGameData2->updateFrom(iter->second, false);
			//container->mainList.emplace((*iter));
			// this is technically illegal
			//GameDataManager* casted = (GameDataManager*)container;
			//casted->updateData((*iter), true);
		}
	}
	//DebugLog(std::to_string((uint64_t)container->gamedataSID.size()));

	//DebugLog(container->name);
	/*
	iter = container->mainList.begin();
	for (; iter != container->mainList.end(); ++iter)
		if ((*iter)->type == (itemType)VARIABLE)
			DebugLog("Contains 1: " + (*iter)->name);
			*/
	// TODO leveleditor?
	//container->addNewData()
	saveGameState_orig(thisptr, container);
	/*
	iter = container->mainList.begin();
	for (; iter != container->mainList.end(); ++iter)
	{
		DebugLog((*iter)->name);
		if ((*iter)->type == (itemType)VARIABLE)
			DebugLog("Contains 2: " + (*iter)->name);
	}*/
}


void (*loadAllPlatoons_orig)(GameWorld* thisptr);
void loadAllPlatoons_hook(GameWorld* thisptr)
{
	loadAllPlatoons_orig(thisptr);
	DebugLog("Platoons Load");
	ogre_unordered_map<std::string, GameData*>::type::iterator iter = ou->savedata.gamedataSID.begin();
	for (; iter != ou->savedata.gamedataSID.end(); ++iter)
	{
		//DebugLog(iter->first + " " + std::to_string((uint64_t)iter->second->type));
		if (iter->second->type == (itemType)VARIABLE)
		{
			DebugLog("Found variable " + iter->second->name);
			DebugLog("Loading : " + std::to_string((int64_t)iter->second->idata.at("value")));
			ou->gamedata.updateData(iter->second, false);
			DebugLog("After Load: " + std::to_string((int64_t)ou->gamedata.gamedataSID.at(iter->first)->idata.at("value")));
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