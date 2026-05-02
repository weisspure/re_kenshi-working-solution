# StatModification_Extension (WIP)

**NOTE: This documentation is a work in progress. Expect missing information and broken links and out of date information.**
A RE_Kenshi runtime plugin that adds dialogue actions and comparison conditions for modifying character skill stats directly from the FCS editor.

## What it does

Allows FCS dialogue authors to write dialogue lines that:

- **Train** a flat number of levels onto the line speaker or the other dialogue side
- **Untrain** a flat number of levels from the line speaker or the other dialogue side
- **Train/untrain until** an exact skill value
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

No FCS C# plugin is required for the actions. Comparison conditions work without the C# helper too, but `StatModification_FCS.dll` makes the condition stat tag field display as a named StatModification skill dropdown instead of a raw number input.

## Four record types

| Type | ID | Purpose |
|---|---|---|
| `STAT_DEFINITION` | 3000 | Identifies which stat to target |
| `CLAMP_PROFILE` | 3001 | Reusable clamping policy |
| `ADJUST_SKILL_LEVEL` | 3002 | Configuration for train/untrain actions |
| `SET_SKILL_LEVEL` | 3003 | Configuration for train/untrain until actions |

And two comparison dialogue conditions:

| Condition | ID | Purpose |
|---|---|---|
| `DC_STAT_LEVEL_COMPARE_UNMODIFIED` | 3004 | Compares two characters' raw base stat values |
| `DC_STAT_LEVEL_COMPARE_MODIFIED` | 3005 | Compares two characters' effective stat values including bonuses |

Single-character stat threshold checks are not implemented in the current pass. Use BFrizzle's Dialogue conditions for those checks. A future pass may add threshold checks under new IDs if they add enough value, such as clearer logging or real squad-aware behavior.

