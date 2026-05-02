# DialogueIdentityProbe Findings

Purpose: one compiled evidence file for the dialogue identity spike.

Use this file as the first stop before changing StatModification_Extension targeting or condition behavior. It separates tested evidence from interpretation and points at the logs that support each claim.

For the no-skipping audit artifacts, see:

- `ASSERTIONS.md`
- `LOG_AUDIT.md`
- `LOG_AUDIT_CONTEXTS.csv`
- `LOG_AUDIT_PHASE_COUNTS.csv`

## Evidence Files

- `RE_Kenshi_log_when_child_nodes.txt`: clean child-node pass. Includes parent flags, child choices, interjector nodes, and race-target cases.
- `RE_Kenshi_log_when_parent_nodes.txt`: clean parent/root-node pass. Includes parent flags and root-line role resolution.

Older spike logs are superseded. They were removed from the audit after FCS record errors were found.

Screenshots captured alongside the clean logs are also part of the evidence for visible dialogue presentation, especially the dialogue-history views and FCS child-choice layout:

- Image 1: parent-node-only dialogue history, showing pacifier and shopkeep parent/root dialogue according to the game.
- Image 2: shopkeep child-package dialogue window as seen by the player.
- Image 3: shopkeep in-game dialogue log after exhausting child-package dialogue options; shopkeep `EV_I_SEE_NEUTRAL_SQUAD` continues for a while.
- Image 4: pacifier child-package dialogue window as seen by the player.
- Image 5: pacifier in-game dialogue log after exhausting child-package dialogue options; pacifier `EV_I_SEE_NEUTRAL_SQUAD` continues for a while.

## Short Version

- `dlg->getSpeaker(line->speaker, line, false)` is meaningful runtime speaker resolution.
- Visible in-game speaker presentation is not always the same thing as runtime speaker resolution.
- Root dialogue nodes were owner/NPC-presented in the clean visible pass, even when runtime speaker resolution selected target-side characters or interjectors.
- Child dialogue nodes in FCS usually display closer to the resolved runtime speaker.
- `parentMonologue` is not the main switch for runtime speaker identity. It may affect dialogue flow/UI.
- In the tested UI flows, both monologue states still allowed dialogue windows, choices, runtime speaker resolution, and action execution.
- `T_TARGET_WITH_RACE` uses the line `target race` list. For child nodes, a no-match race prevents execution. For root/parent nodes, the line still executes and `getSpeaker` falls back to the current conversation target when no race match is found.
- `checkTags` means "candidate considered"; `_doActions` / `phase=context` means "line executed".

## Root vs Child Dialogue Nodes

### Finding

Root dialogue nodes can visibly appear as the dialogue owner/NPC even when runtime speaker resolution can select a target-side character. Child dialogue nodes generally display under the resolved speaker.

### Evidence

Root-node behavior was observed in the clean parent-node screenshots and `RE_Kenshi_log_when_parent_nodes.txt`: visible root lines were owner/NPC-presented, while runtime `lineSpeakerName` and `getSpeaker(lineSpeaker)` still resolved authored roles.

Representative parent/root log lines:

- `RE_Kenshi_log_when_parent_nodes.txt:11919`: shopkeep talk-to-me root, `lineSpeakerName=T_TARGET_WITH_RACE`, resolved `#140805player1`, while visible parent/root dialogue was NPC-presented.
- `RE_Kenshi_log_when_parent_nodes.txt:25665`: shopkeep talk-to-me root, `lineSpeakerName=T_TARGET_IF_PLAYER`, resolved `#140805player5`.
- `RE_Kenshi_log_when_parent_nodes.txt:62856`: pacifier talk-to-me root, `lineSpeakerName=T_TARGET_WITH_RACE`, resolved `#140805player1`.
- `RE_Kenshi_log_when_parent_nodes.txt:987`: shopkeep neutral-squad root, `lineSpeakerName=T_INTERJECTOR1`, resolved `Ninja Guard`.
- `RE_Kenshi_log_when_parent_nodes.txt:44243`: pacifier neutral-squad root, `lineSpeakerName=T_INTERJECTOR1`, resolved `Shinobi Trader`.

Child-node behavior was observed in `RE_Kenshi_log_when_child_nodes.txt` and matching dialogue-history screenshots:

