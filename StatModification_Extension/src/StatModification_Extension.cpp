// StatModification_Extension.cpp
//
// KenshiLib plugin entry point.
//
// This file only installs hooks and forwards runtime work to focused modules:
//   Actions.cpp    - FCS dialogue actions that adjust or set stats
//   Conditions.cpp - FCS dialogue conditions that compare stats
//   Targets.cpp    - dialogue participant resolution helpers
//   Clamp.cpp      - optional clamp profile handling
//   FcsData.cpp    - safe GameData/FCS record readers
//   Logging.cpp    - RE_KENSHI logging helpers

#include "Actions.h"
#include "Conditions.h"
#include "Logging.h"

#include <core/Functions.h>

#include <kenshi/Dialogue.h>

/** @brief Saved pointer to the original Dialogue::_doActions, set during hook installation. */
static void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine) = 0;

/**
 * @brief Hook for Dialogue::_doActions.
 *
 * Called by the engine once per dialogue line execution, before the engine's
 * own action handler runs. Custom stat actions are dispatched here first,
 * then the original function is called so built-in actions still work.
 *
 * If _doActions_orig is null, an error is logged and the call is skipped
 * rather than crashing.
 */
static void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
	if (thisptr != 0 && dialogLine != 0)
		DispatchStatActions(thisptr, dialogLine);

	if (_doActions_orig == 0)
	{
		LogError("_doActions_orig is null, cannot call original");
		return;
	}

	_doActions_orig(thisptr, dialogLine);
}

/**
 * @brief DLL entry point called by KenshiLib when the plugin is loaded.
 *
 * Beginners: start reading here. This function is the doorway into the plugin.
 * It installs both hooks and fails visibly through RE_KENSHI logs if either
 * hook cannot be installed.
 */
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

	if (KenshiLib::SUCCESS != KenshiLib::AddHook(
		KenshiLib::GetRealAddress(&DialogLineData::checkTags),
		&checkTags_hook,
		&checkTags_orig))
	{
		LogError("failed to hook DialogLineData::checkTags");
		return;
	}

	LogInfo("loaded and hooks installed");
}
