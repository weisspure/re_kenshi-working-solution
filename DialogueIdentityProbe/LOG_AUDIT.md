# DialogueIdentityProbe Log Audit

This file records the no-skipping audit approach for the RE_Kenshi probe logs.

The raw logs are large enough that manual searching is risky. The audit therefore has two generated CSV files:

- `ASSERTIONS.md`
  - hand-written test-style assertions built from the generated CSVs
  - use this to review expected vs observed behavior

- `LOG_AUDIT_CONTEXTS.csv`
  - one row per `phase=context`
  - this means one row per probe-marked line that actually reached `_doActions`
  - includes log file, line number, probe id, text, event, parent monologue, line speaker, resolved speaker, conversation target, and `dlg->me`

- `LOG_AUDIT_PHASE_COUNTS.csv`
  - one row per probe id/text per log
  - counts `checkTags`, `originalResult`, `doActions`, `context`, `speakerProbe`, `conditionRow`, and `conditionReference`
  - this is the file to use for negative cases where a line was considered but never executed

## Context Row Counts

| Log | `phase=context` rows |
| --- | ---: |
| `RE_Kenshi_log_when_child_nodes.txt` | 80 |
| `RE_Kenshi_log_when_parent_nodes.txt` | 24 |

Total executed context rows audited so far: 104.

Only the regenerated clean parent/child logs are primary evidence. Older spike logs were removed from the audit because their FCS records were outdated and some labels/settings were wrong.

## How To Read The Audit

Start with `LOG_AUDIT_CONTEXTS.csv`.

For each row:

1. Check `Text` to identify the authored test case.
2. Check `Event` to separate `EV_PLAYER_TALK_TO_ME`, `EV_I_SEE_NEUTRAL_SQUAD`, and followup lines that report `EV_NONE`.
3. Check `ParentMonologue` when available.
4. Compare `Speaker` with `ResolvedSpeaker`.
5. Compare `ResolvedSpeaker`, `ConversationTarget`, and `DlgMe`.
6. If a test case is missing from contexts, search `LOG_AUDIT_PHASE_COUNTS.csv` for that text/probe id.

Then use `LOG_AUDIT_PHASE_COUNTS.csv`.

Rows with `CheckTags > 0` and `Context = 0` are the important negative or non-selected cases. They can mean:

- the line was considered but failed conditions,
- the line passed tag checks but failed later speaker/race/branch eligibility,
- a sibling line was selected instead,
- or the authored record was present but not reached in that test run.

Do not treat those rows as unimportant. They are often where the negative findings come from.

## Immediate Observations From Full Phase Counts

The clean audit found several probe rows with `checkTags` activity but no `phase=context`. These include:

- Hive Prince `T_TARGET_WITH_RACE` no-match cases in player talk-to-me child choices.
- Some sibling child-choice rows where another eligible branch appears to have been selected.
- Interjector setup/routing nodes whose job is to populate flow, not necessarily execute their own probe action.

The strongest race evidence is now two-sided:

- no-match Hive Prince child-choice lines appear in `checkTags` / `originalResult` but do not reach `_doActions`
- comparable Greenlander sibling lines execute
- when a real Hive Prince target exists, a Hive Prince `T_TARGET_WITH_RACE` line can execute, e.g. shopkeep neutral-squad resolved `de Zerka`

That pattern supports `target race` acting as eligibility/routing, not just display naming.

## Current Caveat

The CSV audit is comprehensive for log lines emitted by `DialogueIdentityProbe`, but it is not a final human interpretation of every row. The next review pass should classify every context row into:

- expected positive
- expected negative/non-selected
- repeated parent/root noise
- interjector/routing structure
- needs follow-up

Until that classification is done, prefer `FINDINGS.md` for current conclusions and use these CSVs as the evidence index.

## What The Current DLL Logs

The current probe hooks two places:

- `Dialogue::_doActions`
- `DialogLineData::checkTags`

Only lines with the `SM_PROBE_DIALOGUE_IDENTITY` marker action are logged.

### `phase=checkTags`

Means the line was considered for condition/tag evaluation.

Logs:

- `probeId`, line name, `text0`, line `GameData`
- `me` passed to `checkTags`
- `target` passed to `checkTags`
- runtime race/subrace lists from the line
- FCS `conditions` references
- runtime condition rows: condition enum, comparison, `who`, value

Use it for:

- candidate evaluation
- negative evidence when a line is considered but never executes
- checking condition `who` data
- checking target-race list population

Do not use it as proof the line executed.

### `phase=originalResult`

Means vanilla/original `checkTags` returned.

Logs:

- `originalResult=1` or `0`
- same `me` and `target` passed to `checkTags`

Use it for:

- knowing whether tag/condition checks passed for that candidate evaluation

Important limitation:

`originalResult=1` still does not prove the line executed. Later selection, race resolution, branch routing, or speaker eligibility can still prevent `_doActions`.

### `phase=doActions`

Means the marked line reached `Dialogue::_doActions`.

Logs:

- marker action hit

Use it for:

- strong evidence the line executed

### `phase=context`

Logged immediately after `phase=doActions`.

Logs:

- event enum/int
- parent dialogue flags from `dlg->currentConversation`: `for enemies`, `monologue`, `locked`, `one at a time`
- authored line speaker enum/name
- `dlg->getSpeaker(line->speaker, line, false)`
- `dlg->getConversationTarget().getCharacter()`
- `dlg->me`
- current conversation `GameData`
- current line `GameData`

Use it for:

- actual executed-line identity
- parent monologue/state
- runtime speaker resolution
- owner/target comparison

Important limitation:

Some followup/child/routing rows report `event=EV_NONE` or parent fields as `unavailable`. Those rows can still be valid executed child/routing evidence, but parent flags must be inferred from surrounding parent rows and FCS layout.

### `phase=lineList`

Logs runtime line lists:

- `isTargetRace/runtime target race`
- `isTargetSubRace_specificallyTheTarget`
- `isMyRace`
- `isMySubRace`

Use it for:

- proving whether target-race/subrace data was present at runtime

### `phase=speakerProbe`

Logs `dlg->getSpeaker(...)` for fixed roles:

- `T_ME`
- `T_TARGET`
- `T_TARGET_IF_PLAYER`
- `T_INTERJECTOR1`
- `T_WHOLE_SQUAD`
- `T_TARGET_WITH_RACE`

Use it for:

- comparing all role resolutions on the same executed line
- seeing null vs concrete character behavior
- checking whether `T_TARGET_WITH_RACE` selects someone other than `conversationTarget`

Current limitation:

The probe does not log `T_INTERJECTOR2` or `T_INTERJECTOR3`.

## What Screenshots Still Add

Logs prove runtime identity and execution. Screenshots prove visible UI presentation.

For the parent-node pass, screenshots supplied with the clean logs show all visible parent/root dialogue coming from the NPC/dialogue owner. Keep screenshots for:

- FCS line layout showing the exact parent/root line speaker and target race list
- in-game dialogue bubble/history showing who visibly spoke
- any choice menu showing which child/root options surfaced

This matters because visible presentation is UI evidence, while logs prove runtime identity and execution.
