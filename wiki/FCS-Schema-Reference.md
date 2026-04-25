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

### Add skill levels

| Action key | Target |
|---|---|
| `add skill levels to speaker` | The character speaking the current line |
| `add skill levels to target` | The conversation partner |
| `add skill levels to owner` | The NPC who owns the dialogue package |

`val0` = number of levels to add. Use a positive number.

The referenced record must be an `ADJUST_SKILL_LEVEL` record.

---

### Remove skill levels

| Action key | Target |
|---|---|
| `remove skill levels from speaker` | The character speaking the current line |
| `remove skill levels from target` | The conversation partner |
| `remove skill levels from owner` | The NPC who owns the dialogue package |

`val0` = number of levels to remove. Use a positive number. The runtime negates it automatically.

The referenced record must be an `ADJUST_SKILL_LEVEL` record.

---

### Set skill level

| Action key | Target |
|---|---|
| `set skill level for speaker` | The character speaking the current line |
| `set skill level for target` | The conversation partner |
| `set skill level for owner` | The NPC who owns the dialogue package |

`val0` = the exact value to assign.

The referenced record must be a `SET_SKILL_LEVEL` record.

---

## Target semantics

| Target name | Resolved as |
|---|---|
| speaker | `dlg->getSpeaker(dialogLine->speaker, dialogLine, false)` - handles T_ME, T_TARGET, T_INTERJECTOR, etc. |
| target | `dlg->getConversationTarget().getCharacter()` - the player in a typical NPC conversation |
| owner | `dlg->me` - the NPC who owns the Dialogue object, regardless of who is speaking the current line |

Actions fire for every dialogue line including NPC lines. The identity of speaker, target, and owner does not change based on which side of the conversation the line is on.

