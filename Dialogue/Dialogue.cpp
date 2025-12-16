#include <kenshi/Dialogue.h>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/GameData.h>
#include <kenshi/Gear.h>
#include <kenshi/CharStats.h>
#include <kenshi/Inventory.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/RootObjectFactory.h>
#include <kenshi/Platoon.h>

enum ExtendedDialogConditionEnum
{
	DC_IS_SLEEPING = 1000,
	DC_HAS_SHORT_TERM_TAG,
	DC_IS_ALLY_BECAUSE_OF_DISGUISE,
	DC_STAT_LEVEL_UNMODIFIED,
	DC_STAT_LEVEL_MODIFIED,
	DC_WEAPON_LEVEL,
	DC_ARMOUR_LEVEL
};

static const float SQUAD_CHECK_RADIUS = 900.0f;

// TODO remove?
static bool DialogCompare(int val1, int val2, ComparisonEnum compareBy)
{

	if (compareBy == ComparisonEnum::CE_EQUALS && val1 == val2)
		return true;
	if (compareBy == ComparisonEnum::CE_LESS_THAN && val1 < val2)
		return true;
	if (compareBy == ComparisonEnum::CE_MORE_THAN && val1 > val2)
		return true;

	return false;
}

static bool DialogCompare(int val1, DialogLineData::DialogCondition* condition)
{

	if (condition->compareBy == ComparisonEnum::CE_EQUALS && val1 == condition->value)
		return true;
	if (condition->compareBy == ComparisonEnum::CE_LESS_THAN && val1 < condition->value)
		return true;
	if (condition->compareBy == ComparisonEnum::CE_MORE_THAN && val1 > condition->value)
		return true;

	return false;
}

static bool checkCondition(Character* characterCheck, Character* characterTarget, DialogLineData::DialogCondition* condition)
{
	switch (condition->key)
	{
		case DC_IS_SLEEPING:
			if (!DialogCompare(characterCheck->inSomething == UseStuffState::IN_BED, condition))
				return false;
			break;
		case DC_IS_ALLY_BECAUSE_OF_DISGUISE:
			if (!DialogCompare((characterCheck->isAlly(characterTarget, true) && !characterCheck->isAlly(characterTarget, false)), condition))
				return false;
			break;
		case DC_WEAPON_LEVEL:
		{
			// Note: value is -1 if unarmed
			// this check often doesn't check equipped weapons on back
			Weapon* weapon = characterCheck->getCurrentWeapon();
			// this seems to be the same
			if(!weapon)
				weapon = characterCheck->getThePreferredWeapon();
			int level = weapon == nullptr ? -1 : weapon->getLevel();
			if (!weapon)
			{
				lektor<InventorySection*> sections;
				characterCheck->inventory->getAllSectionsOfType(sections, AttachSlot::ATTACH_WEAPON);
				for (int i = 0; i < sections.size(); ++i)
				{
					const Ogre::vector<InventorySection::SectionItem>::type& items = sections[i]->getItems();
					for (int j = 0; j < items.size(); ++j)
						if (weapon = dynamic_cast<Weapon*>(items[j].item))
							level = std::max(level, weapon->getLevel());
				}

				// cleanup
				free(sections.stuff);
			}
			
			if (!DialogCompare(level, condition))
				return false;
			break;
		}
		case DC_ARMOUR_LEVEL:
		{
			// Note: value is -1 if unarmoured
			lektor<Item*> armour;
			armour.maxSize = 0;
			armour.count = 0;
			armour.stuff = nullptr;
			characterCheck->getInventory()->getEquippedArmour(armour);
			bool hasMatch = false;
			// check if any equipped armour meets condition
			for (int i = 0; i < armour.size(); ++i)
				if (DialogCompare(armour[i]->getLevel(), condition))
					hasMatch = true;
			// unarmoured
			if (armour.size() == 0)
				hasMatch = DialogCompare(-1, condition);
			// garbage collect
			if (armour.stuff)
				free(armour.stuff);
			// return false if no armour matches
			if (!hasMatch)
				return false;
			break;
		}
	}
	return true;
}

