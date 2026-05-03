#pragma once

class Dialogue;
class DialogLineData;

/** Scan a dialogue line for RaceChange actions and apply each supported reference. */
void DispatchRaceChangeActions(Dialogue* dlg, DialogLineData* dialogLine);
