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

bool (*_checkCondition_orig)(Dialogue* thisptr, DialogConditionEnum conditionName, ComparisonEnum compareBy, int val, Character* target, Character* actualConversationTarget);
bool _checkCondition_hook(Dialogue* thisptr, DialogConditionEnum conditionName, ComparisonEnum compareBy, int val, Character* target, Character* actualConversationTarget)
{
	if (conditionName == DC_IS_SLEEPING)
	{
		return ((val > 0) == (actualConversationTarget->inSomething == UseStuffState::IN_BED));
	}
	else if (conditionName == DC_IS_ALLY_BECAUSE_OF_DISGUISE)
	{
		bool checkTrue = (compareBy == ComparisonEnum::CE_EQUALS && val == 1)
			|| (compareBy == ComparisonEnum::CE_MORE_THAN && val == 0);
		// "if ally factoring disguise and not ally not factoring disguise"
		return checkTrue == (target->isAlly(actualConversationTarget, true) && !target->isAlly(actualConversationTarget, false));
	}
	else if (conditionName == DC_WEAPON_LEVEL)
	{
		Weapon* weapon = actualConversationTarget->getCurrentWeapon();
		if (weapon != nullptr)
		{
			if (compareBy == ComparisonEnum::CE_EQUALS && weapon->getLevel() == val)
				return true;
			if (compareBy == ComparisonEnum::CE_LESS_THAN && weapon->getLevel() < val)
				return true;
			if (compareBy == ComparisonEnum::CE_MORE_THAN && weapon->getLevel() > val)
				return true;

		}

		return false;
	}
	else if (conditionName == DC_ARMOUR_LEVEL)
	{
		lektor<Item*> armour;
		armour.maxSize = 0;
		armour.count = 0;
		armour.stuff = nullptr;
		actualConversationTarget->getInventory()->getEquippedArmour(armour);
		for (int i = 0; i < armour.size(); ++i)
		{
			if (compareBy == ComparisonEnum::CE_EQUALS && armour[i]->getLevel() == val)
				return true;
			if (compareBy == ComparisonEnum::CE_LESS_THAN && armour[i]->getLevel() < val)
				return true;
			if (compareBy == ComparisonEnum::CE_MORE_THAN && armour[i]->getLevel() > val)
				return true;
		}

		// garbage collect
		free(armour.stuff);

		return false;
	}
	return _checkCondition_orig(thisptr, conditionName, compareBy, val, target, actualConversationTarget);
}

bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target);
bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target)
{
	for (int i = 0; i < thisptr->conditions.size(); ++i)
	{
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
					if (dialogCondition == ExtendedDialogConditionEnum::DC_HAS_SHORT_TERM_TAG)
					{
						TalkerEnum who = (TalkerEnum)iter->second[i].ptr->idata.find("who")->second;
						// T_ME behaviour - do I have memory tag for target
						Character* conditionTarget = me;
						Character* conditionCheck = target;
						if (who != TalkerEnum::T_ME)
						{
							// swap
							Character* temp = conditionTarget;
							conditionTarget = conditionCheck;
							conditionCheck = temp;
						}

						//ComparisonEnum compareBy = (ComparisonEnum)iter->second[i].ptr->idata.find("compare by")->second;
						int tag = iter->second[i].ptr->idata.find("tag")->second;
						if ((thisptr->conditions[i]->value > 0) != conditionTarget->getCharacterMemoryTag(conditionCheck, (CharacterPerceptionTags_ShortTerm)tag))
						{
							return false;
						}
					}
					// both of these can be done together
					else if (dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_MODIFIED
						|| dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_UNMODIFIED)
					{
						// swap between enum behaviour
						bool unmodified = dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_UNMODIFIED;
						
						TalkerEnum who = (TalkerEnum)iter->second[i].ptr->idata.find("who")->second;
						Character* conditionTarget = target;
						if (who == TalkerEnum::T_ME)
							conditionTarget = me;

						ComparisonEnum compareBy = (ComparisonEnum)iter->second[i].ptr->idata.find("compare by")->second;
						int value = iter->second[i].values[0];
						int tag = iter->second[i].ptr->idata.find("tag")->second;
						float stat = conditionTarget->getStats()->getStat((StatsEnumerated)tag, unmodified);

						if (compareBy == ComparisonEnum::CE_EQUALS && !((int)stat == value))
							return false;
						if (compareBy == ComparisonEnum::CE_LESS_THAN && !((int)stat < value))
							return false;
						if (compareBy == ComparisonEnum::CE_MORE_THAN && !((int)stat > value))
							return false;
					}
				}
			}
		}
	}
	return checkTags_orig(thisptr, me, target);
}

void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine);
void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
	ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = dialogLine->getGameData()->objectReferences.find("take item");
	if (iter != dialogLine->getGameData()->objectReferences.end())
	{
		if (iter->second.size() == 0)
		{
			ErrorLog("Missing references for \"take item\"");
		}

		for (Ogre::vector<GameDataReference>::type::iterator itemIter = iter->second.begin(); itemIter != iter->second.end(); ++itemIter)
		{
			Character* giver = thisptr->getConversationTarget().getCharacter();
			Character* taker = thisptr->me;
			if (giver != nullptr && taker != nullptr)
			{
				for (int i = 0; i < itemIter->values[0]; ++i)
				{
					Item* item = giver->inventory->getItem(itemIter->ptr);

					// stop when the character no longer has instances of the item
					if (item == nullptr)
						break;

					giver->dropItem(item);
					taker->giveItem(item, true, false);
				}
			}
		}
	}

	iter = dialogLine->getGameData()->objectReferences.find("destroy item");
	if (iter != dialogLine->getGameData()->objectReferences.end())
	{
		if (iter->second.size() == 0)
		{
			ErrorLog("Missing references for \"destroy item\"");
		}

		for (Ogre::vector<GameDataReference>::type::iterator itemIter = iter->second.begin(); itemIter != iter->second.end(); ++itemIter)
		{
			Character* target = thisptr->getConversationTarget().getCharacter();
			if (target != nullptr)
			{
				for (int i = 0; i < itemIter->values[0]; ++i)
				{
					Item* item = target->inventory->getItem(itemIter->ptr);

					// stop when the character no longer has instances of the item
					if (item == nullptr)
						break;

					// get rid of inventory references or something, no idea if this is needed but it seems like a good idea
					target->dropItem(item);
					ou->destroy(item, false, "Destroy item event");
				}
			}
		}
	}
	// continue
	_doActions_orig(thisptr, dialogLine);
}

__declspec(dllexport) void startPlugin()
{
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Dialogue::_checkCondition), &_checkCondition_hook, &_checkCondition_orig))
		ErrorLog("Dialogue Extensions: could not install hook!");
	if(KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&DialogLineData::checkTags), &checkTags_hook, &checkTags_orig))
		ErrorLog("Dialogue Extensions: could not install hook!");
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Dialogue::_doActions), &_doActions_hook, &_doActions_orig))
		DebugLog("Dialogue Extensions: Could not hook function!");
}