#pragma once

#include "ActionCore.h"

class Character;
class Dialogue;
class DialogLineData;

/** Convert a target role to a stable lowercase diagnostic string. */
const char* RaceChangeRoleToString(RaceChangeTargetRole role);

/** Resolve the runtime character targeted by a RaceChange dialogue action. */
Character* ResolveRaceChangeTarget(Dialogue* dlg, DialogLineData* dialogLine, RaceChangeTargetRole role);
