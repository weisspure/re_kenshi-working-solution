# For Plugin Authors

This page covers how to add compatibility between StatModification_Extension and a RE_Kenshi plugin that introduces custom `StatsEnumerated` values.

---

## How stat identity works

StatModification_Extension does not use a hardcoded whitelist of stats. When a dialogue author picks an `ADJUST_SKILL_LEVEL` or `SET_SKILL_LEVEL` record, that record has a reference to a `STAT_DEFINITION` record. The `STAT_DEFINITION` record stores a single integer in its `enum value` field. At runtime the plugin reads that integer and casts it directly to `StatsEnumerated`.

This means any valid `StatsEnumerated` integer, including ones added by third-party plugins, will work without any changes to StatModification_Extension.

---

## Choose your dependency model

The easiest option is to make your mod depend on StatModification_Extension and author normal records using this mod's FCS schema.

If you do not want your main mod to depend on StatModification_Extension, the recommended soft-compat option is a separate compatibility patch mod that depends on both mods. That patch can contain the canonical records for your custom stats while leaving your main mod usable on its own.

Advanced authors can also declare StatModification_Extension's custom item types in their own `fcs.def` and ship contract-conforming records directly. This is possible in theory, but currently hypothetical and untested. It is not the preferred route: it squats on shared enum IDs, can confuse FCS if schemas diverge, and those records only have runtime behavior when StatModification_Extension is installed and loaded.

Do not rely on duck-typed records. Runtime expects the custom item types from this plugin's contract, especially `STAT_DEFINITION` and `CLAMP_PROFILE`.

---

## What you need to do as a plugin author

### 1. Publish your integer values

When your RE_Kenshi plugin extends `StatsEnumerated` with custom values, publish those integers in your mod's documentation. Authors cannot create compatible `STAT_DEFINITION` records without knowing the exact integer.

Example documentation format:

```
Custom StatsEnumerated values added by MyStatMod:
  MY_STAT_WILLPOWER = 1100
  MY_STAT_ENDURANCE = 1101
```

### 2. Ship canonical records

In your plugin's `.mod` file, or in a compatibility patch, create a `STAT_DEFINITION` record for each custom stat you want to expose to dialogue authors:

1. Navigate to `Characters > Skill Adjustments > Stat Definitions` in the FCS
2. Create a new `STAT_DEFINITION` record
3. Name it something descriptive, such as `Willpower`
4. Set `enum value` to the integer your plugin uses for that stat

Shipping these records yourself means authors using your plugin alongside this one get immediate access to your stats in the dialogue action picker without any extra steps.

If your stat has a normal safe range, also provide a `CLAMP_PROFILE` record. There is no default clamp for custom stats: no clamp profile means unclamped behavior.

For modder convenience, consider shipping example `ADJUST_SKILL_LEVEL` and `SET_SKILL_LEVEL` records for each custom stat. Provide clamped and unclamped variants when both are useful.

### 3. If you do not ship records

If you do not ship `STAT_DEFINITION` records, authors can still add compatibility themselves. They follow the same steps above using the integer values you have published. This is why publishing the integers matters.

---

## Important: integer stability

The integer values you assign to your custom `StatsEnumerated` entries must be stable across versions of your plugin. If you change them, existing `STAT_DEFINITION` records created by you or by authors will silently target the wrong stat at runtime.

Treat your published integers as a public API.

---

## The type IDs you need to know

| Type | ID | Purpose |
|---|---:|---|
| `STAT_DEFINITION` | 3000 | Defines which `StatsEnumerated` value to target |
| `CLAMP_PROFILE` | 3001 | Optional clamp configuration |
| `ADJUST_SKILL_LEVEL` | 3002 | Adds or removes a stat value |
| `SET_SKILL_LEVEL` | 3003 | Sets a stat value exactly |

If your plugin needs to detect or interact with these records at runtime, these are the `itemType` integers to check against.

---

## What StatModification_Extension does with the integer

For reference, the runtime code that reads your stat integer is:

```cpp
// Reads the StatsEnumerated integer from a STAT_DEFINITION record
// and casts it directly. No whitelist is applied beyond rejecting STAT_NONE (0).
static StatsEnumerated ReadStatEnum(GameData* statDef)
{
    if (statDef == 0)
        return STAT_NONE;

    return (StatsEnumerated)GetIntField(statDef, "enum value", (int)STAT_NONE);
}
```

If the cast produces `STAT_NONE` (0), the action is skipped and an error is logged. All other integers are passed directly to `CharStats::getStatRef` or `CharStats::getStat`.
