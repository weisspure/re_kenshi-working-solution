#pragma once

#include "ClampCore.h"

#include <kenshi/GameData.h>

#include <string>

/**
 * @brief Reads an optional CLAMP_PROFILE reference from an action record.
 *
 * No clamp profile means no clamping. Wrong record types or malformed profiles
 * log errors and continue unclamped because clamping is explicitly optional.
 */
ClampConfig ReadClampProfile(GameData* actionRecord, const std::string& context);
