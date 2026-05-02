# DialogueIdentityProbe Observation Matrix

This is not a normal automated test suite.

The clean spike combines:

- runtime logs from `RE_Kenshi_log_when_child_nodes.txt`
- runtime logs from `RE_Kenshi_log_when_parent_nodes.txt`
- screenshots/user observations from the same clean run

Use this file as a manual observation matrix. `phase=context` proves a marked line reached `_doActions`. Screenshots/user observations prove visible UI presentation and dialogue flow.

Generated audit files:

- `LOG_AUDIT_CONTEXTS.csv`: one executed `phase=context` row per marked line
- `LOG_AUDIT_PHASE_COUNTS.csv`: phase counts per probe id/text, useful for considered-but-not-executed lines

Only the regenerated parent/child logs are primary evidence. Older spike logs are superseded.

## A. Clean Evidence Set

### A1 - What logs are in scope?

Observed:

| Log | Executed `phase=context` rows |
| --- | ---: |
| `RE_Kenshi_log_when_child_nodes.txt` | 80 |
| `RE_Kenshi_log_when_parent_nodes.txt` | 24 |

Inference:

The current audit uses 104 executed context rows from the clean parent/child pass.

Confidence: High.

### A2 - What screenshots are in scope?

Observed:

- Image 1: parent-node-only dialogue history, showing pacifier and shopkeep parent/root dialogue according to the game.
- Image 2: shopkeep child-package dialogue window as seen by the player.
- Image 3: shopkeep in-game dialogue log after exhausting child-package dialogue options; shopkeep `EV_I_SEE_NEUTRAL_SQUAD` continues for a while.
- Image 4: pacifier child-package dialogue window as seen by the player.
- Image 5: pacifier in-game dialogue log after exhausting child-package dialogue options; pacifier `EV_I_SEE_NEUTRAL_SQUAD` continues for a while.

Inference:

The screenshots are the UI/flow evidence. The logs are the runtime identity/execution evidence.

Confidence: High.

## B. Parent Monologue

### B1 - Does event type imply monologue?

Observed:

| Parent setup | Event | `parentMonologue` | Evidence |
| --- | --- | --- | --- |
| Shopkeep talk-to-me | `EV_PLAYER_TALK_TO_ME` | `0` | `RE_Kenshi_log_when_child_nodes.txt:538`, `2407`; `RE_Kenshi_log_when_parent_nodes.txt:11919`, `25665` |
| Pacifier talk-to-me | `EV_PLAYER_TALK_TO_ME` | `1` | `RE_Kenshi_log_when_child_nodes.txt:18125`, `18253`; `RE_Kenshi_log_when_parent_nodes.txt:62856`, `86921` |
| Shopkeep neutral-squad | `EV_I_SEE_NEUTRAL_SQUAD` | `1` | `RE_Kenshi_log_when_child_nodes.txt:120`, `15476`; `RE_Kenshi_log_when_parent_nodes.txt:6077`, `10063` |
| Pacifier neutral-squad | `EV_I_SEE_NEUTRAL_SQUAD` | `0` | `RE_Kenshi_log_when_child_nodes.txt:43`, `17453`; `RE_Kenshi_log_when_parent_nodes.txt:63`, `227` |

Inference:

`parentMonologue` belongs to the parent dialogue record, not the event type. Both tested event families appeared with both monologue values.

Confidence: High.

### B2 - Does monologue control runtime speaker identity?

Observed:

- Shopkeep talk-to-me, `parentMonologue=0`, resolved target-side roles, e.g. `T_TARGET_IF_PLAYER` to `#140805player5` at `RE_Kenshi_log_when_child_nodes.txt:11285`.
- Pacifier talk-to-me, `parentMonologue=1`, resolved target-side roles, e.g. `T_TARGET_WITH_RACE` to `#140805player1` at `RE_Kenshi_log_when_child_nodes.txt:18253` and `T_TARGET` to `#140805player5` at `20067`.
- Parent/root lines also resolved authored target-side and interjector roles under both monologue states.

Inference:

`parentMonologue` does not decide what `getSpeaker(line->speaker, ...)` returns.

Confidence: High.

### B3 - What did monologue do to the tested UI flow?

Observed:

- The shopkeep talk-to-me child package opened a normal dialogue window with player choices while `parentMonologue=0`.
- The pacifier talk-to-me child package also opened a normal dialogue window with player choices while `parentMonologue=1`.
- Shopkeep neutral-squad parent/root dialogue continued in the dialogue log while `parentMonologue=1`.
- Pacifier neutral-squad parent/root dialogue continued in the dialogue log while `parentMonologue=0`, though the user observed fewer/rarer thief `I_SEE_NEUTRAL` outputs and some dialogue appeared in the log/box without obvious speech bubbles above Thief Boss.

Inference:

