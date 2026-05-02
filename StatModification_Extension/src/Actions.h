#pragma once

#include <kenshi/Dialogue.h>

/**
 * @brief Dispatches all supported StatModification action references on one dialogue line.
 *
 * Called from Dialogue::_doActions after the line has passed condition checks.
 * Missing action keys are no-ops. Multiple keys on one line are supported.
 * Train/untrain actions run before until actions, so an until action on the same
 * line intentionally overwrites an earlier adjustment.
 */
void DispatchStatActions(Dialogue* dlg, DialogLineData* dialogLine);
