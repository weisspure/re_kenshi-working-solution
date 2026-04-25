# StatModification_Extension (WIP)

**NOTE: This documentation is a work in progress. Expect missing information and broken links and out of date information.**
A RE_Kenshi runtime plugin that adds dialogue actions and comparison conditions for modifying character skill stats directly from the FCS editor.

## What it does

Allows FCS dialogue authors to write dialogue lines that:

- **Add** a flat number of levels to a character's skill
- **Remove** a flat number of levels from a character's skill
- **Set** a character's skill to an exact value
- **Compare** skill levels between dialogue participants

All of this is authored entirely in the FCS editor with no scripting required.

## Who this is for

| Audience | What to read |
|---|---|
| Mod authors using this in dialogue | [For Mod Authors](For-Mod-Authors) |
| Authors using skill conditions | [Conditions](Conditions) |
| Plugin authors adding custom stats | [For Plugin Authors](For-Plugin-Authors) |
| Anyone who wants the full field reference | [FCS Schema Reference](FCS-Schema-Reference) |

## How it works

The plugin hooks `Dialogue::_doActions` and `DialogLineData::checkTags` in the Kenshi runtime. Custom record types are defined in `fcs.def` and appear in the FCS editor under `Characters > Skill Adjustments` and `Characters > Skill Sets`. The plugin reads those records at runtime and applies the stat changes.

No FCS C# plugin is required for the actions. Comparison conditions work without a C# plugin but display a raw number input for the stat tag field rather than a named dropdown.

## Four record types

| Type | ID | Purpose |
|---|---|---|
| `STAT_DEFINITION` | 3000 | Identifies which stat to target |
| `CLAMP_PROFILE` | 3001 | Reusable clamping policy |
| `ADJUST_SKILL_LEVEL` | 3002 | Configuration for add/remove actions |
| `SET_SKILL_LEVEL` | 3003 | Configuration for set actions |

And two comparison dialogue conditions:

| Condition | ID | Purpose |
|---|---|---|
| `DC_STAT_LEVEL_COMPARE_UNMODIFIED` | 3004 | Compares two characters' raw base stat values |
| `DC_STAT_LEVEL_COMPARE_MODIFIED` | 3005 | Compares two characters' effective stat values including bonuses |

Single-character stat threshold checks are intentionally not duplicated in pass 1. Use BFrizzle's Dialogue conditions for those checks. A future pass may add improved threshold checks under new IDs if they support behavior Dialogue does not, such as squad-aware checks.

