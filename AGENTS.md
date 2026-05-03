# Agent Context

Lazy-load only needed docs. Do not bulk-read spike/setup docs unless task asks.

## Start Here

- Build/env: [README.md](README.md); deeper duplicate/expanded guide: [RE_KENSHI setup/README.md](RE_KENSHI%20setup/README.md).
- Runtime/plugin examples: [HelloWorld](HelloWorld/README.md), [NewPlugin](NewPlugin/README.md), [KillButton](KillButton/README.md), [SkillIncrease](SkillIncrease/README.md), [Dialogue](Dialogue/README.md), [WorldStates](WorldStates/README.md).
- StatModification overview: [StatModification_Extension/README.md](StatModification_Extension/README.md).
- RaceChange human overview: [RaceChange_Extension/README.md](RaceChange_Extension/README.md); agent notes: [RaceChange_Extension/AGENTS.md](RaceChange_Extension/AGENTS.md); cleanup handover: [RaceChange_Extension/NEXT_AGENT_HANDOVER.md](RaceChange_Extension/NEXT_AGENT_HANDOVER.md).

## Supported Surfaces

- Safe compatibility surface for mod authors: `StatModification_Extension` FCS item types, condition IDs, action keys, field names, and the documented author workflow in the wiki.
- Safe compatibility surface for plugin/tool authors: the documented StatModification contract, the documented build/deploy commands, and the tested dialogue identity findings.
- `DialogueIdentityProbe/` is evidence and regression guidance, not a runtime dependency. Keep spike-only hooks and probing code out of `StatModification_Extension`.
- `StatModification_FCS/` is editor support for FCS Extended only. Runtime behavior must not depend on the C# helper being present.
- Internal `src/` helper layout, C++ function names, and hook implementation details are not a public API unless a file explicitly says otherwise.
- `PluginExport/` and `PluginImport/` show direct DLL import/export coupling between plugins. Treat that as an experiment, not as a recommended ecosystem contract.
- `RaceChange_Extension` is intentionally separate from `StatModification_Extension`. Treat its public authoring surface as the direct dialogue action keys and `RACE` references documented in `RaceChange_Extension/README.md` and `RaceChange_Extension/AGENTS.md`, not as part of the StatModification compatibility contract.

## If You Prune The Repo

- Keep these anchors unless the task is explicitly deleting or replacing them: `README.md`, `AGENTS.md`, `.github/instructions/`, `tools/`, `StatModification_Extension/`, `StatModification_FCS/`, `DialogueIdentityProbe/`, and `wiki/`.
- After removing or renaming C++ projects, regenerate the compile database and reindex Serena.
- When examples disappear, preserve the surviving project's support boundary docs before deleting reference material that future agents would have used to infer intent.
- If a remaining file documents tested RE_Kenshi behavior, prefer keeping the evidence doc over keeping a redundant sample project.

## StatModification Docs

- Wiki home: [wiki/Home.md](wiki/Home.md).
- Mod author workflow: [wiki/For-Mod-Authors.md](wiki/For-Mod-Authors.md).
- Plugin author compatibility: [wiki/For-Plugin-Authors.md](wiki/For-Plugin-Authors.md).
- FCS schema: [wiki/FCS-Schema-Reference.md](wiki/FCS-Schema-Reference.md).
- Conditions: [wiki/Conditions.md](wiki/Conditions.md).

## Dialogue Identity Evidence

- Probe usage: [DialogueIdentityProbe/README.md](DialogueIdentityProbe/README.md).
- Main findings: [DialogueIdentityProbe/FINDINGS.md](DialogueIdentityProbe/FINDINGS.md).
- Assertion matrix: [DialogueIdentityProbe/ASSERTIONS.md](DialogueIdentityProbe/ASSERTIONS.md).
- Log audit: [DialogueIdentityProbe/LOG_AUDIT.md](DialogueIdentityProbe/LOG_AUDIT.md).
- Review handoff: [DialogueIdentityProbe/HANDOFF_FOR_REVIEW.md](DialogueIdentityProbe/HANDOFF_FOR_REVIEW.md).

## AI Instruction Files

