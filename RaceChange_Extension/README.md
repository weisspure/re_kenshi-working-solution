# RaceChange_Extension

`RaceChange_Extension` is a small RE_Kenshi proof-of-concept plugin for dialogue-driven race/subrace changes.

It adds two direct dialogue actions that reference existing `RACE` subrace records:

- `change race`
- `change other race`

The action reference value controls transform intent:

- `0`: humanoid/playable race change.
- `1`: experimental animal/non-humanoid intent. When the plugin can find a matching animal template, it spawns an animal replacement instead of only changing the live humanoid's race data.

## Known Limitations

- This is a proof of concept. Test on a throwaway save.
- Animal transforms are intentionally modest: name, stats, and known common runtime state are moved to the spawned animal, but this is not full character migration.
- If animal intent cannot find a matching `ANIMAL_CHARACTER` template, the plugin falls back to the in-place race-change path.
- Animal replacement drops evacuated inventory items to the ground before destroying the source character.

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
