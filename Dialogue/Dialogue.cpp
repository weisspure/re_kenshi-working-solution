#include <kenshi/Dialogue.h>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/GameData.h>

enum ExtendedDialogConditionEnum
{
	DC_IS_SLEEPING = 1000,
	DC_IS_ALLY_BECAUSE_OF_DISGUISE,
	DC_HAS_SHORT_TERM_TAG
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
	else if (conditionName == DC_HAS_SHORT_TERM_TAG)
	{
		// TODO test
	}
	return _checkCondition_orig(thisptr, conditionName, compareBy, val, target, actualConversationTarget);
}

bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target);
bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target)
{
	for (int i = 0; i < thisptr->conditions.size(); ++i)
	{
		if (thisptr->conditions[i]->key == ExtendedDialogConditionEnum::DC_HAS_SHORT_TERM_TAG)
		{
			ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator iter = thisptr->data->objectReferences.find("conditions");
			
			if (iter != thisptr->data->objectReferences.end())
			{
				for (int i = 0; i < iter->second.size(); ++i)
				{
					if (iter->second[i].ptr->idata.find("condition name")->second == ExtendedDialogConditionEnum::DC_HAS_SHORT_TERM_TAG)
					{
						//TalkerEnum who = (TalkerEnum)iter->second[i].ptr->idata.find("who")->second;
						//ComparisonEnum compareBy = (ComparisonEnum)iter->second[i].ptr->idata.find("compare by")->second;
						int tag = iter->second[i].ptr->idata.find("tag")->second;
						if ((thisptr->conditions[i]->value > 0) != me->getCharacterMemoryTag(target, (CharacterPerceptionTags_ShortTerm)tag))
						{
							return false;
						}
					}
				}
			}
			
		}
	}
	return checkTags_orig(thisptr, me, target);
}

__declspec(dllexport) void startPlugin()
{
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Dialogue::_checkCondition), &_checkCondition_hook, &_checkCondition_orig))
		ErrorLog("Dialogue Extensions: could not install hook!");
	if(KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&DialogLineData::checkTags), &checkTags_hook, &checkTags_orig))
		ErrorLog("Dialogue Extensions: could not install hook!");
}