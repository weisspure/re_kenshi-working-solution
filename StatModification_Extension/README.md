# StatModification_Extension

`StatModification_Extension` is a RE_KENSHI plugin that extends dialogue lines with FCS-driven skill/stat actions and conditions.

It is designed for mod authors who want readable FCS records for:

- training or untraining skill levels on the line speaker or the other dialogue side
- training or untraining until an exact skill level
- comparing the same stat between two dialogue participants

The core rule is simple: FCS data chooses the stat and value, runtime code applies it explicitly, and failures are logged instead of guessed.

## Code Map

The plugin entry point is `startPlugin()` in `src/StatModification_Extension.cpp`. Start there if you are learning how the plugin loads.

Runtime behavior is split into focused files under `src/`:

- `src/Actions.cpp`: dialogue actions that adjust or set stats
- `src/Conditions.cpp`: dialogue conditions that compare stats
- `src/Targets.cpp`: speaker, other-side, target, and owner resolution
- `src/Clamp.cpp`: optional clamp profile handling
- `src/FcsData.cpp`: safe FCS/GameData record readers
- `src/Logging.cpp`: RE_KENSHI logging helpers
- `src/Constants.h`: FCS item IDs, condition IDs, action keys, and field names

The nested `StatModification_Extension/` folder is the Kenshi mod package folder (`fcs.def`, `RE_Kenshi.json`, and `.mod` data), not C++ source.

`StatModification_FCS/` is a separate C# FCS editor helper. It makes this plugin's condition `tag` field render as a StatModification skill dropdown in FCS Extended.

## Where To Look

- Start in `src/StatModification_Extension.cpp` to see how the plugin loads and installs hooks.
- Read code comments when you want runtime details: hook chaining, target resolution, FCS reference contracts, and failure behavior live beside the code they explain.
- Read this README for project overview, supported features, and design principles.
- Read the wiki for mod-author guidance and FCS authoring workflow.
- Read `.github/instructions/` for project rules that future coding agents should preserve.

## Custom Item Types

The plugin uses the `3000+` FCS item type range.

| Type | ID | Purpose |
| --- | ---: | --- |
| `STAT_DEFINITION` | 3000 | Defines which `StatsEnumerated` value to target |
| `CLAMP_PROFILE` | 3001 | Optional clamp configuration |
| `ADJUST_SKILL_LEVEL` | 3002 | Configures train/untrain actions |
| `SET_SKILL_LEVEL` | 3003 | Configures train/untrain-until actions |

`STAT_DEFINITION` is intentionally `3000`: it is the first and most important type for compatibility, because other modders can add records for additional stats without changing this plugin.

## Dialogue Actions

Dialogue line actions are read from `objectReferences` in `Dialogue::_doActions`.

Supported action keys:

- `train skill levels`
- `untrain skill levels`
- `train other skill levels`
- `untrain other skill levels`
- `train squad skill levels`
- `untrain squad skill levels`
- `train other squad skill levels`
- `untrain other squad skill levels`
- `train skill levels until`
- `untrain skill levels until`
- `train other skill levels until`
- `untrain other skill levels until`
- `train squad skill levels until`
- `untrain squad skill levels until`
- `train other squad skill levels until`
- `untrain other squad skill levels until`

Train/untrain actions run first. Train/untrain-until actions run second, so an until action on the same line overrides an earlier adjustment.

## Target Resolution

| Role | Runtime source |
| --- | --- |
| speaker | `dlg->getSpeaker(dialogLine->speaker, dialogLine, false)` |
| target | `dlg->getConversationTarget().getCharacter()` |
| owner | `dlg->me` |
| other | the other main dialogue side: if speaker is owner, use target; otherwise use owner |
| speaker squad | active platoon members for the resolved speaker |
| other squad | active platoon members for the resolved other side |

The `speaker` role handles NPC lines, player replies, and interjectors through Kenshi's dialogue speaker resolution.

The public action API is speaker-first. `train skill levels`, `untrain skill levels`, `train skill levels until`, and `untrain skill levels until` apply to the resolved line speaker. The `... other ...` actions apply to the other main dialogue side: if the speaker is the owner, other is the target; otherwise other is the owner.

