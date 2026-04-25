# Conditions

StatModification_Extension adds two custom dialogue conditions for checking character stat levels.

---

## Available conditions

| Condition | ID | What it checks |
|---|---|---|
| `DC_STAT_UNMODIFIED` | 3004 | One character's raw base stat against a fixed threshold |
| `DC_STAT_MODIFIED` | 3005 | One character's effective stat (including bonuses) against a fixed threshold |
| `DC_STAT_COMPARE_UNMODIFIED` | 3006 | Two characters' raw base stats against each other |
| `DC_STAT_COMPARE_MODIFIED` | 3007 | Two characters' effective stats against each other |

**Use `DC_STAT_UNMODIFIED`** when you want to check what a character actually knows — their trained skill level independent of gear. This is the right choice for trainer qualification checks ("does this NPC know Strength well enough to teach it?").

**Use `DC_STAT_MODIFIED`** when you want to check what a character is effectively capable of in the moment, including equipment bonuses.

---

## Setting up a condition in the FCS

In the FCS dialogue condition editor, a stat condition row has four fields:

| Field | What to set |
|---|---|
| Who | `T_ME` to check the dialogue owner. Any other value checks the conversation target. |
| Condition | `DC_STAT_UNMODIFIED` or `DC_STAT_MODIFIED` |
| Tag | The `StatsEnumerated` integer for the stat you want to check (see table below) |
| Value | The threshold to compare against |

The comparison operator (equals / less than / greater than) is set separately on the condition row.

### Tag values for vanilla stats

These are the fixed integers for each vanilla Kenshi stat. Copy the number into the `tag` field.

| Integer | Stat |
|---|---|
| 1 | Strength |
| 2 | Melee Attack |
| 3 | Labouring |
| 4 | Science |
| 5 | Engineering |
| 6 | Robotics |
| 7 | Weapon Smithing |
| 8 | Armour Smithing |
| 9 | Medic |
| 10 | Thievery |
| 11 | Turrets |
| 12 | Farming |
| 13 | Cooking |
| 14 | Hive Medic |
| 15 | Vet |
| 16 | Stealth |
| 17 | Athletics |
| 18 | Dexterity |
| 19 | Melee Defence |
| 21 | Toughness |
| 22 | Assassination |
| 23 | Swimming |
| 24 | Perception |
| 25 | Katanas |
| 26 | Sabres |
| 27 | Hackers |
| 28 | Heavy Weapons |
| 29 | Blunt |
| 30 | Martial Arts |
| 32 | Dodge |
| 33 | Survival |
| 34 | Polearms |
| 35 | Crossbows |
| 36 | Precision Shooting |
| 37 | Lockpicking |
| 38 | Bow Smithing |

For custom stats added by another plugin, use the integer that plugin publishes for its stat.

---

## Example: trainer qualification check

A trainer NPC who only offers training if their own Strength is at least 70 (base, not gear-boosted):

```
Who:       T_ME
Condition: DC_STAT_UNMODIFIED
Tag:       1
Compare:   greater than
Value:     70
```

This condition is placed on the dialogue line that offers training. If the NPC's base Strength is 70 or below, the line does not appear.

---

## Known limitation

The `tag` field currently shows as a plain number input in the FCS rather than a named dropdown. A future C# FCS plugin could add a `StatsEnumerated` dropdown. For now, use the integer table above.

`T_WHOLE_SQUAD` is not supported as a `who` value for these conditions.

---

## Comparison conditions

`DC_STAT_COMPARE_UNMODIFIED` and `DC_STAT_COMPARE_MODIFIED` compare the same stat on two characters rather than comparing one character against a fixed number. This covers cases like "is this trainer more skilled than the student?" without needing to know either character's actual stat value at authoring time.

### Field setup

| Field | What to set |
|---|---|
| Who | Controls the left-hand side. `T_ME` = owner OP target. Any other value = target OP owner. |
| Condition | `DC_STAT_COMPARE_UNMODIFIED` or `DC_STAT_COMPARE_MODIFIED` |
| Tag | The `StatsEnumerated` integer for the stat (same stat is read from both characters) |
| Value | Unused — leave at 0 |

### Example: trainer must outskill the student

An NPC trainer who only offers to teach Strength if their own base Strength exceeds the player's:

```
Who:       T_ME
Condition: DC_STAT_COMPARE_UNMODIFIED
Tag:       1   (Strength)
Compare:   more than
Value:     0   (unused)
```

Reads as: owner's base Strength > target's base Strength.

To reverse the check (player must be weaker than the trainer), use `T_ME` with `more than`. To check that the player is already stronger, set `who` to any non-`T_ME` value with `more than` — this flips the sides so it reads target > owner.
