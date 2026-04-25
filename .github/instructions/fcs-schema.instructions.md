---
applyTo: "**/*.def,**/*.json"
---
# FCS Schema Rules

## fcs.def field syntax
- `enum itemType { NAME=int }` - custom record type
- `enum DialogConditionEnum { NAME=int }` - extends condition dropdown  
- `[TYPE_NAME]` - opens field block for that type
- `fieldname: default "tooltip"` - string/int/float/bool field
- `fieldname: "" multiline "tooltip"` - string with textarea popup in FCS editor
- `fieldname: TYPE (default) "tooltip"` - reference picker (always shows unused Val 0 spinner beside it - document it as unused in tooltip)
- Actions under `[DIALOGUE,DIALOGUE_LINE,WORD_SWAPS]`: `key: TYPE (default) "tooltip"`

## FCS_LAYOUT rules
- **Every entry must have a type.** Labels without types are not valid.
- Declaring `Characters: CHARACTER` in a mod's fcs.def **merges** with the vanilla node - no duplication.
- Nesting under a type already used as a vanilla sub-node (e.g. `Stats: STATS`) **does** duplicate - nest under the parent container type instead.

## Loading and deployment
Both files live in the same mod folder alongside the `.mod` file:
- C++ runtime plugin: `RE_Kenshi.json` → `{ "Plugins": ["name.dll"] }`
- C# FCS editor plugin: `FCS_extended.json` → `{ "FCS_Plugins": ["name.dll"] }`

Both activate together when the user enables the mod. No separate installation step.

## Known pattern
Seeing `enum DialogConditionEnum { }` followed by a full declaration is normal - it's the FCS merging fcs.def files from multiple loaded mods.
