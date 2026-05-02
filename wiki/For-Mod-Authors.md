# For Mod Authors

This page covers how to use StatModification_Extension in your own mod to add skill-modifying dialogue to NPCs.

## Prerequisites

Your mod must have StatModification_Extension loaded as a dependency. The canonical `STAT_DEFINITION` and `CLAMP_PROFILE` records must be available - these are shipped in the framework mod.

---

## The authoring workflow

Skill modification is done entirely from the dialogue line editor. You do not need to create new records for common use cases - just pick from the canonical records already provided.

### Step 1 - Add an action to a dialogue line

In the FCS dialogue editor, select your dialogue line and add one of the available actions from the right-hand actions panel. The line's `speaker` dropdown is the main thing that decides who the plain training actions apply to.

**Train or untrain the line speaker:**
- `train skill levels` - add levels to the resolved line speaker
- `untrain skill levels` - remove levels from the resolved line speaker

**Train or untrain the other main dialogue side:**
- `train other skill levels` - add levels to the other main side
- `untrain other skill levels` - remove levels from the other main side

**Train or untrain a whole active platoon:**
- `train squad skill levels` - add levels to every active member of the resolved line speaker's platoon
- `untrain squad skill levels` - remove levels from every active member of the resolved line speaker's platoon
- `train other squad skill levels` - add levels to every active member of the other main side's platoon
- `untrain other squad skill levels` - remove levels from every active member of the other main side's platoon

**Train or untrain until an exact level:**
- `train skill levels until` - set the resolved line speaker to the value
- `untrain skill levels until` - set the resolved line speaker to the value
- `train other skill levels until` - set the other main side to the value
- `untrain other skill levels until` - set the other main side to the value
- `train squad skill levels until` - set every active member of the resolved line speaker's platoon to the value
- `untrain squad skill levels until` - set every active member of the resolved line speaker's platoon to the value
- `train other squad skill levels until` - set every active member of the other main side's platoon to the value
- `untrain other squad skill levels until` - set every active member of the other main side's platoon to the value

`other` means the character on the other side of the main conversation. In normal talk-to dialogue, this usually means "the other person in the conversation." It does not mean "anyone who is not speaking." If you want to affect an interjector or specific squadmate, pick that role in the line `speaker` dropdown and use the plain `train skill levels` / `untrain skill levels` action.

The `... squad ...` actions are different from using `T_WHOLE_SQUAD` in the line `speaker` dropdown. Squad actions first resolve the speaker or other side, then apply to that character's active platoon members.

The visible name in the in-game dialogue window is not the only thing that matters. When in doubt, treat the FCS `speaker` dropdown as the source of truth for who the plain action targets.

For example, imagine a shopkeeper dialogue. In FCS, the first/root dialogue node often behaves like the shopkeeper's turn to talk. The first child dialogue node is often where the other side gets a reply/choice.

Now imagine the root line appears in game as if the shopkeeper is talking:

```
Text shown in game: "Let me show you how to swing that blade."
FCS speaker:        T_TARGET_IF_PLAYER
Action:             train skill levels
Record:             Melee Attack
val0:               5
```

Even if the visible root dialogue looks like the shopkeeper spoke the line, the action can still train the player, because `T_TARGET_IF_PLAYER` points the line at the player when the target is player-controlled.

Child dialogue nodes in FCS usually display closer to the chosen speaker in game, so this mismatch is less common one node down. The useful authoring rule is:

- first/root dialogue nodes often look like the NPC's turn
- child dialogue nodes often make the chosen speaker more obvious

For unusual dialogue layouts, test in game and use the FCS speaker setup as your guide.

`T_TARGET_WITH_RACE` has one more important rule: it uses the line's `target race` list. If the line is a child choice and one sibling targets `Greenlander` while another sibling targets `Hive Prince`, Kenshi can route into the eligible matching branch instead of showing every sibling as a selectable followup. If no matching target-side character exists, the line may be considered by dialogue checks but never execute its actions.

---

## Targeting in FCS

The most important rule is:

```text
StatModification follows the speaker selected in FCS, not just the name you see on screen.
```

The in-game dialogue window can be misleading, especially on the first/root dialogue node. In tested shopkeeper-style dialogue, root lines were shown as if the NPC spoke them, even when the FCS `speaker` dropdown pointed at the player or another target-side character.

Use this practical model:

| What you want | FCS setup |
|---|---|
| Train the character chosen by the line `speaker` dropdown | Use `train skill levels` |
| Remove levels from the character chosen by the line `speaker` dropdown | Use `untrain skill levels` |
| Set the line speaker to an exact level | Use `train skill levels until` or `untrain skill levels until` |
| Train the other main side of the conversation | Use `train other skill levels` |
| Remove levels from the other main side | Use `untrain other skill levels` |
| Set the other main side to an exact level | Use `train other skill levels until` or `untrain other skill levels until` |

