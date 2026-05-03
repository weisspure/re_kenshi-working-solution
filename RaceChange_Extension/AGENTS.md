# RaceChange Agent Notes (Lean)

Use this as router + invariants. Keep heavy rationale in `NEXT_AGENT_HANDOVER.md`.

## Read Order

1. `README.md` (author-facing contract)
2. `AGENTS.md` (this file)
3. `NEXT_AGENT_HANDOVER.md` (deep cleanup plan)
4. `TEST_PLAN.md` (runtime verification matrix)

## Verify Before Claiming Safe

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

After broad C++ moves/refactors or `.vcxproj` edits:

```powershell
powershell -ExecutionPolicy Bypass -File tools/generate_compile_commands.ps1
```

## Public Authoring Contract

- Public keys: `change race`, `change other race`.
- References must target a `RACE` record.
- `value[0]` meanings are compatibility surface:
  - `0` = humanoid/default in-place path
  - `1` = animal intent path (template lookup via `ANIMAL_CHARACTER.race`)
- Treat new `value[0]` values as product contract changes.

## Runtime Path Contract

- Hook point: `Dialogue::_doActions`.
- Resolve target character using speaker-first runtime model.
- Validate referenced race type before mutation/replacement.
- Humanoid/default intent:
  - remove armour
  - `setRace(targetRace)`
  - reset appearance
  - validate inventory sections
  - restore armour (no destroy)
  - open character editor
- Animal intent with valid template:
  - evacuate full inventory and drop
  - spawn animal template
  - transfer supported state (name/stats/common runtime)
  - validate spawned inventory sections
  - open editor for spawned character
  - destroy source only after replacement is ready
- Animal intent with no template: log fallback, run in-place path.

## Non-Negotiable Boundaries

- Animal replacement is not full migration unless proven by code/tests.
- Full-inventory drop path is not equivalent to humanoid armour-only policy.
- `validateInventorySections()` and editor reopen are behavior-critical.
- `RaceChange_Extension` compatibility is separate from StatModification.

## Logging Policy

- Central filtered logging only; no ad hoc toggles.
- Supported levels: `error`, `warning`, `info`, `debug`, `trace`.
- Default threshold is code-defined in `src/Logging.h` (`RACE_CHANGE_LOG_DEFAULT`) unless runtime config is explicitly wired.

## Maintenance Notes

- Keep `README.md` human-facing and concise.
- Keep deep agent migration/refactor narrative in `NEXT_AGENT_HANDOVER.md`.
- Prefer code + test evidence over prose when docs disagree.
