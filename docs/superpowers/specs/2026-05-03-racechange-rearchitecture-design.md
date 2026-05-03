# RaceChange Rearchitecture Design

Date: 2026-05-03

## Goal

Refactor `RaceChange_Extension` into smaller runtime modules while preserving the current working behavior. The code should be production-ready enough to maintain, but annotated as a demo project for junior C++ contributors learning a test-driven workflow in a hostile runtime environment.

The selected approach is extraction with local cleanup: move the current working behavior into focused modules, and simplify signatures, branches, and control flow only when the call graph and failure cases prove it is safe.

## Non-Goals

- Do not redesign the public FCS authoring contract.
- Do not change `value[0]` meanings. `0` remains humanoid/default intent; `1` remains animal/non-humanoid intent.
- Do not merge humanoid in-place mutation with animal spawn-and-replace behind vague generic helpers.
- Do not remove inventory validation or character editor refresh without in-game verification.
- Do not imply animal replacement performs full character migration. Supported transplant remains name, stats, and known common runtime state.

## Target Layout

Use directories where they make ownership clearer. The preferred layout is:

- `src/actions/inventory/InventoryActions.*`
- `src/actions/appearance/AppearanceActions.*`
- `src/actions/animal/AnimalRaceActions.*`

Existing core/pure support can stay flat for now unless a later cleanup proves a directory adds clarity.

## Target Modules

### `src/actions/inventory/InventoryActions.*`

Owns inventory and equipment policy:

- Remove all inventory items before animal replacement.
- Drop evacuated items for animal replacement.
- Restore all removed items only for paths that intentionally use that policy.
- Remove armour before humanoid race mutation.
- Restore armour after humanoid race mutation.
- Describe item state for diagnostics.

The module must keep the policy split explicit. Full inventory evacuation plus drop is not equivalent to armour-only evacuation plus restore.

### `src/actions/appearance/AppearanceActions.*`

Owns live appearance repair after race data changes:

- Reset appearance data for a target race.
- Replace the appearance `race` reference safely.
- Validate race-derived inventory sections.
- Open or reopen the vanilla character editor for the final active character.

This module should document that editor refresh is behavioral, not cosmetic. Runtime testing showed stale slots/appearance without this sequence.

### `src/actions/animal/AnimalRaceActions.*`

Owns animal replacement details:

- Find an `ANIMAL_CHARACTER` template for a target `RACE`.
- Spawn replacement animals from templates.
- Transfer supported state from source to replacement.
- Destroy the source character only after replacement is ready.

This module should make ownership and lifetime hazards explicit. Kenshi runtime objects are nullable and can be stale or malformed.

### `RaceActions.*`

Becomes orchestration:

1. Scan dialogue action references.
2. Resolve action role and target character.
3. Resolve target race.
4. Classify intent.
5. Select path.
6. Run humanoid mutation or animal replacement.
7. Finalize inventory, appearance, UI, and logging.

`ApplyRaceChangeRef` should read as guard clauses plus named path calls. It should not contain low-level inventory, appearance, or spawn mechanics.

## Cleanup Rules

### Signature Pruning

When moving a helper, check every argument:

- Remove unused parameters.
- Remove parameters always passed with one value if the value is path policy rather than caller data.
- Keep parameters that express an invariant across hostile input scenarios.
- Prefer clear path-specific functions over boolean parameters such as `isAnimalPath`.

### Branch Collapse

Collapse branches only when they resolve identically for:

- Null character.
- Null target race.
- Wrong `GameData` type.
- Unsupported `value[0]`.
- Missing animal template.
- Failed spawn.
- Missing inventory, `ou`, `player`, platoon, or factory.

If two branches differ by ownership, inventory policy, source destruction, or editor target, keep them separate and name them clearly.

### Control Flow

Prefer guard clauses:

- Return early on missing data or unsupported state.
- Use named helpers for path execution.
- Avoid nested `if`, `if/else`, and nested `if/else` chains unless they make state progression clearer than early returns.

## Testing Strategy

Use TDD for each extraction:

1. Add one failing test or concrete verification case for the smallest moved decision.
2. Run the narrow check and confirm the expected failure.
3. Move or introduce the minimum code.
4. Re-run the narrow check.
5. Run `RaceChange_Extension\build.bat` and `RaceChange_Tests\run_tests.bat` before claiming safety.

Pure decisions should be unit tested in `RaceChange_Tests`:

- Action key role mapping.
- `value[0]` intent parsing.
- Path selection and fallback decisions.
- Log level parsing/filtering.
- Any new branch-collapse helper.

Runtime-only Kenshi behavior should be isolated behind small shells and documented with expected manual verification:

- Humanoid to humanoid with armour.
- Humanoid to animal with `value[0] == 1`, armour, and inventory.
- Animal-intent fallback when no valid template exists.

Failure scenarios must be documented by tests where possible and by comments/manual checks where direct tests are impractical.

## Documentation Standard

Headers stay concise and explain module contracts. Source comments explain runtime hazards and behavior that a junior C++ reader would not infer:

- Which functions mutate game state.
- What is safe on null input.
- Which pointers are borrowed from Kenshi and not owned by the plugin.
- Why `value[0]` matters.
- Why source destruction is delayed until replacement is ready.
- Why inventory slots and editor refresh are part of behavior.

Comments should teach intent and hazards, not restate syntax.

## Implementation Order

1. Add pure tests for any path-selection or policy helper needed by the extraction.
2. Extract `src/actions/inventory/InventoryActions.*`, preserving current policy and logs.
3. Extract `src/actions/appearance/AppearanceActions.*`, preserving validation and editor refresh behavior.
4. Extract `src/actions/animal/AnimalRaceActions.*`, preserving template lookup, spawn, transfer, and delayed source destruction.
5. Reshape `ApplyRaceChangeRef` into orchestration with guard clauses.
6. Build and run tests after each module move.
7. Refresh compile commands after project membership changes.

Code edits should be coordinated through one main integration path because `RaceActions.cpp` and `RaceChange_Extension.vcxproj` are shared write points.

## Acceptance Criteria

- `RaceActions.cpp` no longer owns low-level inventory, appearance, or animal spawn mechanics.
- New modules are included in `RaceChange_Extension.vcxproj`.
- Unit tests document pure decisions and failure behavior.
- Existing build and test scripts pass.
- Public behavior documented in `RaceChange_Extension/AGENTS.md`, `README.md`, and `TEST_PLAN.md` remains accurate.
- Manual in-game checks are listed for any runtime path touched but not directly unit-testable.
