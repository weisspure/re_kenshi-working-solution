---
applyTo: "**/*.cpp,**/*.vcxproj,**/*.h"
---
# RE_Kenshi Build Setup

Toolchain: **Release x64, Platform Toolset v100, Unicode charset.** Never use Debug. Copy `HelloWorld` project rather than creating from scratch.

## Paths
- C/C++ Additional Includes: `$(KENSHILIB_DIR)\Include;$(BOOST_INCLUDE_PATH);$(IncludePath)`
- Linker Lib Dirs: `$(KENSHILIB_DIR)\Libraries;$(LibraryPath)`
- Linker Dependencies: `kenshilib.lib;%(AdditionalDependencies)`
- `BOOST_INCLUDE_PATH` = extracted boost root (e.g. `C:\boost_1_60_0`) - not the zip, not the `boost\` subfolder

## Gotchas
- **LNK1107 on KenshiLib.lib** = Git LFS pointer, not real binary. Run `git lfs install` then `git lfs pull`.
- **C4482** = plain C enum used with scope qualifier (`EnumName::Value`). Remove the `EnumName::` prefix.
- **C4018** = signed/unsigned mismatch on `.size()`. Cast to `(int)`.
- **E0276 in Ogre headers** = IntelliSense false positive. Build succeeds.

