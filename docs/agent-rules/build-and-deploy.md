# Build And Deploy

Use for: solution/project build/deploy setup and verification.

## Commands
- Runtime build: `StatModification_Extension\build.bat`
- Runtime tests: `StatModification_Tests\run_tests.bat`
- FCS helper build: `dotnet build StatModification_FCS/StatModification_FCS.csproj -c Release`
- Compile DB: `powershell -ExecutionPolicy Bypass -File tools/generate_compile_commands.ps1`
- Direct C++ target: `StatModification_Extension/StatModification_Extension.vcxproj` (`Release|x64`)

## Expected Outputs
- `StatModification_Extension/x64/Release/StatModification_Extension.dll`
- `StatModification_Tests/x64/Release/StatModification_Tests.exe`
- `StatModification_FCS/bin/Release/net4.8/StatModification_FCS.dll`

## Required C++ Settings
- Toolchain: `Release|x64`, Platform Toolset `v100`, Unicode.
- Include dirs: `$(KENSHILIB_DIR)\Include;$(BOOST_INCLUDE_PATH);$(IncludePath)`
- Library dirs: `$(KENSHILIB_DIR)\Libraries;$(LibraryPath)`
- Link deps: `kenshilib.lib;%(AdditionalDependencies)`
- `BOOST_INCLUDE_PATH` must be extracted Boost root (example: `C:\boost_1_60_0`), not zip and not nested `boost\`.
- `StatModification_Tests` uses header-only Boost.Test from `BOOST_INCLUDE_PATH`; keep tests on pure seams unless runtime probe is explicitly required.

## Deploy Layout
```text
Kenshi/mods/StatModification_Extension/
  RE_Kenshi.json                  loads StatModification_Extension.dll
  FCS_extended.json               loads StatModification_FCS.dll
  fcs.def                         FCS schema
  StatModification_Extension.mod  canonical records
  StatModification_Extension.dll  C++ runtime hooks
  StatModification_FCS.dll        FCS condition stat dropdown helper
```

## Gotchas
- Build success is not runtime proof; verify in game after hook/signature/deploy changes.
- `LNK1107` on `kenshilib.lib` usually means Git LFS pointer; run `git lfs install` then `git lfs pull`.
- `C4482`: plain C enum values cannot use `EnumName::Value`; remove enum-name prefix.
- `E0276` in Ogre headers is usually IntelliSense noise if MSBuild succeeds.
