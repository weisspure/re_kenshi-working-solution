# Pruning + TDD quick refs

## Keep
- AGENTS.md
- README.md
- StatModification_Extension/README.md
- docs/agent-rules/{stat-modification-contract,re-kenshi-runtime,dialogue-identity}.md
- DialogueIdentityProbe/{README,FINDINGS,ASSERTIONS}.md

## Prune policy
- Prune aggressively outside support-boundary docs.
- Keep evidence docs over redundant samples.

## Serena refresh
- compile DB: tools/generate_compile_commands.ps1
- reindex: tools/refresh_serena.ps1
- run only after broad C++/project-shape changes.

## TDD gap
- Main gap is fast local red-green harness/docs around pure seams:
  - action-key/target resolution
  - FCS parsing/validation helpers
  - hook-adjacent decision logic