- Build/deploy: [.github/instructions/build.instructions.md](.github/instructions/build.instructions.md).
- Dialogue identity: [.github/instructions/dialogue-identity.instructions.md](.github/instructions/dialogue-identity.instructions.md).
- FCS schema: [.github/instructions/fcs-schema.instructions.md](.github/instructions/fcs-schema.instructions.md).
- RE_Kenshi runtime: [.github/instructions/re-kenshi-runtime.instructions.md](.github/instructions/re-kenshi-runtime.instructions.md).
- StatModification contract: [.github/instructions/stat-modification.instructions.md](.github/instructions/stat-modification.instructions.md).

## Lazy-Load Routes

- Build/deploy: `README.md` -> `build.instructions.md`.
- StatModification runtime: `StatModification_Extension/README.md` -> `stat-modification.instructions.md` -> `re-kenshi-runtime.instructions.md`.
- FCS/schema/generated data: `wiki/FCS-Schema-Reference.md` -> `fcs-schema.instructions.md`.
- Mod docs: `wiki/For-Mod-Authors.md` + `wiki/Conditions.md` + `wiki/FCS-Schema-Reference.md`.
- Plugin compatibility: `wiki/For-Plugin-Authors.md` + `StatModification_Extension/README.md`.
- Dialogue targeting: `DialogueIdentityProbe/FINDINGS.md` + `DialogueIdentityProbe/ASSERTIONS.md` + `dialogue-identity.instructions.md`.
- RaceChange runtime/cleanup: `RaceChange_Extension/AGENTS.md` -> `RaceChange_Extension/NEXT_AGENT_HANDOVER.md` -> `RaceChange_Extension/TEST_PLAN.md`.

## C++ Code Intelligence

- Generate local clangd/Serena database: `powershell -ExecutionPolicy Bypass -File tools/generate_compile_commands.ps1`.
- Generated `compile_commands.json` is local and ignored; it contains absolute paths from `KENSHILIB_DIR` and `BOOST_INCLUDE_PATH`.
- Serena project is configured in `.serena/project.yml` with `cpp` and `csharp_omnisharp`.
- One-step refresh after broad C++ changes: `powershell -ExecutionPolicy Bypass -File tools/refresh_serena.ps1`.
- Run the refresh after file moves, source splits/merges, `*.vcxproj` edits, include-path/dependency changes, or large refactors that change compile membership.
- If only Serena indexing needs a refresh and `compile_commands.json` is already known-good, re-index directly with `serena project index` or the VS Code task `Reindex Serena`.
- Treat Serena repo setup as already solved in this workspace. Do not spend time re-debugging junctions, MCP wiring, or basic indexing setup unless something is clearly broken.
- When reindexing, do not read or paste the full `serena project index` output into chat unless the command actually fails. It is noisy and wastes tokens.
- `serena project index` output is flooded with `WARNING … Unhandled method 'window/logMessage'` lines. This is normal. Check the exit code, not the warning count.
- Serena is best here for narrow symbol work after indexing: find a specific function/type, inspect references, and stay local to one code path.
- Serena is weaker here for arbitrary header browsing. For raw deps-header inspection or awkward header cases, prefer targeted `read_file` / `rg` style reads.
- Codex MCP setup: add `serena start-mcp-server --project-from-cwd --context=codex` to `~/.codex/config.toml`; `serena setup codex` may fail if Codex is installed through the VS Code extension.
- VS Code/Copilot workspace MCP setup: use stdio command `serena start-mcp-server --context=vscode --project ${workspaceFolder}`.
- Copilot CLI setup: `/mcp add`, STDIO, command `serena start-mcp-server --context=copilot-cli --project-from-cwd`.

## Workflow Notes For Future Agents

- Start from the surviving product surface, not from legacy examples, when answering design or implementation questions.
- For RE_Kenshi dialogue targeting questions, use `DialogueIdentityProbe/FINDINGS.md` and `ASSERTIONS.md` as the evidence base before making claims about speaker semantics.
- For StatModification changes, preserve the distinction between public FCS contract and internal C++ implementation.
- For StatModification action semantics, remember that direct `CharStats::getStatRef(...)` writes are intentional product behavior. Do not frame XP-path APIs as a bug fix; if proposing them, present them as separate optional action types with distinct semantics.
- If continuing a broad investigation or multi-step implementation, load the relevant workspace skills first. Prefer the `using-superpowers` skill at session start, then `systematic-debugging`, `test-driven-development`, or `writing-plans` when the task shape matches.
- If proposing new support for third-party coders, document whether it is meant for mod authors, plugin authors, or editor-helper authors, and state the non-goals explicitly.
