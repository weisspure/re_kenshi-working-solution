#pragma once

#include <kenshi/Appearance.h>
#include <kenshi/InputHandler.h>
#include <kenshi/Globals.h>
#include <kenshi/Character.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Faction.h>
#include <kenshi/FactionRelations.h>
#include <core/Functions.h>
#include <ogre/OgreMaterial.h>
#include <ogre/OgreTechnique.h>
#include <ogre/OgreEntity.h>
#include <ogre/OgreMaterialManager.h>
#include <ogre/OgreSharedPtr.h>
#include <Debug.h>

//bool lastHighlight = false;

void (*GameWorld__mainLoop_GPUSensitiveStuff_orig)(GameWorld* thisptr, float time);
void GameWorld__mainLoop_GPUSensitiveStuff_hook(GameWorld* thisptr, float time)
{
	//if (lastHighlight != key->highlight)
	{
		if(key->highlight)
		{
			const ogre_unordered_set<Character*>::type& characters = thisptr->getCharacterUpdateList();
			for (ogre_unordered_set<Character*>::type::iterator iter = characters.begin(); iter != characters.end(); ++iter)
			{
				if (!(*iter)->getAppearance()->bodyMaterial.isNull())
				{
					// isEnemy if enemy with at least one character
					bool isEnemy = false;
					for (int i = 0; i < thisptr->player->playerCharacters.size() && !isEnemy; ++i)
						isEnemy = (*iter)->isEnemy(thisptr->player->playerCharacters[i], true);
					//for (int i = 0; i < thisptr->player->playerCharacters.size() && !isEnemy; ++i)
					//	isEnemy = (*iter)->shouldIScrewThisGuyOver(thisptr->player->playerCharacters[i]);
					//for (int i = 0; i < thisptr->player->playerCharacters.size() && !isEnemy; ++i)
					//	isEnemy = (*iter)->areYouGonnaGetMe(thisptr->player->playerCharacters[i]);
					for (int i = 0; i < thisptr->player->playerCharacters.size() && !isEnemy; ++i)
						isEnemy = thisptr->player->playerCharacters[i]->areYouGonnaGetMe((*iter));

					// isAlly if allied with all characters
					bool isAlly = true;
					for (int i = 0; i < thisptr->player->playerCharacters.size() && isAlly; ++i)
						isAlly = (*iter)->isAlly(thisptr->player->playerCharacters[i], true);

					// if neither, colour based on relations
					// scale -100-100 to -1-1
					float factionRelations = ((*iter)->getFaction()->relations->getFactionRelation(thisptr->player->getFaction()) / 100.0f);

					if ((*iter)->isPlayerCharacter())
						(*iter)->getAppearance()->bodyMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("coloroverride", Ogre::ColourValue(0.0, 0.0, 1.0, 1.0));
					else if (isEnemy)
						(*iter)->getAppearance()->bodyMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("coloroverride", Ogre::ColourValue(1.0, 0.0, 0.0, 1.0));
					else if (isAlly)
						(*iter)->getAppearance()->bodyMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("coloroverride", Ogre::ColourValue(0.0, 1.0, 0.0, 1.0));
					else
						(*iter)->getAppearance()->bodyMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("coloroverride", Ogre::ColourValue(std::max(0.0f, - factionRelations + 1.0f), factionRelations + 1, 0.0, 1.0));

					(*iter)->getAppearance()->bodyMaterial->getTechnique(0)->getPass(0)->getVertexProgramParameters()->setNamedConstant("overrideDepth", true);
				}
			}
		}
		else
		{
			const ogre_unordered_set<Character*>::type& characters = thisptr->getCharacterUpdateList();
			for (ogre_unordered_set<Character*>::type::iterator iter = characters.begin(); iter != characters.end(); ++iter)
			{
				if (!(*iter)->getAppearance()->bodyMaterial.isNull())
				{
					(*iter)->getAppearance()->bodyMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("coloroverride", Ogre::ColourValue(1.0, 1.0, 1.0, 0.0));
					(*iter)->getAppearance()->bodyMaterial->getTechnique(0)->getPass(0)->getVertexProgramParameters()->setNamedConstant("overrideDepth", false);
				}
			}
		}
		//lastHighlight = key->highlight;
	}
	GameWorld__mainLoop_GPUSensitiveStuff_orig(thisptr, time);
}

__declspec(dllexport) void startPlugin()
{
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff), &GameWorld__mainLoop_GPUSensitiveStuff_hook, &GameWorld__mainLoop_GPUSensitiveStuff_orig))
		ErrorLog("Character Highlight: could not install hook!");
}