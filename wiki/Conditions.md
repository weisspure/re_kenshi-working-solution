# Conditions

StatModification_Extension adds two custom dialogue conditions for comparing character stat levels.

---

## Available conditions

| Condition | ID | What it checks |
|---|---|---|
| `DC_STAT_LEVEL_COMPARE_UNMODIFIED` | 3004 | Two characters' raw base stats against each other |
| `DC_STAT_LEVEL_COMPARE_MODIFIED` | 3005 | Two characters' effective stats against each other |

Single-character stat threshold checks are intentionally not duplicated here. Use BFrizzle's Dialogue conditions for checks like "NPC Strength > 70". StatModification_Extension only adds the comparison conditions that Dialogue does not provide.

---

## Setting up a comparison condition in the FCS

In the FCS dialogue condition editor, a comparison condition row has four fields:

| Field | What to set |
|---|---|
| Who | Controls the left-hand side. `T_ME` = owner OP target. Any other supported value = target OP owner. |
| Condition | `DC_STAT_LEVEL_COMPARE_UNMODIFIED` or `DC_STAT_LEVEL_COMPARE_MODIFIED` |
| Tag | The `StatsEnumerated` integer for the stat (same stat is read from both characters) |
| Value | Unused - leave at 0 |

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

## Example: trainer must outskill the student

An NPC trainer who only offers to teach Strength if their own base Strength exceeds the player's:

```
Who:       T_ME
Condition: DC_STAT_LEVEL_COMPARE_UNMODIFIED
Tag:       1   (Strength)
Compare:   more than
Value:     0   (unused)
```

Reads as: owner's base Strength > target's base Strength.

To reverse the check (player must be weaker than the trainer), use `T_ME` with `more than`. To check that the player is already stronger, set `who` to any non-`T_ME` value with `more than` - this flips the sides so it reads target > owner.

---

## Known limitation

The `tag` field currently shows as a plain number input in the FCS rather than a named dropdown. A future C# FCS plugin could add a `StatsEnumerated` dropdown. For now, use the integer table above.

`T_WHOLE_SQUAD` is not supported as a `who` value for these conditions.

Future pass: add improved single-character threshold conditions under the next available IDs if they support behavior Dialogue does not, such as squad-aware checks.

