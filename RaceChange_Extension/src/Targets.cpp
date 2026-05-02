#include "Targets.h"

#include <kenshi/Dialogue.h>

const char* RaceChangeRoleToString(RaceChangeTargetRole role)
{
	switch (role)
	{
	case RACE_CHANGE_ROLE_SPEAKER:
		return "speaker";
	case RACE_CHANGE_ROLE_OTHER:
		return "other";
	default:
		return "unknown";
	}
}

Character* ResolveRaceChangeTarget(Dialogue* dlg, DialogLineData* dialogLine, RaceChangeTargetRole role)
{
	if (dlg == 0 || dialogLine == 0)
		return 0;

	switch (role)
	{
	case RACE_CHANGE_ROLE_SPEAKER:
		return dlg->getSpeaker(dialogLine->speaker, dialogLine, false);

	case RACE_CHANGE_ROLE_OTHER:
	{
		Character* speaker = dlg->getSpeaker(dialogLine->speaker, dialogLine, false);
		if (speaker == 0)
			return 0;

		if (speaker == dlg->me)
			return dlg->getConversationTarget().getCharacter();

		return dlg->me;
	}

	default:
		return 0;
	}
}

