---
applyTo: "StatModification_Extension/**,wiki/**"
---
# StatModification_Extension - Design Reference

## Type IDs (fcs.def and C++ enum must match exactly)
| Name | ID | Purpose |
|---|---|---|
| STAT_DEFINITION | 3000 | Which stat - memorable anchor, other plugins need this number |
| CLAMP_PROFILE | 3001 | Clamping policy |
| ADJUST_SKILL_LEVEL | 3002 | Add/remove actions |
| SET_SKILL_LEVEL | 3003 | Set actions |
| DC_STAT_LEVEL_COMPARE_UNMODIFIED | 3004 | Condition: compare two characters' base stats |
| DC_STAT_LEVEL_COMPARE_MODIFIED | 3005 | Condition: compare two characters' effective stats |

Because this plugin is not released yet, keep custom enum IDs gapless unless there is a compatibility reason to preserve a retired value. This is a shared collision space; do not leave unused IDs just because an unreleased draft used them.

## Design rules - do not regress
- No stat whitelist - `STAT_DEFINITION.enum_value` cast directly to `StatsEnumerated`, no validation beyond rejecting STAT_NONE
- No default clamping - absent clamp profile = unclamped; canonical records ship both clamped and unclamped variants
- 0 means 0 - no sentinel behavior
- No stringID fallback
- No presets or default amounts
- Explicit dispatcher - fixed named action keys, no dynamic registry

## Condition scope
- Pass 1 only implements `DC_STAT_LEVEL_COMPARE_UNMODIFIED` (3004) and `DC_STAT_LEVEL_COMPARE_MODIFIED` (3005).
- Do not add `DC_STAT_UNMODIFIED` or `DC_STAT_MODIFIED`. Those duplicate BFrizzle's Dialogue stat threshold checks and create FCS dropdown confusion when both plugins are loaded.
- For single-character stat threshold checks, authors should use Dialogue's existing conditions.
- Future pass: add improved threshold checks with `T_WHOLE_SQUAD` support under the next available IDs if we can make them meaningfully different from Dialogue's implementation.

## Compare condition `who` semantics (BFrizzle-compatible)
- `T_ME` = owner (NPC) is left-hand side
- Any other supported value = target (player) is left-hand side
- Compare conditions (3004/3005): `who == T_ME` -> left=me, right=target; evaluates `left OP right`
- `T_WHOLE_SQUAD` is not supported for our conditions yet - unresolved how `getConversationTarget().getCharacter()` behaves when dialogue target is a squad rather than an individual

## Dialogue identity testing caution
- Testing so far is on `EV_TALK_TO_ME` paths, initiated by the player through a button press.
- Other conversation paths are often initiated by the NPC from the FCS perspective. Do not assume `me`, `target`, `speaker`, or left/right condition semantics are identical there.
- BFrizzle's Dialogue behavior is the de facto standard for single-character stat checks until this plugin proves a better implementation in game.
- Before refining or replacing Dialogue-style condition logic, add temporary in-game instrumentation hooks and test non-`EV_TALK_TO_ME` paths. Silent inversion bugs are plausible if owner/target semantics differ.
- `LOG_DIALOGUE_IDENTITY_SPIKE` in `StatModification_Extension.cpp` is a temporary compile-time switch for this. Keep it `false` for normal builds.

## Suggested in-game identity spike records
- Create clearly named dialogue lines/actions that encode their expected placement and target, for example `SM_TEST_EV_TALK_TO_ME_ACTION_SPEAKER_EXPECT_NPC`, `SM_TEST_EV_TALK_TO_ME_ACTION_TARGET_EXPECT_PLAYER`, and equivalents for NPC-initiated/non-talk-to-me paths.
- Put one action per test line where possible so the RE_KENSHI log identifies the action key, resolved speaker, target, and owner without ambiguity.
- Include a mixed-condition test line: one StatModification compare condition that should pass, one vanilla/BFrizzle condition that should fail, and one StatModification action. Expected result: the line/action does not execute. If the action fires, hook-chain or engine condition/action ordering is not what we expect.
- Run the same tests with `who = T_ME` and a non-`T_ME` value for compare conditions to confirm left/right semantics.

## Third-party custom stat compatibility
- Easiest path: a custom-stat mod can depend on StatModification_Extension and author normal canonical records using this mod's FCS schema.
- Recommended soft-compat path: ship a separate compatibility patch mod that depends on both mods. It should contain `STAT_DEFINITION` records for custom stats, optional `CLAMP_PROFILE` records, and optional example `ADJUST_SKILL_LEVEL` / `SET_SKILL_LEVEL` records.
- Native soft support may be possible if another author deliberately declares this plugin's custom item types in their own `fcs.def` and ships contract-conforming records. This is hypothetical and untested. Warn this is advanced: it squats on shared enum IDs, can confuse FCS if schemas diverge, and still requires StatModification_Extension at runtime for actions/conditions to do anything.
- Do not support duck-typed clamp profiles. Runtime should require referenced clamp records to have type `CLAMP_PROFILE` and log wrong types, then continue unclamped because clamping is optional.
- No default clamp for external/custom stats. Authors must explicitly provide clamp profiles when clamped behavior is desired.

## Excluded StatsEnumerated values
These exclusions apply to canonical records shipped by StatModification_Extension. Do not turn this table into a runtime whitelist; runtime validation must remain open-ended for third-party custom stats and reject only `STAT_NONE`.

| Value | Reason |
|---|---|
| STAT_NONE (0) | Sentinel - always reject |
| STAT_WEAPONS (20) | Composite/aggregate, not directly trainable |
| STAT_HIVEMEDIC (14) | Not in CharStats struct - never shipped, exclude |
| STAT_VET (15) | Not in CharStats struct - never shipped, exclude |
| STAT_MASSCOMBAT (31) | Internal, not player-facing, exclude |
| STAT_SURVIVAL (33) | Internal, not player-facing, exclude |
| STAT_END (39) | Sentinel - always reject |

**STAT_FRIENDLY_FIRE (36) = "Precision Shooting" - confirmed real trainable stat, INCLUDE.**

## In-game name corrections for canonical records
- STAT_MEDIC (9) -> record name **"Field Medic"**
- STAT_SMITHING_BOW (38) -> record name **"Crossbow Smith"**

## Outstanding work
- **Canonical .mod records** not yet authored: 34 STAT_DEFINITION records (one per trainable stat), 1 CLAMP_PROFILE ("Default 0-100"), example ADJUST_SKILL_LEVEL and SET_SKILL_LEVEL pairs (clamped + unclamped) for each stat
- **StatModification_FCS C# project** not yet written - needed for named StatsEnumerated dropdown on condition `tag` field; conditions work without it (authors type integer)
- **Squad target behavior** unresolved - what `getConversationTarget().getCharacter()` returns when dialogue target is a squad rather than an individual has not been tested

## Deployment layout
```
Kenshi/mods/StatModification_Extension/
  RE_Kenshi.json                  { "Plugins": ["StatModification_Extension.dll"] }
  StatModification_Extension.dll  C++ runtime hooks
  fcs.def                         schema
  StatModification_Extension.mod  canonical records
  [future] FCS_extended.json      { "FCS_Plugins": ["StatModification_FCS.dll"] }
  [future] StatModification_FCS.dll
```

## Wiki pages (wiki/ directory)
Home.md, For-Mod-Authors.md, Conditions.md, FCS-Schema-Reference.md, For-Plugin-Authors.md