### Shopkeeper examples

NPC teaches the player from a line that is clearly the shopkeeper speaking:

```text
Speaker: T_ME
Action:  train other skill levels
Record:  Strength
val0:    5
```

This reads as: the shopkeeper gives 5 Strength levels to the other main side, usually the player.

Player reply trains the player directly:

```text
Speaker: T_TARGET_IF_PLAYER
Action:  train skill levels
Record:  Melee Attack
val0:    3
```

This reads as: if the target side is player-controlled, train that player character. It can still work even if the root line visually appears under the NPC in the dialogue log.

Interjector gets trained:

```text
Speaker: T_INTERJECTOR1
Action:  train skill levels
Record:  Toughness
val0:    1
```

This only works if the dialogue flow has already populated `T_INTERJECTOR1`. If not, nothing useful is there to train.

Race-selected squadmate gets trained:

```text
Speaker:     T_TARGET_WITH_RACE
Target race: Greenlander
Action:      train skill levels
Record:      Athletics
val0:        2
```

This can train a matching target-side/squad character even when they are not the current conversation target. If nobody on the target side matches the target race list, the line may never run.

### Special speaker roles

| Speaker role | Practical meaning for actions |
|---|---|
| `T_ME` | Active dialogue/package owner, usually the NPC running the package |
| `T_TARGET` | Current conversation target or detected target-side character; not always the player |
| `T_TARGET_IF_PLAYER` | Target only if that target is player-controlled; otherwise may resolve null |
| `T_TARGET_WITH_RACE` | Matching target-side character from the line's `target race` list |
| `T_INTERJECTOR1` | Interjector slot after dialogue flow has populated it |
| `T_WHOLE_SQUAD` | One Kenshi-selected character, not every squad member |

`T_WHOLE_SQUAD` is not squad-wide training. If you use it with a speaker action, only one character gets picked.

For actual squad-wide training, use `train squad skill levels` or `train other squad skill levels` instead of relying on `T_WHOLE_SQUAD`.

### Step 2 - Type a value

The number field on the action row (`val0`) is the amount for the action.

- For train/untrain actions: the number of levels to add or subtract. Use a positive number. `0` means `0`.
- For train/untrain until actions: the exact value to assign. `0` means `0`.

### Step 3 - Pick a record

Click the record picker on the action row and select an `ADJUST_SKILL_LEVEL` or `SET_SKILL_LEVEL` record. The canonical records shipped with this mod are named after the stat they target.

For most uses, pick the clamped version (e.g. `Strength`). The unclamped version (e.g. `Strength Unclamped`) allows the stat to go outside the 0-100 range.

---

## Clamped vs unclamped

The canonical records come in pairs:

| Record name | Clamping |
|---|---|
| `Strength` | Clamped 0-100 |
| `Strength Unclamped` | No clamping |

**Use the clamped version** for normal training dialogue. The stat cannot go below 0 or above 100.

**Use the unclamped version** if you deliberately want to allow extreme values - for special events, curses, unique NPCs, etc.

If you open the record in the FCS you can see the difference: the clamped version has a `CLAMP_PROFILE` reference set to `Default 0-100`. The unclamped version has that field empty.

---

## Example: a trainer NPC

A trainer who teaches the player Strength when they pay for training. The dialogue reply line the player clicks would have:

```
Speaker: T_ME
Action:  train other skill levels
val0:   5
Record: Strength
```

This makes the trainer line give 5 levels of Strength to the other side of the conversation, usually the player, clamped to 0-100.

In normal player-clicked trainer dialogue, `T_TARGET` is usually the player. In NPC-initiated events, the target side can be another detected character. Prefer choosing the participant with the FCS `speaker` dropdown and then using either a plain action for that speaker or an `... other ...` action for the other main dialogue side.

To also check whether the player is eligible before showing the training option, see [Conditions](Conditions).

---

## Creating your own records

You only need to create your own `ADJUST_SKILL_LEVEL` or `SET_SKILL_LEVEL` records if the canonical ones do not fit your use case - for example, a custom clamp range or support for new stats. To do so:

1. Navigate to `Characters > Skill Adjustments` in the FCS tree
2. Create a new `ADJUST_SKILL_LEVEL` record
3. Set the `stat` field by picking a `STAT_DEFINITION` record from `Characters > Skill Adjustments > Stat Definitions`
4. Optionally set the `clamp profile` field by picking a `CLAMP_PROFILE` record from `Characters > Skill Adjustments > Clamp Profiles`
5. Leave `clamp profile` empty for no clamping

Use the `comments` field to document what the record is for. It is ignored by the runtime but visible to other authors in the FCS.
