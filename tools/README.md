# Tooling Helpers

These scripts are optional. They are here for AI coding tools, Serena, clangd, and anyone who wants better C++ navigation outside Visual Studio.

They are not required to build the plugins.

## StatModification unit tests

From the repo root:

```cmd
StatModification_Tests\run_tests.bat
```

This builds and runs the `StatModification_Tests` v100 console test project. The project uses Boost.Test from `BOOST_INCLUDE_PATH` in header-only mode, so it does not add a separate test framework binary dependency.

## Refresh Serena

From the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File tools\refresh_serena.ps1
```

This regenerates `compile_commands.json`, then runs:

```powershell
serena project index
```

If Serena is not installed or not on `PATH`, the compile database is still generated before the script reports the missing `serena` command.

## Generate compile_commands.json only

```powershell
powershell -ExecutionPolicy Bypass -File tools\generate_compile_commands.ps1
```

The generator scans the repo for remaining `*.vcxproj` files, reads their Release x64 include paths and defines, and writes a local `compile_commands.json`.

The generated file is ignored by git because it contains absolute paths. Re-run the script after moving the repo, changing dependency paths, adding C++ files, or deleting projects.

## Dependency path inference

The generator first uses environment variables when they are available:

- `KENSHILIB_DIR`
- `KENSHILIB_DEPS_DIR`
- `BOOST_INCLUDE_PATH`
- `BOOST_ROOT`

If those are missing, it starts from the current repo path and looks for a deps folder next to it:

```text
<some folder>\<repo name>
<some folder>\<repo name>_deps
```

For example, if this repo is at `D:\Modding\KenshiLib_Examples`, the script will try `D:\Modding\KenshiLib_Examples_deps`.

Inside that deps folder it searches for:

- a KenshiLib folder containing `Include` and `Libraries`
- a Boost root folder named `boost_*` containing `boost` and `stage\lib`

It also infers `$(SolutionDir)` as the repo root so local project-to-project include paths resolve correctly.

## Serena MCP file tools and KenshiLib headers

Serena's symbol search and pattern search tools need the deps folder to be **inside the project root**. Create a directory junction once:

```cmd
cd /d C:\Git\KenshiLib_Examples
mklink /J deps ..\KenshiLib_Examples_deps
```

The junction is excluded from git via `.git/info/exclude` (local only — never committed). It is **not** in `.gitignore`, which is intentional: Serena respects `.gitignore` via `ignore_all_files_in_gitignore: true` in `.serena/project.yml`, so putting `deps` in `.gitignore` would silently prevent Serena from traversing it.

Boost headers inside the junction are excluded from Serena indexing via `ignored_paths` in `.serena/project.yml` to avoid scanning ~36k files. KenshiLib headers under `deps/KenshiLib/Include/` are indexed.

After creating the junction, reindex:

```powershell
serena project index
```

clangd does not need the junction — it uses absolute paths in `compile_commands.json` directly.
