# StatModification_Extension

`StatModification_Extension` is a RE_KENSHI plugin that extends dialogue lines with FCS-driven skill/stat actions and conditions.

It is designed for mod authors who want readable FCS records for:

- adding or removing skill levels from a dialogue participant
- setting a skill level to an exact value
- comparing the same stat between two dialogue participants

The core rule is simple: FCS data chooses the stat and value, runtime code applies it explicitly, and failures are logged instead of guessed.

## Custom Item Types

The plugin uses the `3000+` FCS item type range.

| Type | ID | Purpose |
| --- | ---: | --- |
| `STAT_DEFINITION` | 3000 | Defines which `StatsEnumerated` value to target |
| `CLAMP_PROFILE` | 3001 | Optional clamp configuration |
| `ADJUST_SKILL_LEVEL` | 3002 | Adds or removes a stat value |
| `SET_SKILL_LEVEL` | 3003 | Sets a stat value exactly |

`STAT_DEFINITION` is intentionally `3000`: it is the first and most important type for compatibility, because other modders can add records for additional stats without changing this plugin.

## Dialogue Actions

Dialogue line actions are read from `objectReferences` in `Dialogue::_doActions`.

Supported action keys:

- `add skill levels to speaker`
- `add skill levels to target`
- `add skill levels to owner`
- `remove skill levels from speaker`
- `remove skill levels from target`
- `remove skill levels from owner`
- `set skill level for speaker`
- `set skill level for target`
- `set skill level for owner`

Adjust actions run first. Set actions run second, so a set action on the same line overrides an earlier adjustment.

## Target Resolution

| Role | Runtime source |
| --- | --- |
| speaker | `dlg->getSpeaker(dialogLine->speaker, dialogLine, false)` |
| target | `dlg->getConversationTarget().getCharacter()` |
| owner | `dlg->me` |

The `speaker` role handles NPC lines, player replies, and interjectors through Kenshi's dialogue speaker resolution.

`T_WHOLE_SQUAD` is not supported by this plugin.

## Stat Definitions

Actions do not use string IDs or hardcoded stat whitelists. Each action references a `STAT_DEFINITION` record.

`STAT_DEFINITION` fields:

- `enum value`: the integer value cast to `StatsEnumerated`
- `comments`: author-facing notes ignored by runtime

This lets plugin authors ship new stat definition records for custom stats, and lets compatibility mods add records for published stat enum values without recompiling this plugin.

`STAT_NONE` (`0`) is rejected. No other whitelist is applied.

Custom-stat authors have three compatibility options:

- Depend on StatModification_Extension and author normal records using its FCS schema.
- Ship a separate compatibility patch that depends on both mods.
- Advanced: declare this plugin's custom item types in their own `fcs.def` and ship contract-conforming records directly. This can work, but it squats on shared enum IDs and only has runtime behavior when StatModification_Extension is installed and loaded.

Native or patch compatibility should include `STAT_DEFINITION` records, optional `CLAMP_PROFILE` records, and useful example `ADJUST_SKILL_LEVEL` / `SET_SKILL_LEVEL` records. There is no default clamp for custom stats.

## Clamp Profiles

Clamp profiles are optional.

| Field | Purpose |
| --- | --- |
| `clamp min` | inclusive lower bound |
| `clamp max` | inclusive upper bound |

No clamp profile means the stat is written unclamped. A clamp profile means the author explicitly opted into clamping.

## Conditions

The plugin adds two dialogue condition IDs:

| Name | ID | Behavior |
| --- | ---: | --- |
| `DC_STAT_LEVEL_COMPARE_UNMODIFIED` | 3004 | `left.stat OP right.stat`, using base stats |
| `DC_STAT_LEVEL_COMPARE_MODIFIED` | 3005 | `left.stat OP right.stat`, including modifiers |

Conditions use Kenshi's built-in comparison operator field. The `tag` field stores the `StatsEnumerated` integer.

For comparison conditions, `who = T_ME` means `owner OP target`; other supported values mean `target OP owner`.

`T_WHOLE_SQUAD` is not supported for stat conditions and will log an error.

Single-character stat threshold checks are intentionally not duplicated in pass 1. Use BFrizzle's Dialogue conditions for those checks. A future pass may add improved threshold conditions under new IDs if they support behavior Dialogue does not, such as squad-aware checks.

## Runtime Behavior

There are only two mutation primitives:

- adjust: `stat += delta`
- set: `stat = value`

Both use `CharStats::getStatRef`, optionally apply a clamp profile, and log the before/after values to RE_KENSHI.

The plugin does not grant XP, randomise values, use hidden defaults, or apply squad-wide changes.

## Design Principles

- No sentinel values: `0` means `0`
- No string ID fallback
- No presets or tiers
- Explicit behavior over implicit behavior
- FCS-driven data instead of hardcoded stat logic
- Clear RE_KENSHI logs for missing stats, wrong types, null pointers, and unsupported cases

## Install

Build the project, then copy:

- `StatModification_Extension/StatModification_Extension/` to `[Kenshi install dir]/mods/StatModification_Extension/`
- the compiled `StatModification_Extension.dll` into that mod folder

Enable the mod from Kenshi's `Mods` tab when launching with RE_KENSHI.

