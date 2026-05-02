---
applyTo: "KenshiLib_Examples.sln,StatModification_Extension/StatModification_Extension.vcxproj,StatModification_Extension/build.bat,StatModification_FCS/StatModification_FCS.csproj,StatModification_Extension/StatModification_Extension/RE_Kenshi.json,StatModification_Extension/StatModification_Extension/FCS_extended.json"
---
# Build And Deploy

## Commands
- Runtime DLL: `StatModification_Extension\build.bat`
- Runtime unit tests: `StatModification_Tests\run_tests.bat`
- FCS helper DLL: `dotnet build StatModification_FCS/StatModification_FCS.csproj -c Release`
- Code intelligence DB: `powershell -ExecutionPolicy Bypass -File tools/generate_compile_commands.ps1`
- Direct C++ target: `StatModification_Extension/StatModification_Extension.vcxproj` as `Release|x64`
- Outputs:
  - `StatModification_Extension/x64/Release/StatModification_Extension.dll`
  - `StatModification_Tests/x64/Release/StatModification_Tests.exe`
  - `StatModification_FCS/bin/Release/net4.8/StatModification_FCS.dll`

## Required C++ settings
- Toolchain: `Release|x64`, Platform Toolset `v100`, Unicode.
- Include dirs: `$(KENSHILIB_DIR)\Include;$(BOOST_INCLUDE_PATH);$(IncludePath)`
- Library dirs: `$(KENSHILIB_DIR)\Libraries;$(LibraryPath)`
- Link deps: `kenshilib.lib;%(AdditionalDependencies)`
- `BOOST_INCLUDE_PATH` = extracted Boost root, e.g. `C:\boost_1_60_0`; not zip, not nested `boost\`.
- `StatModification_Tests` uses Boost.Test header-only from `BOOST_INCLUDE_PATH`; keep tests on pure seams unless a runtime probe is explicitly required.

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
- VS GUI and VS Code/MSBuild script builds both can produce loadable DLLs; still verify in game after hook/signature/deploy changes.
- Compile success != hook proof. Kenshi load/execution is proof.
- `LNK1107` on `kenshilib.lib` usually means Git LFS pointer; run `git lfs install`, then `git lfs pull`.
- `C4482`: plain C enum values cannot use `EnumName::Value`; remove the prefix.
- `E0276` in Ogre headers usually IntelliSense noise if MSBuild succeeds.
