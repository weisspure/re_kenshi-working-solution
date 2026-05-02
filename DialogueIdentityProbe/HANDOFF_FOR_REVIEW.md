# DialogueIdentityProbe Handoff For Review

Purpose: give another AI or reviewer enough context to double-check the Kenshi dialogue identity findings without needing the whole chat history.

## Project

`DialogueIdentityProbe` is a throwaway RE_Kenshi proof-of-concept plugin, separate from `StatModification_Extension`.

It hooks:

- `Dialogue::_doActions`
- `DialogLineData::checkTags`

The FCS marker action is:

- `SM_PROBE_DIALOGUE_IDENTITY`

When a marked line executes, the plugin logs `phase=context`, `phase=lineList`, and `phase=speakerProbe` rows. `checkTags` also logs candidate evaluation for marked lines.

## Files To Read First

- `DialogueIdentityProbe/FINDINGS.md`
- `DialogueIdentityProbe/ASSERTIONS.md`
- `DialogueIdentityProbe/LOG_AUDIT.md`
- `DialogueIdentityProbe/README.md`
- `.github/instructions/re-kenshi-runtime.instructions.md`
- `DialogueIdentityProbe/src/DialogueIdentityProbe.cpp`

FCS/runtime schema files:

- `DialogueIdentityProbe/DialogueIdentityProbe/fcs.def`
- `DialogueIdentityProbe/DialogueIdentityProbe/RE_Kenshi.json`

Evidence logs:

- `DialogueIdentityProbe/RE_Kenshi_log_when_child_nodes.txt`
- `DialogueIdentityProbe/RE_Kenshi_log_when_parent_nodes.txt`

Older spike logs are superseded. Do not use them as primary evidence.

Older interim files that should be treated as superseded:

- `StatModification_Extension/SPIKE_TEST_RECORDS.md`
- `StatModification_Extension/IdentitySpike/fcs.def`
- spike-only `StatModification_Extension` runtime logging
- older `RE_Kenshi_log_when_spike_*.txt` logs

## What The Probe Logs

For executed lines:

- event enum/int
- parent dialogue flags: `parentForEnemies`, `parentMonologue`, `parentLocked`, `parentOneAtATime`
- line `main > speaker` enum/name
- `dlg->getSpeaker(line->speaker, line, false)`
- speaker probes for `T_ME`, `T_TARGET`, `T_TARGET_IF_PLAYER`, `T_INTERJECTOR1`, `T_WHOLE_SQUAD`, `T_TARGET_WITH_RACE`
- `dlg->getConversationTarget().getCharacter()`
- `dlg->me`
- current dialogue/current line when available
- race data for resolved characters when available
- target race lists and condition references

For candidate evaluation:

- `checkTags` `me` and `target`
- condition references/rows
- `originalResult`

## Main Findings To Verify

### Root vs Child Node Presentation

Root-node pass:

- The clean parent/root screenshot shows both pacifier and shopkeep root dialogue entries presented under the NPC/dialogue owner in the game dialogue log.
- Runtime `getSpeaker(line->speaker, ...)` still resolved authored speaker roles on those root lines, including target-side characters and interjectors.
- Examples: `RE_Kenshi_log_when_parent_nodes.txt:11919` resolved `T_TARGET_WITH_RACE` to `#140805player1`, `25665` resolved `T_TARGET_IF_PLAYER` to `#140805player5`, and `987` resolved `T_INTERJECTOR1` to `Ninja Guard`.
- Interpretation: root-node visible presentation is owner/NPC-biased or special-cased. In FCS authoring terms, the first/root dialogue node often behaves like the dialogue owner's turn to talk.

Child-node pass:

Clean screenshot context:

- Image 1 shows the parent-node-only dialogue history, with pacifier and shopkeep parent/root dialogue according to the game.
- Image 2 shows the shopkeep child-package dialogue window as seen by the player.
- Image 3 shows the in-game dialogue log after exhausting shopkeep child options; shopkeep `EV_I_SEE_NEUTRAL_SQUAD` continues for a while.
- Image 4 shows the pacifier child-package dialogue window as seen by the player.
- Image 5 shows the in-game dialogue log after exhausting pacifier child options; pacifier `EV_I_SEE_NEUTRAL_SQUAD` continues for a while.

Child text nodes behaved more in line with speaker resolution. Dialogue history/screenshots showed child lines under the resolved speaker:

  - `T_TARGET_WITH_RACE_GREENLANDER` displayed as a matching target-side character, e.g. `#140805player1`.
  - `T_TARGET` / `T_TARGET_IF_PLAYER` displayed as target-side characters, e.g. `#140805player5`.
  - `T_WHOLE_SQUAD` displayed as owner side, e.g. `Barman` / `Thief Boss`.
  - populated `T_INTERJECTOR1` displayed as the interjector in parent/root after-interjector rows, e.g. `Ninja Guard`, `Shinobi Trader`, or `Thief`.
- Working rule: `main > speaker` is meaningful for runtime speaker resolution, but visible presentation depends on node position. Root nodes are special; child dialogue nodes generally follow resolved speaker and often behave like the other side's reply/choice turn.

### Role Resolution

