# Conditions

StatModification_Extension adds two custom dialogue conditions for comparing character stat levels.

---

## Available conditions

| Condition | ID | What it checks |
|---|---|---|
| `DC_STAT_LEVEL_COMPARE_UNMODIFIED` | 3004 | Two characters' raw base stats against each other |
| `DC_STAT_LEVEL_COMPARE_MODIFIED` | 3005 | Two characters' effective stats against each other |

Single-character stat threshold checks are not implemented in the current pass. BFrizzle has said this plugin may ship similar checks under separate enum values, but they should not reuse Dialogue's IDs. Until StatModification_Extension adds its own threshold checks, use BFrizzle's Dialogue conditions for checks like "NPC Strength > 70".

---

## Which stat condition should I use?

Use BFrizzle's Dialogue stat conditions when you want to compare one character to a fixed number:

```text
trainer Strength > 70
player Athletics < 30
```

Use StatModification_Extension's comparison conditions when you want to compare two dialogue participants to each other:

```text
trainer Strength > student Strength
player Toughness >= opponent Toughness
```

Dialogue's stat checks are a lightweight baseline. StatModification_Extension is stat-focused, so its checks are intentionally stricter:

- They compare two characters, not one character against a fixed value.
- They use the FCS `who` field to decide which side is the left-hand side.
- They fail closed and log when required condition data is malformed.
- They compare visible whole stat levels, not hidden decimal stat values.

The last point matters a lot. Kenshi stores stats internally as floats. Two characters may both appear as level `90` in the UI while internally being `90.1` and `90.5`. StatModification casts both values to integers before comparing, so those two characters compare as equal because that is what a mod author and player can actually see.

---

## Setting up a comparison condition in the FCS

In the FCS dialogue condition editor, a comparison condition row has four fields:

| Field | What to set |
|---|---|
| Who | Controls the left-hand side. `T_ME` and `T_WHOLE_SQUAD` = owner OP target. Any other supported value = target OP owner. |
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

For custom stats added by another plugin, use the integer that plugin publishes for its stat. Custom stat integers work at runtime, but they may not appear in the optional FCS helper dropdown unless another helper DLL registers them.

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

## Example: trainer and student have equal visible skill

Because StatModification compares visible whole-number levels, this works as authors would expect:

```text
Trainer internal Strength: 90.5
Student internal Strength: 90.1
Both shown in game as:   90
Condition:               DC_STAT_LEVEL_COMPARE_UNMODIFIED
Compare:                 equals
Result:                  true
```

Without this whole-number behavior, equality checks between two characters would almost never be useful.

---

## FCS enum picker

StatModification_Extension ships an FCS editor helper plugin, `StatModification_FCS.dll`. When it is loaded, the condition `tag` field uses a StatModification skill dropdown instead of the full FCS `StatsEnumerated` dropdown or a plain number box.

If the FCS helper is not installed or loaded, the runtime still works, but the `tag` field appears as a plain integer. In that case, use the integer table above.

For third-party custom stats, the runtime path is the same: the `tag` stores the published integer. Plugin authors should ship a companion FCS helper DLL if they want those custom stats to appear in this dropdown. Only one dropdown enum can own a condition ID at a time; a companion helper can replace StatModification's default enum, or expose a combined enum containing both StatModification skills and the plugin's custom stats.

## Known limitation

`T_WHOLE_SQUAD` is accepted as a `who` value, but it does not aggregate the whole squad. The condition hook only receives one evaluated `me` / `target` pair from Kenshi, so `T_WHOLE_SQUAD` behaves like `T_ME`: `owner OP target`. The runtime logs a warning when this happens.

Future pass: add self-contained threshold conditions as `DC_STAT_LEVEL_UNMODIFIED = 3006` and `DC_STAT_LEVEL_MODIFIED = 3007` if they add enough value, such as clearer logging, fail-closed malformed data, or squad-aware checks. Whole-squad threshold behavior would need a real squad/platoon implementation, not just `getSpeaker(T_WHOLE_SQUAD, ...)`, because that returns one selected character.

