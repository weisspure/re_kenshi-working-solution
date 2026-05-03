# StatModification_Extension Contract

Use for: StatModification runtime/editor-helper code, schema files, and wiki contract docs.

## Commands
- Runtime build: `StatModification_Extension\build.bat`
- FCS helper build: `dotnet build StatModification_FCS/StatModification_FCS.csproj -c Release`
- Runtime output: `StatModification_Extension/x64/Release/StatModification_Extension.dll`
- FCS helper output: `StatModification_FCS/bin/Release/net4.8/StatModification_FCS.dll`

## Architecture
- Entry hooks only in `src/StatModification_Extension.cpp`: `_doActions`, `checkTags`.
- Runtime modules: `Actions.cpp`, `Conditions.cpp`, `Targets.cpp`, `FcsData.cpp`, `Clamp.cpp`, `Logging.cpp`.
- IDs/keys/field names live in `src/Constants.h`.
- `StatModification_FCS/` is editor-only support; runtime must not depend on it.
- Deploy package is `StatModification_Extension/StatModification_Extension/` (`fcs.def`, `RE_Kenshi.json`, `FCS_extended.json`, `.mod`).

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
- No stat whitelist. Cast `STAT_DEFINITION["enum value"]` to `StatsEnumerated`; reject only `STAT_NONE`.
- No stringID fallback and no sentinel reinterpretation (`0` stays `0`).
- No default clamp. Missing clamp profile means unclamped.
- Clamp ref type must be `CLAMP_PROFILE`; wrong type logs and continues unclamped.
- Missing/wrong FCS data, null pointers, malformed conditions must log clearly.
- Keep explicit named dispatchers; no dynamic registry for action handling.

## Actions
- Public keys are speaker-first: `train skill levels`, `untrain skill levels`, `train skill levels until`, `untrain skill levels until`.
- `... other ...` keys target the other main dialogue side, not arbitrary non-speakers.
- `... squad ...` keys target active platoon members of resolved speaker/other anchor.
- Other-side rule: if `speaker == owner`, other is target; else other is owner.
- Adjust actions run first; until/set overrides earlier same-line adjustment.
- `T_WHOLE_SQUAD` on this API path resolves to one Kenshi-selected `Character*` (not true squad iteration).

## Conditions
- Compare form is two-party: `left.stat OP right.stat`.
- Compare visible whole levels (`int` cast): e.g., `90.1` and `90.5` both render `90`.
- `who == T_ME`: `owner OP target`.
- `who == T_WHOLE_SQUAD`: owner-side compatibility only; not true squad aggregation.
- Other supported `who`: `target OP owner`.
- Future threshold IDs (`3006/3007`) only if meaningful beyond Dialogue behavior.

## Compatibility
- Soft-compat model: patch mod depends on both mods and authors records with `STAT_DEFINITION` (+ optional `CLAMP_PROFILE`).
- FCS helper dropdowns are editor sugar only.
- Custom stats in actions use `STAT_DEFINITION`; in conditions use raw `tag` ints.
- For condition dropdown UX, third-party helper DLLs can register IDs `3004/3005`, but only one enum owner applies per ID.
- Keep explicit clamp type checks; do not duck-type profiles.

## Canonical Record Rules
- Runtime remains open-ended for custom stats. Canonical exclusions are not runtime whitelist behavior.
- Do not ship canonical records for: `STAT_NONE`, `STAT_END`, `STAT_WEAPONS`, `STAT_HIVEMEDIC`, `STAT_VET`, `STAT_MASSCOMBAT`, `STAT_SURVIVAL`.
- Ship `STAT_FRIENDLY_FIRE` as `Precision Shooting`.
- Use in-game names (`STAT_MEDIC` = `Field Medic`, `STAT_SMITHING_BOW` = `Crossbow Smith`).

## Style
- Keep C++ beginner-readable: guard clauses, one variable per declaration, `Type* name` pointer style.
- Use Doxygen `@brief` on module-facing header functions.
- Comment non-obvious FCS/runtime behavior only.
- Headers are internal project headers, not external plugin API.
