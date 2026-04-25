---
applyTo: "StatModification_Extension/**,wiki/**"
---
# StatModification_Extension - Design Reference

## Type IDs (fcs.def and C++ enum must match exactly)
| Name | ID | Purpose |
|---|---|---|
| STAT_DEFINITION | 3000 | Which stat - memorable anchor, other plugins need this number |
| CLAMP_PROFILE | 3001 | Clamping policy |
| ADJUST_SKILL_LEVEL | 3002 | Add/remove actions |
| SET_SKILL_LEVEL | 3003 | Set actions |
| DC_STAT_UNMODIFIED | 3004 | Condition: one character's base stat vs fixed threshold |
| DC_STAT_MODIFIED | 3005 | Condition: one character's effective stat vs fixed threshold |
| DC_STAT_COMPARE_UNMODIFIED | 3006 | Condition: compare two characters' base stats |
| DC_STAT_COMPARE_MODIFIED | 3007 | Condition: compare two characters' effective stats |

## Design rules - do not regress
- No stat whitelist - `STAT_DEFINITION.enum_value` cast directly to `StatsEnumerated`, no validation beyond rejecting STAT_NONE
- No default clamping - absent clamp profile = unclamped; canonical records ship both clamped and unclamped variants
- 0 means 0 - no sentinel behavior
- No stringID fallback
- No presets or default amounts
- Explicit dispatcher - fixed named action keys, no dynamic registry

## Condition `who` semantics (BFrizzle-compatible)
- `T_ME` = owner (NPC) is subject / left-hand side
- Any other value = target (player) is subject / left-hand side
- Compare conditions (3006/3007): `who == T_ME` → left=me, right=target; evaluates `left OP right`
- T_WHOLE_SQUAD not supported for our conditions yet - unresolved how `getConversationTarget().getCharacter()` behaves when dialogue target is a squad rather than an individual

## Excluded StatsEnumerated values
| Value | Reason |
|---|---|
| STAT_NONE (0) | Sentinel - always reject |
| STAT_WEAPONS (20) | Composite/aggregate, not directly trainable |
| STAT_HIVEMEDIC (14) | Not in CharStats struct - never shipped, exclude |
| STAT_VET (15) | Not in CharStats struct - never shipped, exclude |
| STAT_MASSCOMBAT (31) | Internal, not player-facing, exclude |
| STAT_SURVIVAL (33) | Internal, not player-facing, exclude |
| STAT_END (39) | Sentinel - always reject |

**STAT_FRIENDLY_FIRE (36) = "Precision Shooting" - confirmed real trainable stat, INCLUDE.**

## In-game name corrections for canonical records
- STAT_MEDIC (9) → record name **"Field Medic"**
- STAT_SMITHING_BOW (38) → record name **"Crossbow Smith"**

## Outstanding work
- **Canonical .mod records** not yet authored: 34 STAT_DEFINITION records (one per trainable stat), 1 CLAMP_PROFILE ("Default 0-100"), example ADJUST_SKILL_LEVEL and SET_SKILL_LEVEL pairs (clamped + unclamped) for each stat
- **StatModification_FCS C# project** not yet written - needed for named StatsEnumerated dropdown on condition `tag` field; conditions work without it (authors type integer)
- **Squad target behavior** unresolved - what `getConversationTarget().getCharacter()` returns when dialogue target is a squad rather than an individual has not been tested

## Deployment layout
```
Kenshi/mods/StatModification_Extension/
  RE_Kenshi.json                  { "Plugins": ["StatModification_Extension.dll"] }
  StatModification_Extension.dll  C++ runtime hooks
  fcs.def                         schema
  StatModification_Extension.mod  canonical records
  [future] FCS_extended.json      { "FCS_Plugins": ["StatModification_FCS.dll"] }
  [future] StatModification_FCS.dll
```

## Wiki pages (wiki/ directory)
Home.md, For-Mod-Authors.md, Conditions.md, FCS-Schema-Reference.md, For-Plugin-Authors.md