- `T_TARGET_WITH_RACE_GREENLANDER` displayed under target-side characters such as `#140805player1`.
- `T_TARGET` / `T_TARGET_IF_PLAYER` displayed under target-side characters such as `#140805player5`.
- `T_WHOLE_SQUAD` displayed owner-side in observed cases.
- Populated `T_INTERJECTOR1` displayed under the interjector in the parent/root pass, such as `Ninja Guard`, `Shinobi Trader`, or `Thief`.

Representative log lines:

- `RE_Kenshi_log_when_child_nodes.txt:2407`: shopkeep child, `parentMonologue=0`, `lineSpeakerName=T_TARGET_WITH_RACE`, resolved `#140805player1`.
- `RE_Kenshi_log_when_child_nodes.txt:11285`: shopkeep child, `parentMonologue=0`, `lineSpeakerName=T_TARGET_IF_PLAYER`, resolved `#140805player5`.
- `RE_Kenshi_log_when_child_nodes.txt:18253`: pacifier child, `parentMonologue=1`, `lineSpeakerName=T_TARGET_WITH_RACE`, resolved `#140805player1`.
- `RE_Kenshi_log_when_child_nodes.txt:20067`: pacifier child, `parentMonologue=1`, `lineSpeakerName=T_TARGET`, resolved `#140805player5`.

Note: Several probes that fired within the child-log session (on continuation/interjector-path nodes after the initial dialogue trigger) logged `event=EV_NONE` and `parentForEnemies=unavailable`. This was observed for probes `29`, `32`, `27`, `26`, `28`, `30`, `31`, `17`, and `34`. Speaker resolution still worked normally in those entries. `EV_NONE` here reflects that the original triggering event is no longer in scope at those dialogue nodes, not that the line failed.

### Interpretation

For FCS authors, a practical model is:

- The first/root dialogue node often behaves like the dialogue owner's turn.
- The first child dialogue node often behaves like the other side's reply/choice turn.

This is presentation guidance, not a replacement for runtime identity. Plugins should use runtime identity when mutating game state.

### Confidence

High. The clean parent/root pass confirms visible owner/NPC presentation while runtime speaker roles still resolve independently. The clean child-node pass confirms child choices usually present closer to runtime speaker resolution.

## Parent Monologue

### Finding

`parentMonologue` does not control the runtime speaker identity or targeting model we care about for StatModification_Extension. It is best treated as a parent-dialogue presentation/flow flag.

### Evidence

The clean child and parent passes deliberately logged parent flags and used inverted monologue setups:

| Parent setup | Event | `parentMonologue` | Representative evidence |
| --- | --- | --- | --- |
| Shopkeep talk-to-me | `EV_PLAYER_TALK_TO_ME` | `0` | `RE_Kenshi_log_when_child_nodes.txt:538`, `2407`; `RE_Kenshi_log_when_parent_nodes.txt:11919`, `25665` |
| Pacifier talk-to-me | `EV_PLAYER_TALK_TO_ME` | `1` | `RE_Kenshi_log_when_child_nodes.txt:18125`, `18253`; `RE_Kenshi_log_when_parent_nodes.txt:62856`, `86921` |
| Shopkeep neutral-squad | `EV_I_SEE_NEUTRAL_SQUAD` | `1` | `RE_Kenshi_log_when_child_nodes.txt:120`, `15476`; `RE_Kenshi_log_when_parent_nodes.txt:6077`, `10063` |
| Pacifier neutral-squad | `EV_I_SEE_NEUTRAL_SQUAD` | `0` | `RE_Kenshi_log_when_child_nodes.txt:43`, `17453`; `RE_Kenshi_log_when_parent_nodes.txt:63`, `227` |

This inversion matters. The same event type exists with both monologue states depending on the parent dialogue.

Child speaker resolution worked under both states:

- Shopkeep talk-to-me with `parentMonologue=0` resolved target-side child speakers, e.g. `T_TARGET_IF_PLAYER` to `#140805player5` at `RE_Kenshi_log_when_child_nodes.txt:11285`.
- Pacifier talk-to-me with `parentMonologue=1` resolved target-side child speakers, e.g. `T_TARGET_WITH_RACE` to `#140805player1` at `RE_Kenshi_log_when_child_nodes.txt:18253` and `T_TARGET` to `#140805player5` at `20067`.
- Parent/root lines also resolved target-side and interjector roles under both monologue states, even though the clean screenshots show visible parent/root dialogue as NPC-presented.

UI flow was also exercised under both monologue states:

- Shopkeep talk-to-me child package opened a normal dialogue window with player choices while `parentMonologue=0`.
- Pacifier talk-to-me child package opened a normal dialogue window with player choices while `parentMonologue=1`.
- Shopkeep neutral-squad parent/root dialogue continued in the in-game dialogue log while `parentMonologue=1`.
- Pacifier neutral-squad parent/root dialogue continued in the in-game dialogue log while `parentMonologue=0`, although the user observed fewer/rarer thief `I_SEE_NEUTRAL` outputs and some dialogue appearing in the box/log without obvious speech bubbles above Thief Boss.

### Interpretation

The logs argue against this rule:

```text
monologue decides who the speaker is
```

The better rule is:

```text
line speaker decides runtime identity; monologue may influence UI/flow
```

Practical StatModification rule:

```text
same line speaker + same resolved dialogue context => same target identity model
regardless of parentMonologue
```

In the tested matrix, `parentMonologue` did not change:

- `T_ME` staying the active dialogue/package owner
- `T_TARGET` resolving the current target-side character
- `T_TARGET_IF_PLAYER` depending on player-controlled target-side context
- `T_TARGET_WITH_RACE` using the line `target race` list
- `T_WHOLE_SQUAD` resolving one selected character through `getSpeaker(...)`
- `T_INTERJECTOR1` depending on interjector-slot setup
- `checkTags` being candidate evaluation and `_doActions` being execution evidence

### Remaining Test

Only needed if we want the exact engine-level definition of monologue: duplicate the same parent dialogue and child lines, then toggle only `monologue`. Current evidence is strong enough for the tested UI flow, targeting, and identity conclusions.

### Confidence

High that monologue is out-of-band for runtime speaker identity and StatModification targeting. High that both tested monologue states can still produce player dialogue windows/choices and executed actions. Medium on the exact positive engine-level meaning of monologue.

## `Dialogue::me` / `T_ME`

### Finding

`Dialogue::me` and `T_ME` resolve to the active dialogue/package owner.

### Evidence

- Shopkeep dialogue resolved owner as `Barman`, e.g. `RE_Kenshi_log_when_child_nodes.txt:120` and `RE_Kenshi_log_when_parent_nodes.txt:10063`.
- Pacifier-owned neutral-squad dialogue resolved owner as `Pacifier`, e.g. `RE_Kenshi_log_when_child_nodes.txt:43` and `RE_Kenshi_log_when_parent_nodes.txt:227`.
- Pacifier-style package placed on Thief Boss resolved owner as `Thief Boss`, e.g. `RE_Kenshi_log_when_child_nodes.txt:18125` and `RE_Kenshi_log_when_parent_nodes.txt:98004`.

### Confidence

High.

## `T_TARGET`

### Finding

`T_TARGET` resolves to the current conversation target or detected target-side character. It is not always the player.

### Evidence

- Shopkeep neutral-squad `T_TARGET` resolved `#140805player3`, e.g. `RE_Kenshi_log_when_parent_nodes.txt:9291` (probe `27-IdentitySpike`).
- Pacifier neutral-squad `T_TARGET` resolved `#140805player3`, e.g. `RE_Kenshi_log_when_parent_nodes.txt:179` (probe `49-IdentitySpike`).
- Shopkeep neutral-squad child-session `T_TARGET` resolved `Twitchy Bar Thug` under `EV_NONE`, e.g. `RE_Kenshi_log_when_child_nodes.txt:15388` (probe `27-IdentitySpike`).
- Pacifier talk-to-me child `T_TARGET` resolved `#140805player5`, e.g. `RE_Kenshi_log_when_child_nodes.txt:20067`.

### Confidence

High.

## `T_TARGET_IF_PLAYER`

### Finding

`T_TARGET_IF_PLAYER` resolves like target when the relevant target is player-controlled. It can resolve null when the relevant target is not player-controlled.

### Evidence

- Shopkeep talk-to-me child `T_TARGET_IF_PLAYER` resolved `#140805player5`, e.g. `RE_Kenshi_log_when_child_nodes.txt:11285`.
- Shopkeep talk-to-me parent/root `T_TARGET_IF_PLAYER` resolved `#140805player5`, e.g. `RE_Kenshi_log_when_parent_nodes.txt:25665`.
- Neutral-squad parent/root `T_TARGET_IF_PLAYER` resolved null when the relevant target was not player-controlled, e.g. `RE_Kenshi_log_when_parent_nodes.txt:145` and `9677`.