bool (*DialogLineData_checkConditions_orig)(DialogLineData* thisptr, Dialogue* dialog, Character* target, bool isWordswap);
bool DialogLineData_checkConditions_hook(DialogLineData* thisptr, Dialogue* dialog, Character* target, bool isWordswap)
{
	for (DialogLineData::DialogCondition** condition = thisptr->conditions.begin(); condition < thisptr->conditions.end(); ++condition)
	{
		// T_ME behaviour - do I have memory tag for target
		Character* characterCheck = dialog->getCharacter();
		Character* characterTarget = target;
		
		if ((*condition)->who != TalkerEnum::T_ME && (*condition)->who != TalkerEnum::T_WHOLE_SQUAD)
		{
			// swap
			Character* temp = characterTarget;
			characterTarget = characterCheck;
			characterCheck = temp;
		}

		if (!characterCheck)
		{
			ErrorLog("NO SPEAKER");
			break;
		}

		if (!characterTarget)
		{
			ErrorLog("NO TARGET");
		}

		if ((*condition)->who == TalkerEnum::T_WHOLE_SQUAD)
		{
			// with above branch, characterCheck will be "me"
			ActivePlatoon* activePlatoon = characterCheck->getPlatoon();
			lektor<RootObject*> characters;
			// couldn't find T_WHOLE_SQUAD radius but interjection radius is similar and appears to be 900
			activePlatoon->getCharactersInArea(characters, characterCheck->getPosition(), SQUAD_CHECK_RADIUS, false);

			// if any
			bool found = false;
			for (int i = 0; i < characters.size(); ++i)
			{
				Character* squadChar = dynamic_cast<Character*>(characters[i]);
				if (squadChar)
				{
					if (checkCondition(squadChar, characterTarget, *condition))
						// condition is met -  move on to the next condition
						found = true;
						//break;
				}
			}

			// cleanup
			if (characters.stuff)
				free(characters.stuff);

			if (!found)
				return false;
		}
		else
		{
			if (!checkCondition(characterCheck, characterTarget, *condition))
				return false;
		}
	}
	return DialogLineData_checkConditions_orig(thisptr, dialog, target, isWordswap);
}

bool checkTag(ExtendedDialogConditionEnum dialogCondition, Character* conditionCheck, Character* conditionTarget, ComparisonEnum compareBy, int tag, int value)
{
	if (dialogCondition == ExtendedDialogConditionEnum::DC_HAS_SHORT_TERM_TAG)
	{
		return DialogCompare(conditionCheck->getCharacterMemoryTag(conditionTarget, (CharacterPerceptionTags_ShortTerm)tag), value, compareBy);
	}
	// both of these can be done together
	else if (dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_MODIFIED
		|| dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_UNMODIFIED)
	{
		// swap between enum behaviour
		bool unmodified = dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_UNMODIFIED;

		float stat = conditionCheck->getStats()->getStat((StatsEnumerated)tag, unmodified);
		return DialogCompare((int)stat, value, compareBy);
	}

	// not our problem
	return true;
}

bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target);
bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target)
{
	for (int i = 0; i < thisptr->conditions.size(); ++i)
	{
		// optimization - only do a full check if there's an actual extended condition
		if (thisptr->conditions[i]->key == ExtendedDialogConditionEnum::DC_HAS_SHORT_TERM_TAG
			|| thisptr->conditions[i]->key == DC_STAT_LEVEL_UNMODIFIED
			|| thisptr->conditions[i]->key == DC_STAT_LEVEL_MODIFIED)
		{
			ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = thisptr->data->objectReferences.find("conditions");
			
			if (iter != thisptr->data->objectReferences.end())
			{
				for (int i = 0; i < iter->second.size(); ++i)
				{
					ExtendedDialogConditionEnum dialogCondition = (ExtendedDialogConditionEnum)iter->second[i].ptr->idata.find("condition name")->second;
					ComparisonEnum compareBy = (ComparisonEnum)iter->second[i].ptr->idata.find("compare by")->second;
					TalkerEnum who = (TalkerEnum)iter->second[i].ptr->idata.find("who")->second;
					int tag = iter->second[i].ptr->idata.find("tag")->second;
					int value = iter->second[i].values[0];

					// T_ME behaviour - do I have tag for target
					Character* conditionCheck = me;
					// Note: target can sometimes be null, seems to happen on interjection nodes
					Character* conditionTarget = target;
					if (who != TalkerEnum::T_ME && who != TalkerEnum::T_WHOLE_SQUAD)
					{
						// swap
						Character* temp = conditionTarget;
						conditionTarget = conditionCheck;
						conditionCheck = temp;
					}

					if (who == TalkerEnum::T_WHOLE_SQUAD)
					{
						ActivePlatoon* platoon = conditionCheck->getPlatoon();
						if (platoon)
						{
							lektor<RootObject*> characters;
							// couldn't find T_WHOLE_SQUAD radius but interjection radius is similar and appears to be 900
							platoon->getCharactersInArea(characters, conditionCheck->getPosition(), SQUAD_CHECK_RADIUS, false);

							bool found = false;
							for (int i = 0; i < characters.size(); ++i)
							{
								Character* squadChar = dynamic_cast<Character*>(characters[i]);
								// if any
								if (squadChar)
								{
									if (checkTag(dialogCondition, squadChar, conditionTarget, compareBy, tag, value))
										// condition is met -  move on to the next condition
										found = true;
								}
							}

							// cleanup
							if (characters.stuff)
								free(characters.stuff);

							if (!found)
								return false;
						}
					}
					else
					{
						if (!checkTag(dialogCondition, conditionCheck, conditionTarget, compareBy, tag, value))
							return false;
					}
				}
			}
			// above loop should check all conditions
			break;
		}
	}
	return checkTags_orig(thisptr, me, target);
}

