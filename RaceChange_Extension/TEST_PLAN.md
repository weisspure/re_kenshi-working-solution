# RaceChange Extension Test Plan

This plugin runs inside Kenshi through hooks, so unit tests only cover the pure plugin decisions. Runtime confidence comes from a small manual matrix plus captured `RE_Kenshi.log` evidence.

## Unit Coverage

Covered by `RaceChange_Tests/run_tests.bat`:

- `change race` maps to the resolved line speaker.
- `change other race` maps to the other main dialogue side.
- Unknown action keys are not treated as race-change actions.
- Only the public race-change action keys are recognized.
- The plugin's local validation gate only blocks missing character, missing race reference, or wrong referenced type.
- `value[0]` classifies `0` as humanoid/default intent and `1` as animal intent.
- Logging level names parse case-insensitively and filtering emits only the configured threshold and more severe messages.

Not covered by unit tests:

- Kenshi's `Dialogue::_doActions` call timing.
- `Dialogue::getSpeaker(...)` runtime identity choices.
- `Character::setRace(...)` persistence and side effects.
- Character editor safety from inside `_doActions`.
- Appearance reset behavior.
- Animal template lookup, spawning, state transplant, source-character destruction, and inventory drop behavior.
- Save/reload behavior.

## Dry Run Setup

1. Build with `RaceChange_Extension/build.bat`.
2. Copy `RaceChange_Extension/RaceChange_Extension/` to `[Kenshi install dir]/mods/RaceChange_Extension/`.
3. Ensure the copied mod folder contains:
   - `RaceChange_Extension.dll`
   - `RE_Kenshi.json`
   - `fcs.def`
   - `RaceChange_Extension.mod`
4. Enable `RaceChange_Extension` when launching Kenshi with RE_Kenshi.
5. Use a throwaway save.

## First Safe Case

Author a dialogue reply similar to the vanilla plastic surgeon reply that uses `DA_CHARACTER_EDITOR`.

Recommended first action:

- FCS line speaker: `T_TARGET`
- Action: `change race`
- Reference: a vanilla editor-supported `RACE` subrace, first try `Scorchlander`
- Test character: a normal player Greenlander

Expected immediate behavior:

- Dialogue action fires once.
- `RE_Kenshi.log` has `RaceChange: changing race`.
- The log includes the resolved character, before race, and target race.
- If the character was wearing armour, `RE_Kenshi.log` has `RaceChange: unequipping armour before race change` before the race mutation.
- `RE_Kenshi.log` has `RaceChange: changed race`.
- The after race should match the target race.
- `RE_Kenshi.log` has `RaceChange: resetting appearance data`.
- `RE_Kenshi.log` has `RaceChange: reset appearance data`.
- `RE_Kenshi.log` has `RaceChange: validating inventory sections after race change`.
- `RE_Kenshi.log` has `opening character editor through PlayerInterface::activateCharacterEditMode`.
- The character editor opens.

## Manual Matrix

Run these as separate throwaway-save tests:

- `change race`, child reply, player Greenlander to Scorchlander.
- `change race`, child reply, player Greenlander to Hive Prince.
- `change other race`, NPC-owned line, target player to Scorchlander.
- Cancel editor after race change, then inspect live race/appearance.
- Accept editor after race change, then save and reload.

Optional edge probes:

- A playable but editor-ungrouped subrace such as Fishman, if available.
- A non-humanoid subrace such as Bonedog with `value[0] == 1`, in a disposable save only. Expected current behavior is spawn-and-replace when an `ANIMAL_CHARACTER` template references the target race; if no template is found, the fallback in-place mutation can retain the old `CharacterHuman` object limitation.
- An invalid or unsupported animal-template case, to confirm the fallback is logged and does not destroy the source character before a replacement exists.
- A subrace with no known editor limits file, in a disposable test mod only.
- Root dialogue line vs child dialogue line, because dialogue identity differs by line position.

## Evidence To Capture

For each run, copy the relevant `RE_Kenshi.log` block into `RaceChangePOC/FINDINGS.md` or a project-local findings note.

Capture:

- FCS line location and speaker setting.
- Action key and referenced `RACE`.
- `value[0]` intent, especially whether animal tests use `1`.
- Character before race/subrace.
- Log lines from `RaceChange: changing race` through editor invocation.
- Whether the editor opened.
- Whether the editor displayed/reset appearance for the target subrace.
- For animal replacement, whether inventory was dropped, the spawned template matched the target race, supported state was transplanted, the source was destroyed only after spawn success, and the editor opened for the spawned character.
- What happened after cancel/accept.
- Save/reload result.

## Stop Conditions

Stop and inspect logs before continuing if:

- The action fires more than once for one reply.
- The resolved character is not the intended player character.
- `afterRace` does not match the referenced target race.
- The appearance reset logs are absent after `changed race`.
- The editor does not open.
- The game crashes or hangs inside `_doActions`.
- Appearance appears mismatched after accepting the editor.
