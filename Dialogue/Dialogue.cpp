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

bool (*DialogLineData_checkConditions_orig)(DialogLineData* thisptr, Dialogue* dialog, Character* target, bool isWordswap);
bool DialogLineData_checkConditions_hook(DialogLineData* thisptr, Dialogue* dialog, Character* target, bool isWordswap)
{
	for (DialogLineData::DialogCondition** condition = thisptr->conditions.begin(); condition < thisptr->conditions.end(); ++condition)
	{
		// T_ME behaviour - do I have memory tag for target
		Character* characterTarget = dialog->getCharacter();
		Character* characterCheck = dialog->getConversationTarget().getCharacter();

		if ((*condition)->who != TalkerEnum::T_ME)
		{
			// swap
			Character* temp = characterTarget;
			characterTarget = characterCheck;
			characterCheck = temp;
		}

		switch ((*condition)->key)
		{
			case DC_IS_SLEEPING:
				if (!DialogCompare(characterTarget->inSomething == UseStuffState::IN_BED, *condition))
					return false;
				break;
			case DC_IS_ALLY_BECAUSE_OF_DISGUISE:
				if (!DialogCompare((characterCheck->isAlly(characterTarget, true) && !characterCheck->isAlly(characterTarget, false)), *condition))
					return false;
				break;
			case DC_WEAPON_LEVEL:
			{
				// Note: value is -1 if unarmed
				Weapon* weapon = characterTarget->getCurrentWeapon();
				int level = weapon == nullptr ? -1 : weapon->getLevel();
				if (!DialogCompare(level, *condition))
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
				characterTarget->getInventory()->getEquippedArmour(armour);
				bool hasMatch = false;
				// check if any equipped armour meets condition
				for (int i = 0; i < armour.size(); ++i)
					if (DialogCompare(armour[i]->getLevel(), *condition))
						hasMatch = true;
				// unarmoured
				if (armour.size() == 0)
					hasMatch = DialogCompare(-1, *condition);
				// garbage collect
				free(armour.stuff);
				// return false if no armour matches
				if (!hasMatch)
					return false;
				break;
			}
		}
	}
	return DialogLineData_checkConditions_orig(thisptr, dialog, target, isWordswap);
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
					Character* conditionTarget = me;
					Character* conditionCheck = target;
					if (who != TalkerEnum::T_ME)
					{
						// swap
						Character* temp = conditionTarget;
						conditionTarget = conditionCheck;
						conditionCheck = temp;
					}

					if (dialogCondition == ExtendedDialogConditionEnum::DC_HAS_SHORT_TERM_TAG)
					{
						if(!DialogCompare(conditionTarget->getCharacterMemoryTag(conditionCheck, (CharacterPerceptionTags_ShortTerm)tag), value, compareBy))
							return false;
					}
					// both of these can be done together
					else if (dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_MODIFIED
						|| dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_UNMODIFIED)
					{
						// swap between enum behaviour
						bool unmodified = dialogCondition == ExtendedDialogConditionEnum::DC_STAT_LEVEL_UNMODIFIED;

						float stat = conditionTarget->getStats()->getStat((StatsEnumerated)tag, unmodified);
						if(!DialogCompare((int)stat, value, compareBy));
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
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&DialogLineData::checkConditions), &DialogLineData_checkConditions_hook, &DialogLineData_checkConditions_orig))
		ErrorLog("Dialogue Extensions: could not install hook!");
	if(KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&DialogLineData::checkTags), &checkTags_hook, &checkTags_orig))
		ErrorLog("Dialogue Extensions: could not install hook!");
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Dialogue::_doActions), &_doActions_hook, &_doActions_orig))
		DebugLog("Dialogue Extensions: Could not hook function!");
}