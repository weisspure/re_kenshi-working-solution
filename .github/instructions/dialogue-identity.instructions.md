---
applyTo: "StatModification_Extension/src/Targets.*,StatModification_Extension/src/Actions.*,StatModification_Extension/src/Conditions.*,wiki/For-Mod-Authors.md,wiki/For-Plugin-Authors.md,DialogueIdentityProbe/src/**,DialogueIdentityProbe/ASSERTIONS.md,DialogueIdentityProbe/FINDINGS.md"
---
# Dialogue Identity Findings

Use when changing action targeting, condition participants, or mod-author targeting docs.

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
- `T_TARGET_WITH_RACE`: uses the line `target race` list. For **child nodes**, no matching character prevents the line from executing. For **root/parent nodes**, the line executes regardless and `getSpeaker` falls back to the current `conversationTarget` when no race match is found.
- `T_INTERJECTOR1`: null until an interjector node populates the slot, then resolves to that interjector.
- `T_WHOLE_SQUAD`: observed as one Kenshi-selected character, often owner/NPC side; never squad-wide iteration on this API path.

## UI vs Runtime
- Root dialogue lines were visually owner/NPC-presented even when runtime speaker resolved target-side roles.
- Child dialogue nodes generally displayed under the resolved speaker.
- Document root and child behavior as complementary FCS behavior; do not present one as the only rule.
- Mod-author wording: first/root line often looks like owner turn; child options often look like other side turn.

## Monologue
- Do not use parent `monologue` as targeting rule.
- Opposite monologue states existed across shopkeep/pacifier tests while runtime role resolution still worked.
- Tested flows: monologue did not block dialogue windows, choices, runtime speaker resolution, or action execution.

## Condition Evidence
- `_doActions` proves line executed.
- `checkTags` proves line considered for conditions; it can pass and still not execute due to later selection/routing/speaker/race gates.
- Race no-match rows in `checkTags` without `_doActions` are meaningful negative evidence only when authored line is known and isolated.
