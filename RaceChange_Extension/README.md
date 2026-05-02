# RaceChange_Extension

`RaceChange_Extension` is a small RE_Kenshi proof-of-concept plugin for dialogue-driven race/subrace changes.

It is intentionally separate from `StatModification_Extension`. The public authoring surface is two direct dialogue actions that reference existing `RACE` subrace records:

- `change race`
- `change other race`

Runtime behavior:

1. Hooks `Dialogue::_doActions`.
2. Resolves the dialogue target using the same speaker-first model as `StatModification_Extension`.
3. Validates that the referenced record is a `RACE`.
4. Logs current race, target race, resolved character, and basic race diagnostics.
5. Unequips worn armour before the race mutation so items in slots the target race cannot use are not silently destroyed by inventory validation.
6. Calls `Character::setRace(targetRace)`.
7. Creates and assigns fresh appearance data for the target race.
8. Validates inventory sections so race-derived equipment slots refresh without a save reload.
9. Opens the vanilla character editor through `PlayerInterface::activateCharacterEditMode(character)`.

The first runtime dry run showed that `Character::setRace(...)` alone updates race-derived state such as limb HP, but can leave stale editor appearance data behind. The plugin now resets appearance data before opening the editor.

## Known Limitations

- This proof of concept is intended for humanoid/playable race changes. Changing a live humanoid character to a non-humanoid race can update race stats, but the runtime object is still the original `CharacterHuman`, so equipment-slot layout can remain human-like until reload or behave inconsistently.
- Worn armour is unequipped before changing race. If the character inventory has no room, vanilla inventory behavior decides whether the item can be placed or dropped.

## Install

Build:

```powershell
RaceChange_Extension\build.bat
```

Copy `RaceChange_Extension/RaceChange_Extension/` to `[Kenshi install dir]/mods/RaceChange_Extension/`.

If rebuilding, copy the built DLL from `RaceChange_Extension/x64/Release/RaceChange_Extension.dll` into the copied mod folder.

Enable `RaceChange_Extension` in Kenshi's Mods tab when launching with RE_Kenshi.

## Verification

Run the narrow unit tests:

```powershell
RaceChange_Tests\run_tests.bat
```

Runtime behavior must be verified in-game. See `TEST_PLAN.md` for the dry-run matrix and expected `RE_Kenshi.log` evidence.