### Interpretation

Null in non-player contexts is the intended meaning of "if player", not evidence that the role is unsafe. It is useful when authors only want player-side targets.

### Confidence

High.

## `T_WHOLE_SQUAD`

### Finding

Through `getSpeaker(...)`, `T_WHOLE_SQUAD` resolves one character, not an iterable squad. In observed cases it resolved owner/NPC-side.

### Evidence

- Shopkeep talk-to-me child `T_WHOLE_SQUAD` resolved `Barman`, e.g. `RE_Kenshi_log_when_child_nodes.txt:9040`.
- Pacifier talk-to-me child `T_WHOLE_SQUAD` resolved `Thief Boss`, e.g. `RE_Kenshi_log_when_child_nodes.txt:19591`.
- Parent/root `T_WHOLE_SQUAD` rows resolved owner-side in observed runs, e.g. `RE_Kenshi_log_when_parent_nodes.txt:43`, `8840`, `18730`, `75225`.

### Interpretation

For StatModification_Extension, this is not true squad-wide support. It can still mutate the one Kenshi-selected character if used through speaker resolution. Document it as "one selected character", not "every squad member".

### Remaining Test

If true squad behavior is ever desired, use a real squad/platoon API and test aggregation separately.

### Confidence

High that it is not iterable through `getSpeaker(...)`. Medium on the exact selection rule for the one character.

## `T_INTERJECTOR1`

### Finding

`T_INTERJECTOR1` is null until an interjector slot is established. After an interjector node establishes the slot, it can resolve to the interjector character.

### Evidence

- Child talk-to-me interjector setup/probe rows resolved null before a populated slot, e.g. `RE_Kenshi_log_when_child_nodes.txt:652`, `4905`, `18695`, `19185`.
- Parent/root after-interjector rows resolved populated interjectors, e.g. `Ninja Guard` at `RE_Kenshi_log_when_parent_nodes.txt:987` and `4275`, `Shinobi Trader` at `44243`, and `Thief` at `49316`.

### Confidence

High for `T_INTERJECTOR1`. Untested for `T_INTERJECTOR2` and `T_INTERJECTOR3`.

## `T_TARGET_WITH_RACE`

### Finding

`T_TARGET_WITH_RACE` uses the line's `target race` list to scan for a matching character. It can select a matching target-side/squad character that is not the current `conversationTarget`.

Race-match behavior differs between root/parent and child dialogue nodes:

- **Child nodes**: if no character in the area matches the race list, the line is considered in `checkTags` but `doActions` / `phase=context` does not fire. The line does not execute.
- **Root/parent nodes**: if no character matches the race list, the line still executes. `getSpeaker(T_TARGET_WITH_RACE)` falls back to the current `conversationTarget` even when that character's race does not match the authored list.

### Evidence

Child matching — T_TARGET_WITH_RACE selecting a different character than convTarget:

- Shopkeep child `T_TARGET_WITH_RACE_GREENLANDER` resolved `#140805player1` while conversation target was `#140805player5`, e.g. `RE_Kenshi_log_when_child_nodes.txt:2407`.
- Pacifier child `T_TARGET_WITH_RACE_GREENLANDER` resolved `#140805player1` while conversation target was `#140805player5`, e.g. `RE_Kenshi_log_when_child_nodes.txt:18253`.

Root/parent matching — T_TARGET_WITH_RACE selecting a race-matching character:

- Parent/root `T_TARGET_WITH_RACE_GREENLANDER` resolved `#140805player1` (Greenlander, different from convTarget `#140805player3`), e.g. `RE_Kenshi_log_when_parent_nodes.txt:103`.
- Shopkeep neutral-squad root `T_TARGET_WITH_RACE_GREENLANDER` resolved `Twitchy Bar Thug` (Greenlander) while convTarget was also `Bar Thug` (Greenlander), e.g. `RE_Kenshi_log_when_parent_nodes.txt:6077`.
- Shopkeep talk-to-me root `T_TARGET_WITH_RACE_GREENLANDER` resolved `#140805player1` (Greenlander), e.g. `RE_Kenshi_log_when_parent_nodes.txt:11919`.
- Pacifier talk-to-me root `T_TARGET_WITH_RACE_GREENLANDER` resolved `#140805player1` (Greenlander), e.g. `RE_Kenshi_log_when_parent_nodes.txt:62856`.

