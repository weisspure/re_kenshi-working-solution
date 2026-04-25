# StatModification_Extension (WIP)

**NOTE: This documentation is a work in progress. Expect missing information and broken links and out of date information.**
A RE_Kenshi runtime plugin that adds dialogue actions and conditions for modifying character skill stats directly from the FCS editor.

## What it does

Allows FCS dialogue authors to write dialogue lines that:

- **Add** a flat number of levels to a character's skill
- **Remove** a flat number of levels from a character's skill
- **Set** a character's skill to an exact value
- **Check** a character's skill level as a dialogue condition

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

No FCS C# plugin is required for the actions. Conditions work without a C# plugin but display a raw number input for the stat tag field rather than a named dropdown.

## Four record types

| Type | ID | Purpose |
|---|---|---|
| `STAT_DEFINITION` | 3000 | Identifies which stat to target |
| `CLAMP_PROFILE` | 3001 | Reusable clamping policy |
| `ADJUST_SKILL_LEVEL` | 3002 | Configuration for add/remove actions |
| `SET_SKILL_LEVEL` | 3003 | Configuration for set actions |

And two dialogue conditions:

| Condition | ID | Purpose |
|---|---|---|
| `DC_STAT_UNMODIFIED` | 3004 | Checks a stat's raw base value |
| `DC_STAT_MODIFIED` | 3005 | Checks a stat's effective value including bonuses |
