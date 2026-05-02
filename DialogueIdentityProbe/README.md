# DialogueIdentityProbe

Throwaway RE_KENSHI plugin for dialogue identity and target-resolution tests.
It is intentionally separate from `StatModification_Extension`.

## Build

Run:

```bat
DialogueIdentityProbe\build.bat
```

Expected output:

```text
DialogueIdentityProbe\x64\Release\DialogueIdentityProbe.dll
```

Copy/use the mod folder at `DialogueIdentityProbe\DialogueIdentityProbe\` with:

- `fcs.def`
- `RE_Kenshi.json`
- `DialogueIdentityProbe.dll`

## FCS Setup

Create one marker record:

- Type: `DIALOGUE_IDENTITY_PROBE`
- Name: `DIP Marker`

On every test dialogue line, add one effect/action:

- Field: `SM_PROBE_DIALOGUE_IDENTITY`
- Record: `DIP Marker`
- Value: `0`

That one action is deliberately boring: it only means "log every probe method for this line." The checklist below is the self-documenting part that the older multi-action spike used to provide.

## FCS Checklist

Create two parent dialogue/event setups:

- `[ ]` `DIP EV_PLAYER_TALK_TO_ME`
- `[ ]` `DIP EV_I_SEE_NEUTRAL_SQUAD`

`monologue` belongs to the parent `DIALOGUE` record, not to individual `DIALOGUE_LINE` records and not to the `_lines` list. Test it by creating a second parent dialogue with the same child-line checklist and a different `monologue` value.

For each event setup, note the parent dialogue `monologue` flag:

- `[ ]` `EV_PLAYER_TALK_TO_ME` with `monologue = false`
- `[ ]` `EV_PLAYER_TALK_TO_ME` with `monologue = true`
- `[ ]` `EV_I_SEE_NEUTRAL_SQUAD` with `monologue = false`
- `[ ]` `EV_I_SEE_NEUTRAL_SQUAD` with `monologue = true`

`EV_PLAYER_TALK_TO_ME` with parent dialogue `monologue = true` may be odd or unused in normal gameplay, but it is worth testing once because it may change whether the child lines are treated as NPC speech, player reply flow, or target resolution.

Under each event setup, create these speaker-resolution lines:

| Done | Event | Line Text | `main > speaker` | Effect |
| --- | --- | --- | --- | --- |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `SM PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_ME` | `T_ME` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `SM PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_TARGET` | `T_TARGET` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `SM PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_TARGET_IF_PLAYER` | `T_TARGET_IF_PLAYER` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `SM PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_INTERJECTOR1` | `T_INTERJECTOR1` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `SM PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_WHOLE_SQUAD` | `T_WHOLE_SQUAD` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `SM PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_TARGET_WITH_RACE` | `T_TARGET_WITH_RACE` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_ME` | `T_ME` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_TARGET` | `T_TARGET` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_TARGET_IF_PLAYER` | `T_TARGET_IF_PLAYER` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_INTERJECTOR1` | `T_INTERJECTOR1` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_WHOLE_SQUAD` | `T_WHOLE_SQUAD` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_TARGET_WITH_RACE` | `T_TARGET_WITH_RACE` | `SM_PROBE_DIALOGUE_IDENTITY` |

Interjector-specific subtest:

| Done | Event | Line Text | Node Type / Placement | `main > speaker` | Effect |
| --- | --- | --- | --- | --- | --- |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_INTERJECTOR1` | normal dialogue line before/without an interjector node | `T_INTERJECTOR1` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_INTERJECTOR1_AFTER_INTERJECTOR_NODE` | line after an interjector node establishes interjector 1 | `T_INTERJECTOR1` | `SM_PROBE_DIALOGUE_IDENTITY` |

The point is to distinguish "what does `T_INTERJECTOR1` return when no interjector slot has been populated?" from "what does it return after an interjector node has populated that slot?"

If duplicating the full table for both parent-dialogue monologue states is too much, do a focused monologue pass first by creating a separate monologue parent dialogue and adding only these child lines:

| Done | Event | `monologue` | Line Text | `main > speaker` | Effect |
| --- | --- | --- | --- | --- | --- |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `true` | `SM PROBE MONO EV_PLAYER_TALK_TO_ME SPEAKER T_ME` | `T_ME` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `true` | `SM PROBE MONO EV_PLAYER_TALK_TO_ME SPEAKER T_TARGET` | `T_TARGET` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `true` | `SM PROBE MONO EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_ME` | `T_ME` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `true` | `SM PROBE MONO EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_TARGET` | `T_TARGET` | `SM_PROBE_DIALOGUE_IDENTITY` |

Use only two event families first:

- `EV_PLAYER_TALK_TO_ME` for player-invoked talk
- `EV_I_SEE_NEUTRAL_SQUAD` for NPC/world-invoked squad detection

For each event, create one line for each `main > speaker` value:

- `T_ME`
- `T_TARGET`
- `T_TARGET_IF_PLAYER`
- `T_INTERJECTOR1`
- `T_WHOLE_SQUAD`
- `T_TARGET_WITH_RACE`

Use clear line text:

```text
SM PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_ME
SM PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_WHOLE_SQUAD
```

## Race Tests

Kenshi's local `fcs.def` exposes race matching as the dialogue-line reference list `target race`; the line `speaker` field says `T_TARGET_WITH_RACE` returns a target matching that list, or fails. The local headers do not expose a separate `DC_IS_TARGET_RACE` condition enum.

Create:

| Done | Event | Line Text | `main > speaker` | `target race` | Effect |
| --- | --- | --- | --- | --- | --- |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `SM PROBE RACE PLAYER GREENLANDER` | `T_TARGET_WITH_RACE` | `Greenlander` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_PLAYER_TALK_TO_ME` | `SM PROBE RACE PLAYER HIVE PRINCE` | `T_TARGET_WITH_RACE` | `Hive Prince` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE RACE NEUTRAL GREENLANDER` | `T_TARGET_WITH_RACE` | `Greenlander` | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `EV_I_SEE_NEUTRAL_SQUAD` | `SM PROBE RACE NEUTRAL HIVE PRINCE` | `T_TARGET_WITH_RACE` | `Hive Prince` | `SM_PROBE_DIALOGUE_IDENTITY` |

Optional extra: vary `main > speaker` while keeping the same `target race` list.

## Condition Tests

For condition resolution, add normal FCS conditions to lines that also have `SM_PROBE_DIALOGUE_IDENTITY`.

Create one mixed-condition line:

| Done | Line Text | `main > speaker` | Conditions | Effect |
| --- | --- | --- | --- | --- |
| `[ ]` | `SM PROBE CONDITIONS PASS ONLY` | `T_ME` | One condition expected to pass | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `SM PROBE CONDITIONS PASS FAIL` | `T_ME` | One condition expected to pass, one expected to fail | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `SM PROBE CONDITIONS TARGET WHO` | `T_TARGET` | Same condition with `who = T_TARGET` if available | `SM_PROBE_DIALOGUE_IDENTITY` |
| `[ ]` | `SM PROBE CONDITIONS TARGET_WITH_RACE WHO` | `T_TARGET_WITH_RACE` | Same condition with `who = T_TARGET_WITH_RACE` if available | `SM_PROBE_DIALOGUE_IDENTITY` |

Expected behavior to confirm:

- `checkTags` logs and reports `originalResult=0`
- `_doActions` does not log for that line if failed conditions block actions

## Logs To Paste Back

Filter for:

```text
DialogueIdentityProbe:
```

Useful phases:

- `phase=checkTags`
- `phase=conditionRow`
- `phase=conditionReference`
- `phase=originalResult`
- `phase=doActions`
- `phase=context`
- `phase=lineList`
- `phase=speakerProbe`

Each probe includes `probeId`, line data, event enum/int, parent dialogue flags, speaker enum/name, `getSpeaker(...)` results, conversation target, `dlg->me`, character race data, and condition rows.

For how to interpret each log phase, see `LOG_AUDIT.md`.

## Observed Notes

Evidence logs reviewed:

- `RE_Kenshi_log_when_spike_shopkeep.txt`
- `RE_Kenshi_log_when_spike_pacifier.txt`
- `RE_Kenshi_log_when_spike_talk_to_both.txt`

Root-node dialogue can resolve `main > speaker` through `dlg->getSpeaker(...)` without changing who visibly speaks the line. In current shopkeeper/pacifier tests, every visible root line came from the NPC/dialogue owner even when `main > speaker` was set to `T_TARGET`, `T_TARGET_IF_PLAYER`, or `T_TARGET_WITH_RACE`. In FCS authoring terms, treat the first/root dialogue node as often being the dialogue owner's turn to talk.

Treat these as separate questions:

- Runtime identity resolution: use `phase=speakerProbe`, `phase=context`, and `lineSpeakerResolved`.
- Visible speech owner: verify in-game. Root nodes may force or effectively behave as NPC/owner speech.
- Child dialogue nodes are different: in `RE_Kenshi_log_when_child_nodes.txt` plus dialogue-history screenshots, executed child lines generally displayed under the character selected by `lineSpeakerName` / `getSpeaker(lineSpeaker)`, e.g. `T_TARGET_WITH_RACE` displayed as `Barth`, `T_TARGET` / `T_TARGET_IF_PLAYER` displayed as `Fish` or `Dirt`, `T_WHOLE_SQUAD` displayed as the owner side, and populated `T_INTERJECTOR1` displayed as the interjector. In FCS authoring terms, treat the first child node as often being the other side's reply/choice turn.

When line text labels disagree with runtime, trust `event=...`, `lineSpeakerName=...`, and `currentConversation=...` in the log. Some reused lines may still have old `text0` labels.

### Current Findings

`Dialogue::me` / `T_ME`:

- Resolves to the active dialogue/package owner.
- Shopkeeper tests: Barman.
- Pacifier `EV_I_SEE_NEUTRAL_SQUAD`: Pacifier when Pacifier owns the active package.
- Pacifier-style package placed on Thief Boss: Thief Boss for `EV_PLAYER_TALK_TO_ME`.

`dlg->getConversationTarget().getCharacter()` / `T_TARGET`:

- Resolves to the current conversation target or detected target-side character.
- It is not universally "the player"; in `EV_I_SEE_NEUTRAL_SQUAD`, it changes with whichever character/squad member triggered or was selected by the event.

`T_TARGET_IF_PLAYER`:

- In `EV_PLAYER_TALK_TO_ME`, resolved like `T_TARGET` in observed shopkeeper and pacifier-style package tests.
- In `EV_I_SEE_NEUTRAL_SQUAD`, sometimes resolved like `T_TARGET` and sometimes resolved null. Null is expected when the relevant target is not player-controlled; this is the purpose of `T_TARGET_IF_PLAYER`, not evidence that the role is generally unsafe.

`T_WHOLE_SQUAD`:

- Resolved to one character in observed tests, not to an iterable squad.
- In observed runs it resolved to the owner/NPC side. More logs are needed before claiming whether this is always leader, owner, random, or context-dependent.
- For StatModification_Extension action `speaker` targeting, this can still be useful because the action mutates the single resolved character. It should be documented as one selected character, not as squad-wide behavior.
- For true squad-wide operations, use a real squad/platoon API rather than `getSpeaker(T_WHOLE_SQUAD, ...)`.

`T_TARGET_WITH_RACE`:

- Depends on the line `target race` list.
- Can resolve null when the race list is absent or no matching target-side character is available.
- Can resolve a matching target-side/squad character that is not the current `conversationTarget`.
- Examples: in talk-to tests, conversation target could be `Spike` or `Shaku`, while `T_TARGET_WITH_RACE` selected another matching character such as `Thumper`.
- The isolated authored `PACIFIER PROBE EV_PLAYER_TALK_TO_ME SPEAKER T_TARGET_WITH_RACE_HIVE_PRINCE` no-match case did not produce a matching `_doActions` / `phase=context` execution, even after repeated attempts where the matching Greenlander line did execute. Treat target-race mismatch as an execution gate: the line can be evaluated without executing its actions.
- In the same run, `probeId=52-IdentitySpike.mod` produced repeated `checkTags` / `originalResult=1` entries with `target={null}` and some `me` characters of race `Hive Prince`, but no corresponding `_doActions` block. Treat this as candidate evaluation, not executed dialogue.
- In child-choice screenshots, an eligible `T_TARGET_WITH_RACE` / `Greenlander` sibling could execute while non-matching sibling options were not surfaced as ordinary selectable followups. This supports the idea that `target race` participates in routing/eligibility, not just in final speaker naming.

`T_INTERJECTOR1`:

- Resolves null before/without an interjector slot being established.
- After an interjector node, resolves to the interjector character, e.g. `Ninja Guard`, `Guild Plastic Surgeon`, `Shinobi Trader`, or `Thief`.
- Unlimited/blank interjector nodes can appear in the dialogue UI/history as `[...]`. In the child-node pass, those records often had blank `text0` in runtime logs and served as routing/interjection structure rather than authored visible text.

`checkTags` vs `_doActions`:

- `checkTags` logs candidate evaluation and can run many times for lines that are merely considered.
- `_doActions` / `phase=context` is the evidence that a dialogue line actually executed.
- A probe action does not log from `_doActions` when vanilla line resolution/conditions prevent the line from executing. For negative tests, absence of `_doActions` after authored setup is meaningful, especially when nearby `checkTags` lines prove the candidate was considered.
- Current evidence supports: actions fire only after the line passes the relevant vanilla condition/resolution gate. A custom/probe marker being present on the line is not enough to force action execution.

Monologue:

- `monologue` is on the parent `DIALOGUE`, not `DIALOGUE_LINE`.
- Current visible-speech observation came from naturally occurring dialogue variants where shopkeeper and pacifier-style packages had different monologue/non-monologue parent setups. The authored setup is part of the evidence even though the current runtime log does not print the parent `monologue` bool directly.
- The probe now logs parent `DIALOGUE` flags on `phase=context`: `parentForEnemies`, `parentMonologue`, `parentLocked`, and `parentOneAtATime`.
- Observed parent flags in the child-node pass: shopkeep `EV_I_SEE_NEUTRAL_SQUAD` parent had `parentMonologue=1`, shopkeep `EV_PLAYER_TALK_TO_ME` parent had `parentMonologue=0`, pacifier `EV_I_SEE_NEUTRAL_SQUAD` parent had `parentMonologue=0`, and pacifier `EV_PLAYER_TALK_TO_ME` parent had `parentMonologue=1`.
- The inversion is important: the same event type appears with opposite `parentMonologue` values depending on the parent dialogue. Shopkeep talk-to-me is non-monologue while pacifier talk-to-me is monologue; shopkeep neutral-squad is monologue while pacifier neutral-squad is non-monologue.
- Child dialogue nodes still followed resolved speaker under both monologue states. Examples include shopkeep talk-to-me child lines with `parentMonologue=0` resolving target-side roles, and pacifier talk-to-me child lines with `parentMonologue=1` resolving target-side roles.
- Current evidence therefore argues against `parentMonologue` being the main switch for runtime speaker identity. It may affect dialogue flow, UI presentation, or whether a branch behaves like a normal choice/ambient line, but `getSpeaker(...)` still resolves from the authored line speaker.
- The stronger identity split is node placement plus authored speaker/race eligibility: root dialogue nodes behaved owner/NPC-biased in visible presentation, while child dialogue nodes generally followed resolved speaker. A duplicated setup that toggles only `monologue` would still be useful for UI/flow, not because current evidence suggests it controls identity.
