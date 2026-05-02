# For Plugin Authors

This page covers how to add compatibility between StatModification_Extension and a RE_Kenshi plugin that introduces custom `StatsEnumerated` values.

---

## How stat identity works

StatModification_Extension does not use a hardcoded whitelist of stats. When a dialogue author picks an `ADJUST_SKILL_LEVEL` or `SET_SKILL_LEVEL` record, that record has a reference to a `STAT_DEFINITION` record. The `STAT_DEFINITION` record stores a single integer in its `enum value` field. At runtime the plugin reads that integer and casts it directly to `StatsEnumerated`.

This means any valid `StatsEnumerated` integer, including ones added by third-party plugins, will work without any changes to StatModification_Extension.

This open-ended behavior is runtime behavior. The optional FCS helper dropdown for comparison condition tags is editor support. Custom stats should ship their own companion FCS helper when you want your stats to appear in the condition `tag` dropdown instead of requiring authors to type integers manually.

## How stat mutation works

StatModification_Extension's existing train/untrain actions are direct stat writes, not XP rewards. Runtime uses `CharStats::getStatRef(...)` and writes the resulting float directly, with an optional `CLAMP_PROFILE` applied afterward.

That is an intentional compatibility and authoring choice, not an approximation of Kenshi's native progression rules. It keeps dialogue rewards deterministic, allows flat level rewards, and lets authors opt into vanilla-like limits only when they want them.

KenshiLib does expose XP-oriented `CharStats` entry points, so a future plugin version could add separate action types that award XP through Kenshi's progression path. If that ever happens, treat those as additional semantics with different author intent, not as a migration away from direct-write actions.

---

## Dialogue identity and targeting

If you are writing or reviewing a RE_Kenshi plugin that uses dialogue hooks, keep two ideas separate:

- what the player sees in the dialogue window
- which character Kenshi resolves internally for the current line

StatModification_Extension applies actions using Kenshi's runtime speaker resolution:

```cpp
dlg->getSpeaker(dialogLine->speaker, dialogLine, false)
```

That value comes from the FCS line `speaker` dropdown. It is not always the same as the visible name shown in the in-game dialogue window.

For beginners, the safe rule is:

```text
When code needs to know who a dialogue line applies to, trust Kenshi's resolved speaker, not only the on-screen name.
```

Clean probe findings:

- `Dialogue::me` is the active dialogue/package owner.
- `dlg->getConversationTarget().getCharacter()` is the current conversation target or detected target-side character. It is not always the player.
- `getSpeaker(line->speaker, ...)` is meaningful for authored speaker roles.
- Parent/root dialogue nodes can be visually owner/NPC-presented even when runtime speaker resolution selects a target-side character or interjector.
- Child dialogue nodes usually display closer to the resolved runtime speaker.
- `parentMonologue` did not block dialogue windows, choices, runtime speaker resolution, or action execution in the tested flows.

For action targeting, this plugin uses these public roles:

| Public role | Runtime meaning |
|---|---|
| speaker | `getSpeaker(dialogLine->speaker, dialogLine, false)` |
| other | if speaker is `dlg->me`, use conversation target; otherwise use `dlg->me` |
| speaker squad | active platoon members for the resolved speaker |
| other squad | active platoon members for the resolved other side |

`other` is intentionally a simple two-main-sides helper. It is not a full participant graph. If an author wants to affect an interjector or a race-selected squadmate, they should put that role in the FCS line `speaker` dropdown and use a plain speaker action.

The squad roles are only used by explicit `... squad ...` action keys. They resolve the same anchor character as `speaker` or `other`, then enumerate that character's active platoon. They are not a reinterpretation of the `T_WHOLE_SQUAD` speaker enum.

Special cases:

- `T_TARGET_IF_PLAYER` can resolve null when the relevant target is not player-controlled. That is expected.
- `T_TARGET_WITH_RACE` uses the line's `target race` list and can select a matching target-side character that is not the current conversation target.
- `T_WHOLE_SQUAD` through `getSpeaker(...)` resolves one character, not a squad collection. Do not treat it as true squad-wide behavior.

For the current evidence, see `DialogueIdentityProbe/FINDINGS.md`.

If you are new to C++ plugins, use the findings above as the working model and avoid inventing targeting rules from screenshots alone.

---

## Choose your dependency model

The easiest option is to make your mod depend on StatModification_Extension and author normal records using this mod's FCS schema.

