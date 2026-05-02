#include "Targets.h"

#include <kenshi/Platoon.h>
#include <kenshi/RootObject.h>

/** @brief Converts a TargetRole to the matching FCS-facing role name. */
std::string TargetRoleToString(TargetRole role)
{
	switch (role)
	{
	case ROLE_SPEAKER:
		return "speaker";

	case ROLE_OTHER:
		return "other";

	case ROLE_SPEAKER_SQUAD:
		return "speaker squad";

	case ROLE_OTHER_SQUAD:
		return "other squad";
	}

	return "unknown";
}

/** @brief Adds a non-null Character pointer to a target list. */
static void AddCharacter(std::vector<Character*>& targets, Character* character)
{
	if (character == 0)
		return;

	targets.push_back(character);
}

/**
 * @brief Enumerates active platoon members for an already-resolved anchor character.
 *
 * This intentionally uses the active platoon container rather than
 * Dialogue::getSpeaker(T_WHOLE_SQUAD), because that speaker path returns one
 * selected character rather than an iterable squad.
 */
static void AddPlatoonCharacters(std::vector<Character*>& targets, Character* anchor)
{
	if (anchor == 0)
		return;

	ActivePlatoon* platoon = anchor->getPlatoon();
	if (platoon == 0)
		return;

	lektor<RootObject*>* things = platoon->getThings();
	if (things == 0)
		return;

	for (uint32_t i = 0; i < things->size(); ++i)
	{
		RootObject* object = (*things)[i];
		Character* character = dynamic_cast<Character*>(object);
		AddCharacter(targets, character);
	}
}

/**
 * @brief Resolves a Character* from a Dialogue using a TargetRole.
 *
 * ROLE_SPEAKER delegates to Kenshi's getSpeaker so NPC lines, player replies,
 * and interjectors use the same resolution path as vanilla dialogue.
 *
 * ROLE_OTHER is a two-side helper: owner speakers resolve other to target;
 * every other speaker resolves other to owner.
 */
Character* ResolveTarget(Dialogue* dlg, DialogLineData* dialogLine, TargetRole role)
{
	if (dlg == 0)
		return 0;

	switch (role)
	{
	case ROLE_SPEAKER:
		if (dialogLine == 0)
			return 0;
		return dlg->getSpeaker(dialogLine->speaker, dialogLine, false);

	case ROLE_OTHER:
		if (dialogLine == 0)
			return 0;
		{
			Character* speaker = dlg->getSpeaker(dialogLine->speaker, dialogLine, false);
			if (speaker == 0)
				return 0;

			Character* owner = dlg->me;
			if (speaker == owner)
				return dlg->getConversationTarget().getCharacter();

			return owner;
		}

	case ROLE_SPEAKER_SQUAD:
		return ResolveTarget(dlg, dialogLine, ROLE_SPEAKER);

	case ROLE_OTHER_SQUAD:
		return ResolveTarget(dlg, dialogLine, ROLE_OTHER);
	}

	return 0;
}

std::vector<Character*> ResolveTargets(Dialogue* dlg, DialogLineData* dialogLine, TargetRole role)
{
	std::vector<Character*> targets;

	switch (role)
	{
	case ROLE_SPEAKER:
	case ROLE_OTHER:
		AddCharacter(targets, ResolveTarget(dlg, dialogLine, role));
		break;

	case ROLE_SPEAKER_SQUAD:
	case ROLE_OTHER_SQUAD:
		AddPlatoonCharacters(targets, ResolveTarget(dlg, dialogLine, role));
		break;
	}

	return targets;
}
