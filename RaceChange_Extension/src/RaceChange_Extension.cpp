#include "Logging.h"
#include "RaceActions.h"

#include <core/Functions.h>

#include <kenshi/Dialogue.h>

static void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine) = 0;

static void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
	if (thisptr != 0 && dialogLine != 0)
		DispatchRaceChangeActions(thisptr, dialogLine);

	if (_doActions_orig == 0)
	{
		LogError("_doActions_orig is null, cannot call original");
		return;
	}

	_doActions_orig(thisptr, dialogLine);
}

__declspec(dllexport) void startPlugin()
{
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(
		KenshiLib::GetRealAddress(&Dialogue::_doActions),
		&_doActions_hook,
		&_doActions_orig))
	{
		LogError("failed to hook Dialogue::_doActions");
		return;
	}

	LogInfo("loaded and Dialogue::_doActions hook installed");
}