For the tested flows, monologue did not prevent player dialogue windows, choices, action execution, or runtime speaker resolution. It may still affect ambient frequency, speech-bubble presentation, or automatic flow, but it is not the targeting or speaker-identity switch.

Confidence:

- High for "monologue does not control targeting/speaker identity."
- High for "both monologue states can still show dialogue windows and choices in these tested setups."
- Medium for the exact positive UI meaning of monologue, because the tests compare real shopkeep/pacifier setups rather than a single duplicated setup with only the monologue flag changed.

## C. Root vs Child Dialogue Nodes

### C1 - How do parent/root nodes present visible speech?

Observed:

- In the clean parent-node-only screenshot, all visible parent/root dialogue came out under the NPC/dialogue owner in the in-game dialogue log.
- Runtime logs from the same parent/root pass still resolved authored speaker roles separately:
  - `T_TARGET_WITH_RACE` to `#140805player1` at `RE_Kenshi_log_when_parent_nodes.txt:11919`
  - `T_TARGET_IF_PLAYER` to `#140805player5` at `RE_Kenshi_log_when_parent_nodes.txt:25665`
  - `T_INTERJECTOR1` to `Ninja Guard` at `987`
  - `T_INTERJECTOR1` to `Shinobi Trader` at `44243`

Inference:

Parent/root nodes are visually owner/NPC-biased in the tested UI, but runtime `getSpeaker(...)` still follows the authored line speaker.

Confidence: High.

### C2 - How do child nodes present visible speech and choices?

Observed:

- Shopkeep child-package dialogue window showed player-facing choices.
- Pacifier child-package dialogue window showed player-facing choices.
- Shopkeep child history showed target-side/player-labelled child lines such as `#140805player1` and `#140805player5`.
- Pacifier child history showed target-side/player-labelled child lines such as `#140805player1` and `#140805player5`.
- Logs match this: shopkeep child `T_TARGET_WITH_RACE` resolved `#140805player1` at `RE_Kenshi_log_when_child_nodes.txt:2407`; shopkeep child `T_TARGET_IF_PLAYER` resolved `#140805player5` at `11285`; pacifier child `T_TARGET` resolved `#140805player5` at `20067`.

Inference:

Child dialogue nodes usually present closer to runtime speaker resolution and support normal player choice flow in the tested talk-to-me cases.

Confidence: High.

## D. Owner And Target Roles

### D1 - What is `Dialogue::me` / `T_ME`?

Observed:

- Shopkeep owner resolved as `Barman`, e.g. `RE_Kenshi_log_when_child_nodes.txt:120`.
- Pacifier-owned neutral-squad dialogue resolved as `Pacifier`, e.g. `RE_Kenshi_log_when_parent_nodes.txt:227`.
- Pacifier-style package placed on Thief Boss resolved owner as `Thief Boss`, e.g. `RE_Kenshi_log_when_parent_nodes.txt:98004`.

Inference:

`T_ME` / `Dialogue::me` is the active dialogue/package owner.

Confidence: High.

### D2 - What is `T_TARGET`?

Observed:

- Shopkeep neutral-squad contexts resolved target-side non-player characters such as `Twitchy Bar Thug` and `Mercenary Captain`.
- Pacifier neutral-squad contexts resolved target-side characters such as `Hep`, `#140805player3`, and `Bar Thug`.
- Talk-to-me child `T_TARGET` resolved the player-side target, e.g. pacifier child `#140805player5` at `RE_Kenshi_log_when_child_nodes.txt:20067`.

Inference:

`T_TARGET` is the current conversation target / detected target-side character. It is not universally "the player."

Confidence: High.

## E. Speaker Dropdown Roles

### E1 - What does `T_TARGET_IF_PLAYER` do?

Observed:

- In player talk-to-me, `T_TARGET_IF_PLAYER` resolved player-side, e.g. `#140805player5` at `RE_Kenshi_log_when_child_nodes.txt:11285` and `RE_Kenshi_log_when_parent_nodes.txt:25665`.
- In neutral-squad contexts where the relevant target was not player-controlled, it resolved null, e.g. `RE_Kenshi_log_when_parent_nodes.txt:145` and `9677`.

Inference:

`T_TARGET_IF_PLAYER` means "target, but only if that target is player-controlled." Null in non-player contexts is expected behavior.

Confidence: High.

### E2 - What does `T_WHOLE_SQUAD` do through `getSpeaker(...)`?

Observed:

- Shopkeep child `T_WHOLE_SQUAD` resolved one character, `Barman`, at `RE_Kenshi_log_when_child_nodes.txt:9040`.
- Pacifier child `T_WHOLE_SQUAD` resolved one character, `Thief Boss`, at `RE_Kenshi_log_when_child_nodes.txt:19591`.
- Parent/root `T_WHOLE_SQUAD` rows also resolved owner-side, e.g. `RE_Kenshi_log_when_parent_nodes.txt:43`, `8840`, `18730`, `75225`.