// returns num items taken
int takeItems(Character* giver, Character* taker, GameData* itemData, int count)
{
	for (int i = 0; i < count; ++i)
	{
		Item* item = giver->inventory->getItem(itemData);

		// stop when the character no longer has instances of the item
		if (item == nullptr)
			return i;

		giver->dropItem(item);
		taker->giveItem(item, true, false);
	}
	return count;
}

// returns num items destroyed
int destroyItems(Character* target, GameData* itemData, int count)
{
	for (int i = 0; i < count; ++i)
	{
		Item* item = target->inventory->getItem(itemData);

		// stop when the character no longer has instances of the item
		if (item == nullptr)
			return i;

		// get rid of inventory references or something, no idea if this is needed but it seems like a good idea
		target->dropItem(item);
		ou->destroy(item, false, "Destroy item event");
	}
	return count;
}

void doRefAction(const std::string &action, Ogre::vector<GameDataReference>::type &ref, Dialogue* thisptr)
{
	if (ref.size() == 0)
	{
		ErrorLog("Missing references for \"" + action + "\"");
	}

	if (action == "take item" || action == "take item from squad")
	{
		for (Ogre::vector<GameDataReference>::type::iterator itemIter = ref.begin(); itemIter != ref.end(); ++itemIter)
		{
			Character* giver = thisptr->getConversationTarget().getCharacter();
			Character* taker = thisptr->me;
			if (giver != nullptr && taker != nullptr)
			{
				if (action == "take item")
				{
					takeItems(giver, taker, itemIter->ptr, itemIter->values[0]);
				}
				else
				{
					ActivePlatoon* activePlatoon = giver->getPlatoon();
					lektor<RootObject*> characters;
					// couldn't find T_WHOLE_SQUAD radius but interjection radius is similar and appears to be 900
					activePlatoon->getCharactersInArea(characters, taker->getPosition(), SQUAD_CHECK_RADIUS, false);

					int itemsLeft = itemIter->values[0];
					for (int c = 0; c < characters.size(); ++c)
					{
						Character* squadChar = dynamic_cast<Character*>(characters[c]);
						if (squadChar)
							itemsLeft -= takeItems(squadChar, taker, itemIter->ptr, itemsLeft);
						if (itemsLeft == 0)
							break;
					}

					// cleanup
					free(characters.stuff);
				}
			}
		}
	}
	else if(action == "destroy item" || action == "destroy item from squad")
	{

		for (Ogre::vector<GameDataReference>::type::iterator itemIter = ref.begin(); itemIter != ref.end(); ++itemIter)
		{
			Character* target = thisptr->getConversationTarget().getCharacter();
			if (target != nullptr)
			{
				if (action == "destroy item")
				{
					destroyItems(target, itemIter->ptr, itemIter->values[0]);
				}
				else
				{
					ActivePlatoon* activePlatoon = target->getPlatoon();
					lektor<RootObject*> characters;
					// couldn't find T_WHOLE_SQUAD radius but interjection radius is similar and appears to be 900
					activePlatoon->getCharactersInArea(characters, target->getPosition(), SQUAD_CHECK_RADIUS, false);

					int itemsLeft = itemIter->values[0];
					for (int c = 0; c < characters.size(); ++c)
					{
						Character* squadChar = dynamic_cast<Character*>(characters[c]);
						if (squadChar)
							itemsLeft -= destroyItems(squadChar, itemIter->ptr, itemsLeft);
						if (itemsLeft == 0)
							break;
					}

					// cleanup
					free(characters.stuff);
				}
			}
		}
	}
}

void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine);
void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = dialogLine->getGameData()->objectReferences.find("take item");
	if (iter != dialogLine->getGameData()->objectReferences.end())
		doRefAction("take item", iter->second, thisptr);
	
	iter = dialogLine->getGameData()->objectReferences.find("take item from squad");
	if (iter != dialogLine->getGameData()->objectReferences.end())
		doRefAction("take item from squad", iter->second, thisptr);
	
	iter = dialogLine->getGameData()->objectReferences.find("destroy item");
	if (iter != dialogLine->getGameData()->objectReferences.end())
		doRefAction("destroy item", iter->second, thisptr);

	iter = dialogLine->getGameData()->objectReferences.find("destroy item from squad");
	if (iter != dialogLine->getGameData()->objectReferences.end())
		doRefAction("destroy item from squad", iter->second, thisptr);

	// continue
	_doActions_orig(thisptr, dialogLine);
}

__declspec(dllexport) void startPlugin()
{
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&DialogLineData::checkConditions), &DialogLineData_checkConditions_hook, &DialogLineData_checkConditions_orig))
		ErrorLog("Dialogue Extensions: could not install hook!");
	if(KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&DialogLineData::checkTags), &checkTags_hook, &checkTags_orig))
		ErrorLog("Dialogue Extensions: could not install hook!");
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Dialogue::_doActions), &_doActions_hook, &_doActions_orig))
		DebugLog("Dialogue Extensions: Could not hook function!");
}