# Agent Context

Read only what task needs. No bulk reads.

## Quick Routes

- Build/env: [README.md](README.md) -> [docs/agent-rules/build-and-deploy.md](docs/agent-rules/build-and-deploy.md) -> [RE_KENSHI setup/README.md](RE_KENSHI%20setup/README.md) if needed.
- StatModification runtime: [StatModification_Extension/README.md](StatModification_Extension/README.md) -> [docs/agent-rules/stat-modification-contract.md](docs/agent-rules/stat-modification-contract.md) -> [docs/agent-rules/re-kenshi-runtime.md](docs/agent-rules/re-kenshi-runtime.md).
- FCS/schema: [wiki/FCS-Schema-Reference.md](wiki/FCS-Schema-Reference.md) -> [docs/agent-rules/fcs-schema.md](docs/agent-rules/fcs-schema.md).
- Dialogue identity evidence: [DialogueIdentityProbe/FINDINGS.md](DialogueIdentityProbe/FINDINGS.md) + [DialogueIdentityProbe/ASSERTIONS.md](DialogueIdentityProbe/ASSERTIONS.md) -> [docs/agent-rules/dialogue-identity.md](docs/agent-rules/dialogue-identity.md).
- RaceChange runtime: [RaceChange_Extension/README.md](RaceChange_Extension/README.md) -> [RaceChange_Extension/AGENTS.md](RaceChange_Extension/AGENTS.md) -> [RaceChange_Extension/TEST_PLAN.md](RaceChange_Extension/TEST_PLAN.md). Active refactor only: also load [RaceChange_Extension/NEXT_AGENT_HANDOVER.md](RaceChange_Extension/NEXT_AGENT_HANDOVER.md).

## Supported Surfaces

- Mod-author compat surface: StatModification FCS item types, condition IDs, action keys, field names, documented wiki workflow.
- Plugin-author compat surface: documented StatModification contract, build/deploy commands, dialogue identity findings.
- DialogueIdentityProbe is evidence/regression only. Not runtime dependency.
- StatModification_FCS is editor helper only. Runtime must not depend on it.
- Internal src layout, helper names, hooks are internal unless file says public.
- PluginExport/PluginImport coupling is experiment, not ecosystem contract.
- RaceChange_Extension public surface is separate from StatModification; use RaceChange docs only.

## Prune Guardrails

- Keep anchors unless task explicitly deletes/replaces: README.md, AGENTS.md, docs/agent-rules/, tools/, StatModification_Extension/, StatModification_FCS/, DialogueIdentityProbe/, wiki/.
- After C++ project remove/rename: regenerate compile DB + reindex Serena.
- If examples removed: keep surviving support-boundary docs.
- Prefer keeping tested RE_Kenshi evidence docs over redundant examples.

## C++ Code Intelligence

- Generate compile DB: powershell -ExecutionPolicy Bypass -File tools/generate_compile_commands.ps1
- Refresh after broad C++ changes: powershell -ExecutionPolicy Bypass -File tools/refresh_serena.ps1
- If compile_commands.json already good, run serena project index or VS Code task Reindex Serena.
- Ignore warning flood "Unhandled method 'window/logMessage'"; check exit code.
- Serena strong for narrow symbol/reference work; weak for random header browsing.
- For raw deps/header reads use read_file or rg.
- Do not re-debug Serena setup unless clearly broken.

## Workflow Notes

- Start from surviving product surface, not legacy examples.
- Dialogue targeting claims must cite DialogueIdentityProbe findings/assertions.
- Keep StatModification public contract separate from internal C++ details.
- Direct CharStats::getStatRef(...) writes are intentional behavior, not bug.
- If proposing ecosystem support, state target audience (mod author vs plugin author vs editor-helper) and non-goals.
