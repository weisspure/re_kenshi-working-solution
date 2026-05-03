# RaceChange Agent Notes

This file is for coding agents and maintainers. Keep `README.md` human-facing: what the plugin does, how to author the FCS actions, how to build/install, and where to verify. Put implementation hazards, cleanup direction, logging internals, and future-agent memory here or in `NEXT_AGENT_HANDOVER.md`.

Before broad edits to `CLAUDE.md` or `AGENTS.md`, check for the local `claude-md-improver` skill at `~/.agents/skills/claude-md-improver/SKILL.md` and follow it if present. It is not required for normal code changes.

## Commands

Use these checks before claiming RaceChange work is safe:

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

After file moves, source splits/merges, `*.vcxproj` edits, include-path/dependency changes, or large C++ refactors, refresh code intelligence from the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File tools/generate_compile_commands.ps1
```

## Public Surface

- `RaceChange_Extension` is intentionally separate from `StatModification_Extension`.
- The public authoring surface is two direct dialogue action keys:
  - `change race`
  - `change other race`
- Each action references an existing `RACE` subrace record.
- `value[0]` is part of the authoring contract:
  - `0`: humanoid/playable in-place mutation path.
  - `1`: animal/non-humanoid intent. The runtime looks for an `ANIMAL_CHARACTER` template whose `race` list references the target `RACE`.
- Treat new `value[0]` meanings as compatibility changes, not internal refactors.

## Runtime Behavior

Current high-level flow:

1. Hook `Dialogue::_doActions`.
2. Resolve the dialogue target using the same speaker-first model as `StatModification_Extension`.
3. Validate that the referenced record is a `RACE`.
4. Log current race, target race, resolved character, and basic race diagnostics.
5. Classify humanoid/default intent or animal intent from `value[0]`.
6. For humanoid/default intent, remove worn armour without destroying it, call `Character::setRace(targetRace)`, reset appearance data, validate inventory sections, restore removed armour with destruction disabled, and open the vanilla character editor through `PlayerInterface::activateCharacterEditMode(character)`.
7. For animal intent with a matching `ANIMAL_CHARACTER` template, remove all inventory items, drop them to the ground, spawn the animal template, transplant the supported state, destroy the source character after replacement is ready, validate the spawned character's inventory sections, and open the character editor for the spawned character.
8. For animal intent without a matching template, fall back to the in-place mutation path after logging that no animal template was found.

## Known Runtime Boundaries

- Animal replacement is intentionally modest. It transplants name, stats, and known common runtime state; do not describe it as full character migration unless code and tests prove that.
- Missing or malformed animal template data falls back to in-place mutation, which can still behave like the old non-humanoid limitation.
- Humanoid/playable race changes unequip worn armour before changing race. If inventory has no room, vanilla inventory behavior decides whether the item can be placed or dropped.
- Animal replacement evacuates all inventory items and drops them to the ground before source-character destruction. This is intentional and not equivalent to the humanoid armour-restore policy.
- `validateInventorySections()` and `activateCharacterEditMode(...)` are part of the observed working runtime flow, not cosmetic cleanup.

## Logging

RaceChange logs through RE_Kenshi logging functions with a central level filter. This KenshiLib build exposes `DebugLog` and `ErrorLog`; the wrapper keeps semantic levels at call sites while routing non-error messages through `DebugLog`.

Current default threshold: `info`.

Supported level names:

- `error`
- `warning`
- `info`
- `debug`
- `trace`

Names parse case-insensitively. Runtime config loading is not wired yet; changing the default currently requires editing `RACE_CHANGE_LOG_DEFAULT` in `src/Logging.h`.

Keep debug/trace call sites behind the central filter. Do not reintroduce ad hoc toggles such as `ENABLE_ACTION_SCAN_LOGS`.

## Cleanup Route

Use `NEXT_AGENT_HANDOVER.md` as the detailed cleanup plan. That handover is agent-facing and may be more current than public docs during refactors. Prefer code and logs over prose when they disagree.

Use `TEST_PLAN.md` for runtime dry-run coverage and expected `RE_Kenshi.log` evidence. Unit tests cover only pure decisions; they do not prove in-game hook, inventory, editor, spawn, or source-destruction behavior.
