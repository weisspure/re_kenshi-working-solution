# For Mod Authors

This page covers how to use StatModification_Extension in your own mod to add skill-modifying dialogue to NPCs.

## Prerequisites

Your mod must have StatModification_Extension loaded as a dependency. The canonical `STAT_DEFINITION` and `CLAMP_PROFILE` records must be available — these are shipped in the framework mod.

---

## The authoring workflow

Skill modification is done entirely from the dialogue line editor. You do not need to create new records for common use cases — just pick from the canonical records already provided.

### Step 1 — Add an action to a dialogue line

In the FCS dialogue editor, select your dialogue line and add one of the available actions from the right-hand actions panel:

**Add skill levels:**
- `add skill levels to speaker` — the character speaking this line
- `add skill levels to target` — the conversation partner
- `add skill levels to owner` — the NPC who owns the dialogue package

**Remove skill levels:**
- `remove skill levels from speaker`
- `remove skill levels from target`
- `remove skill levels from owner`

**Set skill level exactly:**
- `set skill level for speaker`
- `set skill level for target`
- `set skill level for owner`

### Step 2 — Type a value

The number field on the action row (`val0`) is the amount for the action.

- For add/remove actions: the number of levels to add or subtract. Use a positive number. `0` means `0`.
- For set actions: the exact value to assign. `0` means `0`.

### Step 3 — Pick a record

Click the record picker on the action row and select an `ADJUST_SKILL_LEVEL` or `SET_SKILL_LEVEL` record. The canonical records shipped with this mod are named after the stat they target.

For most uses, pick the clamped version (e.g. `Strength`). The unclamped version (e.g. `Strength Unclamped`) allows the stat to go outside the 0–100 range.

---

## Clamped vs unclamped

The canonical records come in pairs:

| Record name | Clamping |
|---|---|
| `Strength` | Clamped 0–100 |
| `Strength Unclamped` | No clamping |

**Use the clamped version** for normal training dialogue. The stat cannot go below 0 or above 100.

**Use the unclamped version** if you deliberately want to allow extreme values — for special events, curses, unique NPCs, etc.

If you open the record in the FCS you can see the difference: the clamped version has a `CLAMP_PROFILE` reference set to `Default 0-100`. The unclamped version has that field empty.

---

## Example: a trainer NPC

A trainer who teaches the player Strength when they pay for training. The dialogue reply line the player clicks would have:

```
Action: add skill levels to target
val0:   5
Record: Strength
```

This adds 5 levels of Strength to the conversation target (the player), clamped to 0–100.

To also check whether the player is eligible before showing the training option, see [Conditions](Conditions).

---

## Creating your own records

You only need to create your own `ADJUST_SKILL_LEVEL` or `SET_SKILL_LEVEL` records if the canonical ones do not fit your use case — for example, a custom clamp range or support for new stats. To do so:

1. Navigate to `Characters > Skill Adjustments` in the FCS tree
2. Create a new `ADJUST_SKILL_LEVEL` record
3. Set the `stat` field by picking a `STAT_DEFINITION` record from `Characters > Skill Adjustments > Stat Definitions`
4. Optionally set the `clamp profile` field by picking a `CLAMP_PROFILE` record from `Characters > Skill Adjustments > Clamp Profiles`
5. Leave `clamp profile` empty for no clamping

Use the `comments` field to document what the record is for. It is ignored by the runtime but visible to other authors in the FCS.
