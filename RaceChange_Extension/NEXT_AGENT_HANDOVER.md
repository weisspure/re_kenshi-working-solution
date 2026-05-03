# RaceChange Extension Next-Agent Handover

This note exists because the current RaceChange runtime works, but the code grew organically. The next useful work is deliberate cleanup: collapse duplicated branches, split responsibilities into modules, improve readability, introduce configurable log levels, and grow serious test coverage. Protect behavior while doing it.

## Current Baseline

- The user has committed a clean working version.
- Treat `RaceChange_Extension/src/RaceActions.cpp` as the runtime golden path until proven otherwise.
- The animal transform path is currently in a happy, working place, but it is intentionally modest. It transplants basic identity/state only: name, stats, and runtime state. Do not imply full character migration unless the code and tests prove it.
- Animal replacement is less fragile than the failed cleanup attempt made it, but it still needs hardening because Kenshi runtime objects should be treated as hostile terrain.
- The in-place race mutation path still matters for humanoid/playable races.
- `RaceChange_Extension/README.md` may lag behind the implementation. Check code and logs before trusting prose.

## First Commands To Run

Run these before proposing architecture changes. They are meant to make the next agent inspect the same pressure points that made the audit useful:

```powershell
git status --short
rg -n "static .*\\(|ApplyRaceChangeRef|setRace|SpawnAnimalFromTemplate|FindAnimalTemplateForRace|RemoveAllInventoryItemsBeforeRaceChange|DropAllItems|RestoreRemovedInventoryItemsAfterRaceChange|validateInventorySections|activateCharacterEditMode|values\\[0\\]" RaceChange_Extension/src
rg -n "AnimalRaceActions|ActionCore|RaceActions|FcsData|Logging|Targets" RaceChange_Extension/RaceChange_Extension.vcxproj
rg -n "change race|change other race|animal|ANIMAL_CHARACTER|value\\[0\\]|setRace|character editor|inventory|slots" RaceChange_Extension wiki RaceChange_Tests
RaceChange_Extension\build.bat
```

If debugging a runtime report, also search the game log for the RaceChange block:

```powershell
rg -n "RaceChange:|change race|animal|inventory|spawn|destroyed source|validateInventorySections|activateCharacterEditMode" "C:\Program Files (x86)\Steam\steamapps\common\Kenshi\RE_Kenshi_log.txt"
```

## What The Previous Audit Found

These were real issues and are the intended target of the next cleanup:

- `RaceActions.cpp` is too large and mixes several responsibilities: FCS reference scanning, diagnostics, race mutation, animal template lookup, inventory evacuation, spawn/migration, appearance reset, and UI/editor refresh.
- Some helpers belong in core/support files rather than race-action logic, especially object-reference formatting, string field reads, race data lookup, and action scan logging.
- Animal-specific behavior wants a clearer boundary. Do this by extracting the current working behavior, not by redesigning it from memory.
- `ActionCore` should stay pure and testable. It is a good home for action-key parsing, target-role mapping, intent parsing, policy decisions, and other pure branch-collapsing helpers, but not for runtime Kenshi object mutation.
- The UI/editor refresh behavior is not cosmetic. Opening the character editor after reset/validation is part of the observed working flow, especially for appearance and slot refresh. Do not remove it without an in-game test matrix.
- The `value[0]` intent flag has product meaning now: `0` is humanoid/default, `1` is animal intent. Keep that explicit unless the user chooses a new authoring contract.
- Project membership matters. A file can exist under `src` and still be unused if it is not in `RaceChange_Extension.vcxproj`.

## Refactor Goals

The goal is not just smaller files. The target quality bar is production-style C++ code that still feels approachable to junior C++ readers. A new contributor should be able to open the source and understand intent, ownership, failure behavior, and where to add tests.

The runtime should read as a small set of decisions:

1. Scan dialogue actions.
2. Resolve target character and target race.
3. Classify intent/path.
4. Prepare inventory/appearance state.
5. Execute either in-place mutation or animal replacement.
6. Finalize inventory/UI/logging.

Prefer early returns over nested `if`/`else` blocks when validation fails or a path is complete. `ApplyRaceChangeRef` should become an orchestration function, not the place where every detail lives.

Collapse branches only when they truly share policy. Do not collapse branches just because both paths touch inventory or call `OpenCharacterEditor`; the animal spawn-and-replace path and humanoid in-place path have different invariants.

As functions move, consider carefully where each one belongs and whether it should be pure, const-friendly, runtime-mutating, or private to one translation unit. Do not make helpers globally visible just to make extraction easy.

## Documentation Standard

- `README.md`: human-facing. `AGENTS.md`: agent/maintainer memory + runtime behavior. `NEXT_AGENT_HANDOVER.md`: cleanup handoff. `CLAUDE.md`: delegates to `AGENTS.md`.
- Add Doxygen `@brief` on module-facing headers and implementation comments on non-obvious RE_Kenshi behavior as part of cleanup.

Add Doxygen or implementation comments throughout the extension source as part of the cleanup.

