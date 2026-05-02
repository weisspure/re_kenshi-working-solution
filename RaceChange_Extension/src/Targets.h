#pragma once

#include "ActionCore.h"

class Character;
class Dialogue;
class DialogLineData;

const char* RaceChangeRoleToString(RaceChangeTargetRole role);
Character* ResolveRaceChangeTarget(Dialogue* dlg, DialogLineData* dialogLine, RaceChangeTargetRole role);