The `... squad ...` actions are explicit platoon-wide variants. They first resolve the speaker or other side using the same rules as the single-character actions, then apply the stat change to each active character in that resolved character's platoon. They do not use `getSpeaker(T_WHOLE_SQUAD, ...)` for iteration.

`other` means the character on the other side of the main conversation. It does not mean "anyone who is not speaking." If you want to affect an interjector or a specific squadmate, set that character as the line speaker and use the plain speaker action.

Runtime speaker resolution and visible dialogue presentation are related but not identical. Current probes show root dialogue nodes can be visibly presented by the owner/NPC even when `getSpeaker(...)` resolves a target-side character. Child text nodes generally display under the resolved speaker. StatModification actions use runtime speaker resolution, not the visible speech label alone.

Put another way: plain speaker actions follow what FCS resolves from the line's `speaker` dropdown, not necessarily the name currently shown in the dialogue UI. This matches Kenshi's runtime model and keeps targeting data-driven through FCS.

If a line's speaker is `T_WHOLE_SQUAD`, this plugin still asks Kenshi for one `Character*` with `getSpeaker(...)`. That means speaker-targeted actions can apply to the individual Kenshi resolves, but they are not squad-wide operations. Current probe results suggest this may resolve to the owner/NPC side in some contexts; treat it as "one selected character", not "every squad member".

`T_TARGET_IF_PLAYER` may resolve null when the target-side character is not player-controlled. In that case a speaker-targeted action logs the failed character resolution and skips.

`T_TARGET_WITH_RACE` uses the line's `target race` list. If no eligible target-side character matches that list, the line may never execute, so the action will not fire.

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

The optional `StatModification_FCS.dll` editor helper makes that `tag` field render as a StatModification skill dropdown in FCS Extended. Runtime still works without the helper, but authors would have to type stat integers manually.

These are not a duplicate of BFrizzle's single-character stat threshold checks. Dialogue's threshold checks are a lightweight baseline that answer `character.stat OP fixed value`. StatModification is stat-focused, so its stricter comparison checks answer `left.stat OP right.stat`, fail closed on malformed data, and log clear runtime errors.

StatModification also compares visible whole-number stat levels. Kenshi stores hidden decimal values, but players and FCS authors think in displayed levels. A character at internal `90.5` and a character at internal `90.1` both count as level `90` for equality checks.

For comparison conditions, `who = T_ME` means `owner OP target`; `who = T_WHOLE_SQUAD` also uses the owner side to respect FCS role semantics observed in action-side probes. Other supported values mean `target OP owner`.

`T_WHOLE_SQUAD` is accepted for comparison conditions, but it does not aggregate the whole squad. The condition hook receives one evaluated `me` / `target` pair from Kenshi, so `T_WHOLE_SQUAD` behaves like `T_ME`: `owner OP target`. The runtime logs a warning so authors know this is not true squad-wide behavior.

Single-character stat threshold checks are intentionally not duplicated in pass 1. Use BFrizzle's Dialogue conditions for those checks. A future pass may add improved threshold conditions under new IDs if they support behavior Dialogue does not, such as squad-aware checks.

## Runtime Behavior

There are only two mutation primitives under the author-facing training wording:

- train/untrain: `stat += delta`
- train/untrain until: `stat = value`

Both use `CharStats::getStatRef` and optionally apply a clamp profile. Successful writes can log before/after values through the central debug toggle in `src/Logging.cpp`; warnings and errors are always logged.

This direct-write behavior is intentional. It does not try to mimic Kenshi's XP progression, hidden training curves, or vanilla stat ceilings. Authors who want vanilla-like limits can attach a `CLAMP_PROFILE`; authors who want flat level rewards or values beyond vanilla limits can omit the clamp or set their own bounds.

The plugin does not grant XP, randomise values, or use hidden defaults. Squad-wide changes only happen through the explicit `... squad ...` action keys.

KenshiLib does expose XP-oriented `CharStats` methods such as `xpStat_eventBased(...)`, but those are only relevant if the project later adds separate author-facing action types for XP awards. They are not a replacement for the current direct-write actions.

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
- the compiled `StatModification_FCS.dll` into that mod folder if you want the FCS condition stat dropdown

Build commands:

```powershell
StatModification_Extension\build.bat
dotnet build StatModification_FCS/StatModification_FCS.csproj -c Release
```

Enable the mod from Kenshi's `Mods` tab when launching with RE_KENSHI.