Audience:

- C++ newcomers reading the source to learn how the plugin works.
- Future maintainers trying to understand RE_Kenshi runtime hazards.
- Mod/plugin authors checking how the public FCS behavior maps to code.

Rules:

- Public headers should stay concise unless they are genuinely the best reading surface for a contract.
- Runtime-mutating functions should state what they mutate and what ownership/lifetime assumptions they rely on.
- Null-tolerant helpers should explicitly say what happens on null input.
- Pure helpers should say that they do not touch game state.
- Comments should teach intent and hazards, not restate obvious C++ syntax.
- Prefer short examples or concrete notes when a function depends on odd Kenshi behavior, such as `value[0]`, `GameData::objectReferences`, inventory slots, source-character destruction, or editor refresh.
- Keep implementation comments sparse and useful. For this extension, source-side comments are often more valuable than header comments because readers usually inspect the hook/runtime flow directly instead of importing the plugin as a library.

## Module Direction

Use extraction with behavior-preserving commits. A reasonable destination shape:

- `src/core/FcsData.*`: null-safe `GameData` helpers, string-field reads, object-reference lookup/formatting.
- `src/core/Logging.*`: log level config, log filtering, string descriptions, diagnostic formatting.
- `src/core/ActionCore.*`: public action keys, role mapping, validation gates, intent parsing, pure path/policy decisions.
- `src/runtime/Targets.*`: dialogue target resolution.
- `src/runtime/InventoryActions.*`: armour/full-inventory evacuation, restore, drop, and item state logging.
- `src/runtime/AppearanceActions.*`: appearance data reset and race reference update.
- `src/runtime/AnimalRaceActions.*`: animal template lookup, spawn, name/stat/common-state migration, source destroy.
- `src/RaceActions.*`: dispatcher/orchestrator only.

Do not do all of this in one patch. Extract one boundary, build, and inspect diffs before continuing.

After file moves or project edits, update `RaceChange_Extension.vcxproj` and refresh code intelligence if needed:

```powershell
powershell -ExecutionPolicy Bypass -File tools/generate_compile_commands.ps1
```

## Branch-Collapse Guidance

Good candidates for simplification:

- Repeated validation/log/return blocks around missing character, missing race, wrong record type, and unsupported intent.
- Repeated finalization sequences after mutation/replacement if they can be expressed as explicit helper calls without hiding path-specific behavior.
- Repeated race/action diagnostic formatting.
- Multiple local helpers that read/describe `GameData` references.

Bad candidates for premature collapse:

- Animal spawn-and-replace versus in-place `setRace`.
- Drop-versus-restore inventory policy.
- Humanoid armour-only evacuation versus full inventory evacuation.
- Editor opening for source character versus spawned replacement.

When in doubt, name the path instead of merging it. A clear `RunAnimalReplacement(...)` and `RunInPlaceRaceMutation(...)` is better than one generic function with vague booleans.

## Logging Direction

Introduce logging as a small centralized subsystem before expanding diagnostics further. The desired shape is closer to `logger.info(Messages::SpawningAnimalTemplate, args...)` than large string concatenation blocks inside runtime logic.

Design goals:

- One central logger instance or access point for the extension.
- Log level driven by mod config if practical, for example `RE_Kenshi.json` or a small adjacent config file. If runtime config parsing is awkward, start with a safe default plus an obvious extension point.
- Default behavior should not spam normal players who install the mod.
- Debug/trace logging should be powerful enough for support and development when deliberately enabled.
- Call sites should express intent, not formatting details.
- Logging should be easy for junior C++ readers to follow.

Suggested levels:

- `error`: action cannot complete, data is invalid, spawn/mutation fails, or an invariant is violated.
- `warning`: action can continue but behavior is degraded, fallback is used, optional runtime object is missing, or expected template evidence is absent.
- `info`: high-level lifecycle events: action scan hit, target resolved, path selected, mutation/replacement started/completed, editor opened.
- `debug`: structured state useful during development: pointers, detailed race/template fields, inventory item state, object reference summaries.
- `trace`: noisy per-item/per-reference/per-branch details that should normally be hidden.

Rules:

- Do not surface logs below the configured level.
- Keep default logging quiet enough for normal users. `info` is a reasonable default while this is still a proof of concept; `warning` may be better once stable.
- Log filtering should live in `Logging.*`, not scattered call-site `if` checks.
- Prefer call sites like `LogDebug(...)` / `LogTrace(...)` over a boolean such as `ENABLE_ACTION_SCAN_LOGS`.
- Expensive log message construction should be gated so disabled debug/trace logs do not build large strings.
- Document how the level is configured. If reading config from `RE_Kenshi.json` is practical, use that; otherwise start with a compile-time/default setting and leave a clear TODO.

Message organization:

- Move large log-message construction out of `RaceActions.cpp`.
- Prefer named message helpers/templates over inline concatenation.
- A C++ equivalent of a JS message dictionary is encouraged if it stays simple and type-safe enough for this codebase.
- Good call-site target:

```cpp
Logger::Get().Info(LogMessage::ChangingRace(character, beforeRace, targetRace, actionKey, role, intent));
Logger::Get().Debug(LogMessage::AnimalTemplateFound(targetRace, animalTemplate));
Logger::Get().Trace(LogMessage::InventoryItemRemoved(item, inventory));
```

Possible implementation shapes:

- `Logging.h/.cpp` owns `enum LogLevel`, `class Logger`, config loading, filtering, and output to RE_Kenshi.
- `LogMessages.h/.cpp` owns named message builders such as `ChangingRace(...)`, `AnimalTemplateFound(...)`, and `InventoryItemRemoved(...)`.
- If templates are used, keep them compile-time constants plus small formatting helpers. Avoid clever generic formatting that makes debugging harder than the current strings.
- If a singleton is used, keep it boring and explicit: `Logger::Get()` or `GetLogger()`. Avoid hidden global initialization hazards around RE_Kenshi startup.
- Tests should cover level parsing/filtering and representative message builders.

Do not let the logger design become a framework project. It should remove noise from the runtime code, not introduce a new maze.

## Testing Direction

Testing is currently pitiful for the ambition of this plugin. Treat this rearchitecture as a TDD exercise, not just a file move.

Use the repo/local superpowers workflow before meaningful implementation:

- Use systematic debugging when investigating runtime regressions.
- Use test-driven development when extracting or changing behavior.
- Use verification-before-completion before claiming a cleanup is safe.

For each function moved or introduced, ask:

- What is the happy path?
- What are the null/missing-data paths?
- What happens with the wrong `GameData` type?
- What happens with unsupported `value[0]`?
- What happens when inventory calls fail?
- What happens when an animal template is absent or malformed?
- What happens when runtime globals such as `ou`, `player`, platoon, or inventory are missing?
- What happens when multiple characters or dialogue sides are involved?

Coverage targets:

- Pure unit tests for action key parsing, role selection, intent parsing, validation gates, and path/policy decisions.
- Unit tests for any extractable non-runtime data helpers using fake/minimal `GameData`.
- Scenario tests for humanoid in-place mutation decisions.
- Scenario tests for animal replacement decisions, including missing template fallback.
- Tests for log-level filtering rules once logging is configurable.
- Integration-style tests around multi-step orchestration where practical, even if the actual game calls are mocked/faked.

Treat Kenshi as hostile terrain:

- Null pointers are plausible.
- References can point at unexpected record types.
- Object reference lists can be empty or malformed.
- Runtime globals can be absent.
- Inventory operations can fail or have surprising side effects.
- A build can succeed while the copied in-game DLL is stale.

When code cannot be tested outside the game, isolate the smallest untestable shell and push decisions into testable helpers.

## Non-Negotiable Runtime Checks

Before claiming a refactor is safe:

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

Then do at least one in-game dry run for each changed runtime path:

- Humanoid to humanoid, wearing armour.
- Humanoid to animal with `value[0] == 1`, wearing armour and carrying inventory.
- A fallback/invalid animal-template case if animal detection or template lookup changed.

For the animal case, verify in the game and log:

- Inventory/gear is handled by the intended path before replacement.
- The spawned animal has the expected race/template.
- The deliberately supported transplanted state is present: name, stats, and known runtime state.
- Human equipment slots are gone or no longer usable after the transform.
- The source human is destroyed only after the replacement is ready.
- The editor opens for the final active character, not the destroyed source.

## Known Footguns

- `Inventory::getAllItems()` may not tell the whole story for equipped gear. If changing inventory handling, inspect both logs and in-game slot UI.
- Restoring removed items versus dropping them is behaviorally significant. Do not replace one with the other as a "cleanup" unless the user explicitly chooses that policy.
- `Character::setRace(...)` alone can update race-derived stats while leaving the live object class human-like. That is why the animal spawn-and-replace path exists.
- `validateInventorySections()` and `activateCharacterEditMode(...)` were easy to dismiss as UI refreshes, but they are part of the observed working sequence.
- `RaceChange_Extension/build.bat` builds the DLL. Check whether your test environment loads `x64/Release/RaceChange_Extension.dll` or a copied mod-folder DLL.
- Keep unrelated repo cleanup out of this work. This extension is a proof of concept and runtime behavior matters more than tidy file counts.

## Suggested Next Step

The safest useful first cleanup sequence:

1. Reconcile `README.md` with the current spawn-and-replace animal behavior.
2. Add Doxygen comments to the current public headers so intent is captured before moving code.
3. Add or update pure tests around `value[0]` intent parsing, action-key targeting, and any new pure path-selection helper.
4. Introduce log levels in `Logging.*`, replacing ad hoc scan-log toggles, with tests for filtering.
5. Extract low-risk `FcsData`/diagnostic helpers, adding tests as each helper moves.
6. Split inventory, appearance, and animal replacement helpers into runtime modules.
7. Only then reshape `ApplyRaceChangeRef` into early-return orchestration with named path helpers.
