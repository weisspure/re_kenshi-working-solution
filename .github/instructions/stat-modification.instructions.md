---
applyTo: "StatModification_Extension/src/**,StatModification_Extension/StatModification_Extension/fcs.def,StatModification_Extension/StatModification_Extension/FCS_extended.json,StatModification_Extension/StatModification_Extension/RE_Kenshi.json,StatModification_FCS/**,wiki/Home.md,wiki/For-Mod-Authors.md,wiki/For-Plugin-Authors.md,wiki/FCS-Schema-Reference.md,wiki/Conditions.md"
---
# StatModification_Extension Contract

## Commands
- Build runtime: `StatModification_Extension\build.bat`
- Build FCS helper: `dotnet build StatModification_FCS/StatModification_FCS.csproj -c Release`
- Runtime DLL output: `StatModification_Extension/x64/Release/StatModification_Extension.dll`
- FCS helper output: `StatModification_FCS/bin/Release/net4.8/StatModification_FCS.dll`

## Architecture
- Entry: `src/StatModification_Extension.cpp`; installs `_doActions` + `checkTags` hooks only.
- Actions: `src/Actions.cpp`; named FCS action dispatch + stat mutation.
- Conditions: `src/Conditions.cpp`; comparison conditions + `checkTags_hook`.
- Targets: `src/Targets.cpp`; speaker/other-side resolution.
- Data: `src/FcsData.cpp`; FCS/GameData validation.
- Clamp: `src/Clamp.cpp`; optional profiles; absent profile = unclamped.
- Logging: `src/Logging.cpp`; RE_KENSHI messages; successful writes use `LogDebug` behind `ENABLE_DEBUG_LOGS`.
- Constants: `src/Constants.h`; IDs, action strings, FCS field names. Keep magic strings there.
- Editor helper: `StatModification_FCS/`; C# for FCS Extended only. Runtime must not depend on it.
- Deploy package: `StatModification_Extension/StatModification_Extension/`; `fcs.def`, `RE_Kenshi.json`, `FCS_extended.json`, `.mod`.

## IDs
| Name | ID | Purpose |
|---|---:|---|
| `STAT_DEFINITION` | 3000 | Stat enum target; compatibility anchor |
| `CLAMP_PROFILE` | 3001 | Optional clamp policy |
| `ADJUST_SKILL_LEVEL` | 3002 | Train/untrain action config |
| `SET_SKILL_LEVEL` | 3003 | Train/untrain-until action config |
| `DC_STAT_LEVEL_COMPARE_UNMODIFIED` | 3004 | Compare base visible stat levels |
| `DC_STAT_LEVEL_COMPARE_MODIFIED` | 3005 | Compare effective visible stat levels |

Before first release, keep IDs gapless unless preserving compatibility with shipped data.

## Non-Negotiable Behavior
- No stat whitelist: cast `STAT_DEFINITION["enum value"]` to `StatsEnumerated`; reject only `STAT_NONE`.
- No stringID fallback for runtime behavior.
- No sentinel values: `0` means `0`.
- No default clamp: absent `clamp profile` means unclamped.
- Clamp ref must be `CLAMP_PROFILE`; wrong type logs and continues unclamped.
- Missing/wrong FCS data, null ptrs, unsupported cases, malformed conditions: log clearly.
- Use explicit named dispatchers; do not replace action handling with a dynamic registry.

## Actions
- Public action keys are speaker-first: `train skill levels`, `untrain skill levels`, `train skill levels until`, `untrain skill levels until`.
- `... other ...` action keys target the other main dialogue side, not "any character who is not speaking".
- `... squad ...` action keys target active platoon members for the resolved speaker or other-side anchor.
- Other-side rule: resolve speaker/owner/target; if `speaker == owner`, other = target; else other = owner.
- Adjust actions run before until/set; until/set overrides earlier adjustment on same line.
- `T_WHOLE_SQUAD` via action speaker resolution = one Kenshi-selected `Character*`, not whole squad.

## Conditions
- Current form: compare two participants, `left.stat OP right.stat`.
- Stricter than BFrizzle Dialogue threshold checks, which compare one character to fixed value.
- Compare visible whole levels: cast both stat floats to `int`; `90.1` and `90.5` both equal visible `90`.
- `who == T_ME`: owner left side, `owner OP target`.
- `who == T_WHOLE_SQUAD`: owner-side compatibility; warn not true squad aggregation.
- Other supported `who`: target left side, `target OP owner`.
- Future threshold IDs may use `3006/3007` if adding value beyond Dialogue: fail-closed logging or real squad support.

## Compatibility
- Custom stat mods can depend on this mod and author normal records.
- Preferred soft-compat: separate patch mod depends on both mods; includes `STAT_DEFINITION`, optional `CLAMP_PROFILE`, example action records.
- Advanced authors may squat on custom item types in own `fcs.def`; risky if schemas diverge; runtime works only when this plugin installed.
- FCS helper dropdowns = editor sugar only.
- Custom stats work in actions via `STAT_DEFINITION`; in conditions via raw `tag` ints.
- Third-party condition dropdown UX: companion FCS helper DLLs should replace/register condition default for IDs `3004/3005`; may expose combined enum of built-in + custom stat ints.
- Only one enum type can own a condition ID dropdown at a time.
- Do not duck-type clamp profiles; explicit type check protects authors from wrong-record refs.

## Canonical Record Rules
- Runtime stays open-ended for custom stats; do not turn canonical exclusions into a runtime whitelist.
- Never ship canonical records for `STAT_NONE`, `STAT_END`, `STAT_WEAPONS`, `STAT_HIVEMEDIC`, `STAT_VET`, `STAT_MASSCOMBAT`, or `STAT_SURVIVAL`.
- Do ship `STAT_FRIENDLY_FIRE` as "Precision Shooting"; it is a real trainable stat.
- Use in-game names: `STAT_MEDIC` = "Field Medic", `STAT_SMITHING_BOW` = "Crossbow Smith".

## Style
- Keep C++ beginner-readable: guard clauses over nested `if`/`else`, one variable per declaration, `Type* name` pointer style.
- Use Doxygen `@brief` comments on module-facing header functions.
- Comment non-obvious FCS/runtime behavior; do not restate syntax.
- Headers are internal project headers, not supported external C++ API.