- `Dialogue::me` / `T_ME`: active dialogue/package owner.
- `dlg->getConversationTarget().getCharacter()` / `T_TARGET`: current conversation target or detected target-side character, not always the player.
- `T_TARGET_IF_PLAYER`: resolves like target when the relevant target is player-controlled; can resolve null when not player-controlled.
- `T_WHOLE_SQUAD`: resolves to one selected character, not an iterable squad. In observed tests it resolved to owner/NPC side.
- `T_INTERJECTOR1`: null before/without an interjector slot; resolves to the interjector after an interjector node establishes one.
- `T_TARGET_WITH_RACE`: uses the line `target race` list and may select a matching target-side/squad character that is not the current `conversationTarget`.
- In child-choice layouts, evidence supports that an eligible `target race` branch can execute while non-matching sibling branches are not surfaced as ordinary selectable followups.

### Race / Negative Action Results

Authored no-match tests used Hive Prince target race while no matching target-side character existed in the player talk-to-me child-choice setup.

Important example:

- `SHOPKEEP CHILD PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_TARGET_WITH_RACE_HIVE_PRINCE`
- `PACIFIER CHILD PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_TARGET_WITH_RACE_HIVE_PRINCE`

Observed:

- The Hive Prince no-match lines appeared in `checkTags` / candidate evaluation.
- They did not produce matching `_doActions` / `phase=context` execution when no eligible target-side character existed.
- Nearby matching Greenlander lines did execute.
- A positive Hive Prince match also exists in the clean logs: shopkeep neutral-squad `T_TARGET_WITH_RACE_HIVE_PRINCE` resolved `de Zerka` at `RE_Kenshi_log_when_child_nodes.txt:15543`.

Interpretation to verify:

- Target-race mismatch acts as an execution/eligibility gate.
- A probe marker/action on a line is not enough to force `_doActions`; vanilla line selection/resolution must pass first.

### checkTags vs _doActions

- `checkTags` is noisy candidate evaluation.
- `_doActions` / `phase=context` is evidence that a line actually executed.
- If an authored probe line appears in `checkTags` but not `_doActions`, that is meaningful negative evidence, not automatically an untested case.

### Monologue

- `monologue` is on parent `DIALOGUE`, not `DIALOGUE_LINE`.
- Probe now logs `parentMonologue`.
- Observed parent states in the child-node pass:
  - shopkeep `EV_I_SEE_NEUTRAL_SQUAD`: `parentMonologue=1`
  - shopkeep `EV_PLAYER_TALK_TO_ME`: `parentMonologue=0`
  - pacifier `EV_I_SEE_NEUTRAL_SQUAD`: `parentMonologue=0`
  - pacifier `EV_PLAYER_TALK_TO_ME`: `parentMonologue=1`

Monologue may affect UI/dialogue flow, but the strongest current visible-speaker split is root node vs child text node.
The inversion is important evidence: shopkeep talk-to-me is non-monologue while pacifier talk-to-me is monologue; shopkeep neutral-squad is monologue while pacifier neutral-squad is non-monologue.
Child and parent/root runtime speaker roles resolved under both monologue states, so current evidence argues against `parentMonologue` being the main switch for runtime speaker identity.
Clean screenshots show the dialogue window itself still opens and offers choices under both pacifier/shopkeep monologue states. The tested UI flows are enough to conclude that `parentMonologue` did not block dialogue windows, choices, runtime speaker resolution, or action execution in this matrix. Treat `parentMonologue` as a dialogue-flow/UI variable, but do not use it to explain away `getSpeaker(...)` identity results.

### Blank / Interjector UI Entries

- Some UI/history entries displayed as `[...]`.
- These correlate with blank `text0=""` records and/or unlimited/interjector routing structure.
- Treat them as routing/interjection structure unless more evidence shows otherwise.

## Questions For Reviewer

Please verify or challenge:

- Is root-node owner/NPC presentation a known Kenshi dialogue behavior?
- Is child text-node presentation expected to follow `main > speaker` / `getSpeaker(lineSpeaker)`?
- Does `target race` for `T_TARGET_WITH_RACE` act as a hard eligibility gate before `_doActions`?
- Does `checkTags originalResult=1` with no `_doActions` imply the line passed tag conditions but failed later line selection/speaker/race resolution?
- Is `T_WHOLE_SQUAD` expected to resolve one character through `getSpeaker`, and under what rules?
- Are `[...]` entries expected for blank/unlimited interjector nodes?

## Prompt For External AI

```text
Please review the Kenshi dialogue identity proof-of-concept in this repo.

Start with:
- DialogueIdentityProbe/HANDOFF_FOR_REVIEW.md
- DialogueIdentityProbe/README.md
- .github/instructions/re-kenshi-runtime.instructions.md
- DialogueIdentityProbe/src/DialogueIdentityProbe.cpp

Then inspect logs:
- DialogueIdentityProbe/RE_Kenshi_log_when_child_nodes.txt
- DialogueIdentityProbe/RE_Kenshi_log_when_parent_nodes.txt

Goal: confirm or challenge the findings about Kenshi dialogue identity resolution:
- root nodes appear owner/NPC-presented even when speaker resolves target-side
- child text nodes generally display as the resolved speaker
- T_ME, T_TARGET, T_TARGET_IF_PLAYER, T_WHOLE_SQUAD, T_INTERJECTOR1, and T_TARGET_WITH_RACE semantics
- target-race no-match lines can be considered in checkTags but not execute _doActions
- parent monologue effects vs root/child node effects

Please distinguish tested evidence from plausible interpretation, and suggest the smallest additional test that would falsify the current working rules.
```
