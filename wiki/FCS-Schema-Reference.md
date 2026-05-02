# FCS Schema Reference

Complete reference for all record types and dialogue actions defined by StatModification_Extension.

---

## Record types

### STAT_DEFINITION (type ID 3000)

Identifies a stat that can be targeted by skill actions. The canonical records for all vanilla Kenshi stats are shipped with this mod. You should not need to create these unless you are adding support for a custom stat from another plugin.

Found in the FCS tree under: `Characters > Skill Adjustments > Stat Definitions`

| Field | Type | Description |
|---|---|---|
| `enum value` | int | The `StatsEnumerated` integer for this stat. See [Conditions](Conditions) for vanilla values. |
| `comments` | string | Author-facing notes. Ignored by the runtime. |

---

### CLAMP_PROFILE (type ID 3001)

A reusable clamping policy referenced by `ADJUST_SKILL_LEVEL` and `SET_SKILL_LEVEL` records. If an action record has no `CLAMP_PROFILE` reference, no clamping is applied.

Found in the FCS tree under: `Characters > Skill Adjustments > Clamp Profiles`

| Field | Type | Default | Description |
|---|---|---|---|
| `clamp min` | int | 0 | Inclusive lower bound. The stat will not be set below this value. |
| `clamp max` | int | 100 | Inclusive upper bound. The stat will not be set above this value. |
| `comments` | string | | Author-facing notes. Ignored by the runtime. |

---

### ADJUST_SKILL_LEVEL (type ID 3002)

Configuration for add and remove skill actions. The amount to add or subtract is typed inline on the dialogue action row, not stored on this record.

Found in the FCS tree under: `Characters > Skill Adjustments`

| Field | Type | Description |
|---|---|---|
| `stat` | STAT_DEFINITION reference | Which stat to modify. Pick a canonical STAT_DEFINITION record. The Val 0 shown next to this field is unused. |
| `clamp profile` | CLAMP_PROFILE reference | Optional. Leave empty for no clamping. The Val 0 shown next to this field is unused. |
| `comments` | string | Author-facing notes. Ignored by the runtime. |

---

### SET_SKILL_LEVEL (type ID 3003)

Configuration for set skill actions. The target value is referenced inline on the dialogue action row, not stored on this record.

Found in the FCS tree under: `Characters > Skill Sets`

| Field | Type | Description |
|---|---|---|
| `stat` | STAT_DEFINITION reference | Which stat to set. Pick a canonical STAT_DEFINITION record. The Val 0 shown next to this field is unused. |
| `clamp profile` | CLAMP_PROFILE reference | Optional. Leave empty for no clamping. The Val 0 shown next to this field is unused. |
| `comments` | string | Author-facing notes. Ignored by the runtime. |

---

## Dialogue actions

All actions are available on `DIALOGUE`, `DIALOGUE_LINE`, and `WORD_SWAPS` items.

The number typed on the action row is `val0`. It is always read literally - `0` means `0`, there is no sentinel behavior.

### Train / untrain skill levels

| Action key | Target |
|---|---|
| `train skill levels` | The line speaker resolved from the FCS `speaker` dropdown |
| `untrain skill levels` | The line speaker resolved from the FCS `speaker` dropdown |
| `train other skill levels` | The opposite dialogue side |
| `untrain other skill levels` | The opposite dialogue side |
| `train squad skill levels` | Every active member of the resolved line speaker's platoon |
| `untrain squad skill levels` | Every active member of the resolved line speaker's platoon |
| `train other squad skill levels` | Every active member of the opposite dialogue side's platoon |
| `untrain other squad skill levels` | Every active member of the opposite dialogue side's platoon |

`val0` = number of levels to train or untrain. Use a positive number.

The referenced record must be an `ADJUST_SKILL_LEVEL` record.

### Train / untrain until a skill level

| Action key | Target |
|---|---|
| `train skill levels until` | The line speaker resolved from the FCS `speaker` dropdown |
| `untrain skill levels until` | The line speaker resolved from the FCS `speaker` dropdown |
| `train other skill levels until` | The opposite dialogue side |
| `untrain other skill levels until` | The opposite dialogue side |
| `train squad skill levels until` | Every active member of the resolved line speaker's platoon |
| `untrain squad skill levels until` | Every active member of the resolved line speaker's platoon |
| `train other squad skill levels until` | Every active member of the opposite dialogue side's platoon |
| `untrain other squad skill levels until` | Every active member of the opposite dialogue side's platoon |

`val0` = the exact value to assign. The "train" and "untrain" wording is for FCS readability: both set the stat exactly to `val0`.

The referenced record must be a `SET_SKILL_LEVEL` record.

---

## Target semantics

| Target name | Resolved as |
|---|---|
| speaker | `dlg->getSpeaker(dialogLine->speaker, dialogLine, false)` - asks Kenshi to resolve the line's `speaker` field, including roles such as `T_ME`, `T_TARGET`, `T_INTERJECTOR1`, `T_TARGET_IF_PLAYER`, `T_TARGET_WITH_RACE`, and `T_WHOLE_SQUAD`. |
| target | `dlg->getConversationTarget().getCharacter()` - the current conversation/event target. This is often the player in player-initiated talk, but not always. |
| owner | `dlg->me` - the character that owns the active dialogue/package, regardless of who is speaking the current line. |
| other | The other main dialogue side: if the resolved speaker is owner, other is target. Otherwise, other is owner. |
| speaker squad | Resolve `speaker`, then enumerate active characters in that character's platoon. |
| other squad | Resolve `other`, then enumerate active characters in that character's platoon. |

Actions fire only after the dialogue line executes. The `speaker` role follows the line's authored `speaker` field at runtime. Root-node visual speech can still appear under the owner/NPC even when runtime speaker resolution picks a target-side character; child text nodes generally display under the resolved speaker.

The public action API is speaker-first. Use the line's `speaker` dropdown to choose who trains or untrains. Use the `... other ...` actions when the line speaker is giving training to, or taking training from, the other main dialogue side.

`other` means the character on the other side of the main conversation. It does not mean "anyone who is not speaking." If you need to affect an interjector or race-selected squadmate, make that character the line speaker and use the plain speaker action.

`T_WHOLE_SQUAD` through `speaker` resolution means one Kenshi-selected character, not every squad member. In current probes it resolved owner/NPC-side. `T_TARGET_WITH_RACE` uses the line's `target race` list and may prevent the line from executing if no matching target-side character exists.

Use the explicit `... squad ...` action keys when you want platoon-wide training or setting. Those actions do real platoon enumeration from the resolved speaker or other-side character instead of relying on the `T_WHOLE_SQUAD` speaker enum.

---

## FCS editor helper

`StatModification_FCS.dll` is optional but recommended for authoring conditions. It teaches FCS Extended that this plugin's condition `tag` field is a StatModification skill enum value, so authors can pick an exposed stat by name instead of typing the integer enum value.

Runtime behavior does not depend on the helper. The C++ plugin still reads the integer stored in the condition tag field.

