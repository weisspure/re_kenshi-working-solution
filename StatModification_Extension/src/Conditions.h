#pragma once

#include <kenshi/Dialogue.h>
#include <kenshi/Character.h>

/** @brief Saved pointer to the original DialogLineData::checkTags, filled by AddHook. */
extern bool (*checkTags_orig)(DialogLineData* thisptr, Character* me, Character* target);

/**
 * @brief Hook for DialogLineData::checkTags.
 *
 * Evaluates this plugin's comparison conditions, then calls the original so
 * vanilla and other-plugin conditions still decide whether the line can run.
 */
bool checkTags_hook(DialogLineData* thisptr, Character* me, Character* target);
