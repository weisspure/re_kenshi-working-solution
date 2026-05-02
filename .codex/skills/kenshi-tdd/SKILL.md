---
name: kenshi-tdd
description: Apply test-driven development in this KenshiLib_Examples repo without introducing incompatible modern .NET tooling.
---

# KenshiLib TDD

Use this skill when applying TDD, adding tests, fixing bugs test-first, or choosing a test framework in this repo.

## Repo Constraints

- C# FCS helper projects target `net4.8`.
- Prefer NUnit or xUnit test projects targeting `net48`.
- Do not introduce TUnit or .NET 8-only test tooling.
- `StatModification_FCS/` is editor support for FCS Extended only; runtime behavior must not depend on it.
- `DialogueIdentityProbe/` is evidence and regression guidance, not a runtime dependency.

## Workflow

1. Write one failing test or one concrete verification case for the smallest behavior.
2. Run the narrowest relevant check and confirm it fails for the expected reason.
3. Implement the minimum change needed to pass.
4. Re-run the narrow check, then any nearby build or regression check that fits the change.
5. Refactor only while checks are green.

## C# Guidance

- Put tests in a separate `[ProjectName].Tests` project.
- Target `net48` unless there is a repo-approved reason to multi-target.
- Prefer behavior-focused tests through public APIs over testing private implementation details.
- Keep FCS helper tests independent from the live Kenshi/FCS executable where practical.

## C++ Runtime Guidance

- Runtime C++ plugin behavior may be hard to unit test directly.
- Prefer narrow seams, probes, captured logs, documented manual verification, or regression evidence when direct unit tests are impractical.
- Keep spike-only probing code out of `StatModification_Extension`.
- Preserve the distinction between public compatibility contracts and internal hook/helper implementation details.
