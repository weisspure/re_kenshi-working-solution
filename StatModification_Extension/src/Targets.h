#pragma once

#include <kenshi/Dialogue.h>
#include <kenshi/Character.h>

#include <string>
#include <vector>

enum TargetRole
{
	ROLE_SPEAKER, ///< Current line speaker via Dialogue::getSpeaker.
	ROLE_OTHER,	  ///< The other main dialogue side from the resolved line speaker.
	ROLE_SPEAKER_SQUAD, ///< Active platoon members for the resolved line speaker.
	ROLE_OTHER_SQUAD	  ///< Active platoon members for the other main dialogue side.
};

/** @brief Converts a TargetRole to the matching FCS-facing role name. */
std::string TargetRoleToString(TargetRole role);

/**
 * @brief Resolves a Character* from a Dialogue using StatModification target semantics.
 *
 * Speaker uses Kenshi's own line-speaker resolver. Other resolves the opposite
 * dialogue side from that speaker.
 */
Character* ResolveTarget(Dialogue* dlg, DialogLineData* dialogLine, TargetRole role);

/**
 * @brief Resolves one or more Characters from a Dialogue using StatModification target semantics.
 *
 * Single-character roles return one entry. Squad roles resolve the anchor
 * character first, then enumerate that character's active platoon.
 */
std::vector<Character*> ResolveTargets(Dialogue* dlg, DialogLineData* dialogLine, TargetRole role);