Inference:

Through `getSpeaker(...)`, `T_WHOLE_SQUAD` is not an iterable squad. It returns one selected character. In observed cases, that character was owner/NPC-side.

Confidence:

- High that it is not squad-wide through `getSpeaker(...)`.
- Medium on the exact selection rule for the one character.

### E3 - What does `T_INTERJECTOR1` do?

Observed:

- Child talk-to-me interjector setup/probe rows resolved null before a populated slot, e.g. `RE_Kenshi_log_when_child_nodes.txt:652`, `4905`, `18695`, `19185`.
- Parent/root after-interjector rows resolved populated interjectors:
  - `Ninja Guard` at `RE_Kenshi_log_when_parent_nodes.txt:987` and `4275`
  - `Shinobi Trader` at `44243`
  - `Thief` at `49316`

Inference:

`T_INTERJECTOR1` depends on an interjector slot being populated by dialogue flow.

Confidence: High for `T_INTERJECTOR1`. `T_INTERJECTOR2` and `T_INTERJECTOR3` remain untested.

## F. Target Race

### F1 - Does `T_TARGET_WITH_RACE` use the line `target race` list?

Observed:

- Shopkeep child `T_TARGET_WITH_RACE_GREENLANDER` resolved `#140805player1` while conversation target was `#140805player5`, at `RE_Kenshi_log_when_child_nodes.txt:2407`.
- Pacifier child `T_TARGET_WITH_RACE_GREENLANDER` resolved `#140805player1` while conversation target was `#140805player5`, at `RE_Kenshi_log_when_child_nodes.txt:18253`.
- Parent/root `T_TARGET_WITH_RACE_GREENLANDER` selected matching target-side characters, e.g. `RE_Kenshi_log_when_parent_nodes.txt:103`, `6077`, `11919`, `62856`.
- Parent/root `T_TARGET_WITH_RACE_GREENLANDER` at `RE_Kenshi_log_when_parent_nodes.txt:63` resolved `Hep` (Scorchlander) — convTarget was also `Hep` (Scorchlander). Authored race list = Greenlander. No Greenlander was present. The line executed and `getSpeaker` fell back to convTarget.
- Shopkeep neutral-squad `T_TARGET_WITH_RACE_HIVE_PRINCE` resolved `de Zerka` when an eligible Hive Prince existed, at `RE_Kenshi_log_when_child_nodes.txt:15543`.

Inference:

`T_TARGET_WITH_RACE` uses the line `target race` list for runtime selection and can select someone other than the current `conversationTarget`.

Confidence: High.

### F2 - What happens when no matching target race exists?

Observed:

- Player talk-to-me Hive Prince no-match child rows: probes `25-IdentitySpike.mod` (SHOPKEEP) and `46-IdentitySpike.mod` (PACIFIER) appeared in `checkTags` / `originalResult` in `RE_Kenshi_log_when_child_nodes.txt` (e.g. L61, L564, L18144, L18195) but produced no `doActions` / `phase=context` in either log.
- Matching Greenlander sibling rows executed.
- When a real Hive Prince was present, the Hive Prince line executed: `RE_Kenshi_log_when_child_nodes.txt:15543`.

Inference:

For **child dialogue nodes**: no matching target-side race prevents the line from executing. `checkTags` may still consider the candidate.

For **root/parent dialogue nodes**: the line executes regardless of race match. `getSpeaker(T_TARGET_WITH_RACE)` falls back to the current conversation target when no race match is found.

Confidence: High.

## G. `checkTags` vs `_doActions`

### G1 - What does `checkTags` prove?

Observed:

Some probe rows appeared in `checkTags` / `originalResult` in the logs with no corresponding `phase=context` (e.g. probes `25-IdentitySpike.mod` and `46-IdentitySpike.mod` in `RE_Kenshi_log_when_child_nodes.txt`).

Inference:

`checkTags` proves candidate evaluation. It does not prove execution.

Confidence: High.

### G2 - What does `_doActions` / `phase=context` prove?

Observed:

`phase=context` appears only after `_doActions` sees the marker action.

Inference:

`_doActions` / `phase=context` is the strongest runtime evidence that a marked line executed.

Confidence: High.

## H. Open Tests For StatModification_Extension

These were not answered by DialogueIdentityProbe alone:

- Actual renamed StatModification actions should mutate the same character resolved by `getSpeaker(...)`.
- StatModification conditions need separate tests because they run through `checkTags`.
- A mixed condition/action line should confirm that if any vanilla/other-plugin condition fails, our action does not execute.
- `T_INTERJECTOR2` and `T_INTERJECTOR3` remain untested for squad-banter cases.