If you do not want your main mod to depend on StatModification_Extension, the recommended soft-compat option is a separate compatibility patch mod that depends on both mods. That patch can contain the canonical records for your custom stats while leaving your main mod usable on its own.

Advanced authors can also declare StatModification_Extension's custom item types in their own `fcs.def` and ship contract-conforming records directly. This is possible in theory, but currently hypothetical and untested. It is not the preferred route: it squats on shared enum IDs, can confuse FCS if schemas diverge, and those records only have runtime behavior when StatModification_Extension is installed and loaded.

Do not rely on duck-typed records. Runtime expects the custom item types from this plugin's contract, especially `STAT_DEFINITION` and `CLAMP_PROFILE`.

If you choose the enum/type-squatting route, copy the contract exactly: item type IDs, field names, field types, action keys, and condition IDs must match StatModification_Extension's `fcs.def`. Squatting can make records loadable and runtime-compatible, but it does not extend the `StatModification_FCS.dll` condition dropdown. For custom stat condition authoring, ship a companion FCS helper that registers an editor-facing enum for the same condition IDs.

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

### 3. Ship a companion FCS helper for condition dropdowns

Comparison conditions store the stat in the dialogue condition `tag` field as the same `StatsEnumerated` integer. The runtime accepts custom stat integers there too.

For good authoring UX, ship a small FCS Extended helper DLL beside your RE_Kenshi plugin. Its job is to make your custom stat enum show up as the dropdown for:

- `DC_STAT_LEVEL_COMPARE_UNMODIFIED` (`3004`)
- `DC_STAT_LEVEL_COMPARE_MODIFIED` (`3005`)

The enum names are what authors see in the dropdown. The enum integers must be your real `StatsEnumerated` values:

```csharp
enum MyStatConditionTagEnum
{
    MY_STAT_WILLPOWER = 1100,
    MY_STAT_ENDURANCE = 1101,
}
```

Patch `forgotten_construction_set.dialog.ConditionControl.createDefaults`, get the static `conditionDefaults` dictionary, and set the default object for both StatModification condition IDs to one of your enum values. The default object's enum type is what FCS Extended uses to build the dropdown.

If that sounds abstract, the job of the helper is just:

```text
tell FCS which enum type to use for the condition tag dropdown
```

The runtime plugin still reads an integer. The helper only improves the editor experience.

Use replacement behavior, not add-only behavior, because `StatModification_FCS.dll` may already have registered its own dropdown:

```csharp
object conditionDefaults = Traverse
    .Create(AccessTools.TypeByName("forgotten_construction_set.dialog.ConditionControl"))
    .Field("conditionDefaults")
    .GetValue();

Type conditionEnumType = AccessTools.TypeByName("forgotten_construction_set.DialogConditionEnum");
Type dictionaryType = typeof(Dictionary<,>).MakeGenericType(conditionEnumType, typeof(object));
PropertyInfo itemProperty = dictionaryType.GetProperty("Item");

object defaultTag = MyStatConditionTagEnum.MY_STAT_WILLPOWER;
object unmodifiedKey = Enum.ToObject(conditionEnumType, 3004);
object modifiedKey = Enum.ToObject(conditionEnumType, 3005);

itemProperty.SetValue(conditionDefaults, defaultTag, new object[] { unmodifiedKey });
itemProperty.SetValue(conditionDefaults, defaultTag, new object[] { modifiedKey });
```

That helper needs the same package pattern as `StatModification_FCS`: reference `BFrizzleFoShizzle.FCS_extended.Plugin` and `Lib.Harmony`, implement `IPlugin`, create a `Harmony` instance in `Init`, and include the DLL in your mod folder through `FCS_extended.json`.

If you are just starting out, copy that pattern first and change the enum and condition IDs second. Do not start by designing a more abstract system.

FCS Extended stores one default tag value per condition ID, so condition dropdown helpers do not merge automatically. StatModification's bundled helper uses add-only registration, while companion helpers should use replacement. That means your helper can load before or after `StatModification_FCS.dll` and still own the dropdown. Runtime still reads the stored integer either way.

If you want authors to pick both vanilla StatModification skills and your custom skills from one dropdown, include both sets in your companion enum and use the same integer values. That combined helper becomes the first-class compatibility experience for your plugin.

### 4. If you do not ship records

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
| `ADJUST_SKILL_LEVEL` | 3002 | Configures train/untrain actions |
| `SET_SKILL_LEVEL` | 3003 | Configures train/untrain-until actions |

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
