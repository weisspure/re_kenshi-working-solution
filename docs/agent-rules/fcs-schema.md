# FCS Schema Rules

Use for: `fcs.def`, FCS plugin json, and schema reference updates.

## Syntax
- Custom item type: `enum itemType { NAME=int }`
- Custom dialogue condition: `enum DialogConditionEnum { NAME=int }`
- Type block: `[TYPE_NAME]`
- Scalar field: `fieldname: default "tooltip"`
- Multiline string: `fieldname: "" multiline "tooltip"`
- Reference picker: `fieldname: TYPE (default) "tooltip"`; FCS still shows unused Val 0 spinner.
- Dialogue action key: define under `[DIALOGUE,DIALOGUE_LINE,WORD_SWAPS]`.

## Layout Rules
- Every `FCS_LAYOUT` entry needs type; label-only entries can crash FCS Extended.
- `Characters: CHARACTER` merges with vanilla node; use for custom records under Characters.
- Empty `enum DialogConditionEnum { }` before later declarations is normal merge behavior.

## StatModification Schema Contract
- `fcs.def`, `src/Constants.h`, `StatModification_FCS/StatModificationFcsPlugin.cs`, and docs must use same custom IDs.
- Action strings in `fcs.def` must exactly match `src/Constants.h`.
- `STAT_DEFINITION["enum value"]` is the only runtime stat selector.
- `ADJUST_SKILL_LEVEL` and `SET_SKILL_LEVEL` reference `STAT_DEFINITION` plus optional `CLAMP_PROFILE`.
- `CLAMP_PROFILE` is opt-in; do not imply clamp defaults in tooltips.
- Condition `tag` stores `StatsEnumerated` int; `StatModification_FCS.dll` renders StatModification skill dropdown in FCS Extended.

## Deployment Files
- `RE_Kenshi.json`: `{ "Plugins": ["StatModification_Extension.dll"] }`
- `FCS_extended.json`: `{ "FCS_Plugins": ["StatModification_FCS.dll"] }`
- Both JSON files must sit beside `fcs.def`, `.mod`, and DLLs in enabled Kenshi mod folder.