Root/parent no-match fallback — T_TARGET_WITH_RACE executing despite race mismatch:

- Pacifier neutral-squad root `T_TARGET_WITH_RACE_GREENLANDER` (authored race: Greenlander) executed at `RE_Kenshi_log_when_parent_nodes.txt:63`; `getSpeaker` resolved `Hep` (Scorchlander) and `conversationTarget` was also `Hep` (Scorchlander). No Greenlander was present at that moment. The line executed and `getSpeaker` returned the current convTarget as a fallback.

Child no-match — T_TARGET_WITH_RACE considered but not executed:

- Pacifier `CHILD PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_TARGET_WITH_RACE_HIVE_PRINCE` (probe `56-IdentitySpike`) appeared in `checkTags` with `originalResult=1` repeatedly (e.g. `RE_Kenshi_log_when_child_nodes.txt:61`, `17471`) but produced no `doActions` / `phase=context` in either log. No Hive Prince was present in those Pacifier sessions.
- When a real Hive Prince target was present, the Hive Prince line executed: shopkeep neutral-squad `T_TARGET_WITH_RACE_HIVE_PRINCE` resolved `de Zerka` (Hive Prince) at `RE_Kenshi_log_when_child_nodes.txt:15543`.

### Interpretation

`target race` participates in eligibility/routing. It is not just a cosmetic speaker label.

For child dialogue nodes, a race-mismatched line is gated out and does not execute. For root/parent dialogue nodes, the line executes regardless and `getSpeaker(T_TARGET_WITH_RACE)` falls back to the current conversation target when no race match is found.

### Confidence

High for child no-match gating and Hive Prince positive-match. High for root/parent fallback behavior based on the Hep (Scorchlander) observation at `RE_Kenshi_log_when_parent_nodes.txt:63`.

## `checkTags` vs `_doActions`

### Finding

`checkTags` is candidate evaluation and can be noisy. `_doActions` / `phase=context` is evidence that a dialogue line actually executed.

### Evidence

- Pacifier `CHILD PROBE EV_I_SEE_NEUTRAL_SQUAD SPEAKER T_TARGET_WITH_RACE_HIVE_PRINCE` (probe `56-IdentitySpike`) appeared in `checkTags` with `originalResult=1` multiple times in `RE_Kenshi_log_when_child_nodes.txt` (e.g. L61, L68, L17471, L17478) but produced no `doActions` / `phase=context` in either log.
- The matching Greenlander sibling line (probe `45-IdentitySpike`) did execute in the same child session, e.g. `RE_Kenshi_log_when_child_nodes.txt:18253`.
- A Hive Prince line did execute when an eligible Hive Prince target existed: `RE_Kenshi_log_when_child_nodes.txt:15543`.
- Parent/root lines often produced many repeated `checkTags` entries before or without a corresponding action execution.

### Interpretation

For negative tests, absence of `_doActions` is meaningful when:

- the authored line was present,
- nearby `checkTags` proves the line was considered,
- comparable sibling/positive cases executed.

`checkTags originalResult=1` means tag conditions passed for that candidate evaluation. It does not prove later line selection, speaker resolution, race eligibility, or branch routing allowed the line to execute.

### Confidence

High.

## StatModification_Extension Implications

- Use `dlg->getSpeaker(dialogLine->speaker, dialogLine, false)` for speaker-targeted actions.
- Keep public API speaker-first: plain actions affect resolved line speaker; `other` actions affect the other main conversation side.
- Document that visible root-node speech can mislead authors. Runtime speaker is what the action follows.
- Do not treat `T_WHOLE_SQUAD` as squad-wide mutation.
- Do not use `parentMonologue` to decide identity.
- Conditions still need their own targeted tests because condition `who` evaluation occurs through `checkTags`, not `_doActions`.

## Remaining Tests Before Calling This Fully Settled

1. Run actual StatModification_Extension actions after the public API rename on both root and child dialogue nodes.
2. Test custom StatModification comparison conditions for `who = T_ME`, `T_TARGET`, `T_TARGET_IF_PLAYER`, `T_WHOLE_SQUAD`, `T_INTERJECTOR1`, and `T_TARGET_WITH_RACE`.
3. Add one mixed-condition line where our condition passes, a vanilla or other-plugin condition fails, and our action is present. Expected: action does not execute.
4. If desired, add `T_INTERJECTOR2` and `T_INTERJECTOR3` tests for squad-banter cases.
