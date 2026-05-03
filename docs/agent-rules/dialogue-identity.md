# Dialogue Identity Findings

Use for: action targeting, condition participants, and mod-author targeting docs.

## Tested Evidence
- Probe project: `DialogueIdentityProbe/`; keep spike-only hooks/data out of `StatModification_Extension`.
- Tested events: `EV_PLAYER_TALK_TO_ME` and `EV_I_SEE_NEUTRAL_SQUAD`.
- Tested layouts: root dialogue lines and one-node-down child dialogue nodes.
- Evidence: `DialogueIdentityProbe/FINDINGS.md`, `LOG_AUDIT.md`, regenerated `RE_Kenshi_log_when_*` logs.

## Runtime Roles
- `Dialogue::me` = active dialogue/package owner.
- `dlg->getConversationTarget().getCharacter()` = current event/conversation target; not always player.
- `dlg->getSpeaker(line->speaker, line, false)` = runtime source for FCS `speaker` dropdown.
- Actions follow runtime speaker resolution, not visible dialogue UI name only.

## Speaker Dropdown Semantics
- `T_ME`: owner side.
- `T_TARGET`: current target side; can be a non-player character in NPC-initiated events.
- `T_TARGET_IF_PLAYER`: resolves when the relevant target is player-controlled; can resolve null otherwise.
- `T_TARGET_WITH_RACE`: uses line `target race` list.
  - child nodes: no matching character prevents execution.
  - root/parent nodes: line still executes; `getSpeaker` falls back to current `conversationTarget` when no race match exists.
- `T_INTERJECTOR1`: null until an interjector node populates the slot, then resolves to that interjector.
- `T_WHOLE_SQUAD`: one Kenshi-selected character on this API path; not squad-wide iteration.

## UI vs Runtime
- Root dialogue lines can appear owner/NPC-presented even when runtime speaker resolves target-side roles.
- Child dialogue nodes usually display under resolved speaker.
- Document root and child behavior as complementary; do not present one as universal rule.

## Monologue
- Do not use parent `monologue` as targeting rule.
- Tested flows showed monologue state did not block dialogue window, choice flow, speaker resolution, or action execution in this matrix.

## Condition Evidence
- `_doActions` proves line executed.
- `checkTags` proves line considered for conditions; it can pass and still not execute due to later selection/routing/speaker/race gates.
- Race no-match rows in `checkTags` without `_doActions` are meaningful negative evidence only when authored line is known/isolated.
