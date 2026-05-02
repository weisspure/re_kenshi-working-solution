# Race Change Dialogue POC Findings

Purpose: feasibility notes for a future RE_Kenshi extension that changes a player character's race/subrace in place, then immediately opens Kenshi's character editor so the editor rebuilds race-appropriate appearance data.

This began as a code-diving POC. A first standalone runtime POC now exists in `RaceChange_Extension/`.

Implemented first runtime slice:

- New RE_Kenshi plugin folder: `RaceChange_Extension/`.
- New copyable mod package: `RaceChange_Extension/RaceChange_Extension/`.
- Direct FCS dialogue actions:
  - `change race: RACE (0)`
  - `change other race: RACE (0)`
- Runtime hook: `Dialogue::_doActions`.
- Targeting model: speaker-first, with `other` matching the StatModification-style owner/target opposite-side helper.
- Runtime behavior: validate referenced `RACE`, log diagnostics, call `Character::setRace(targetRace)`, reset appearance data for the target race, then call `PlayerInterface::activateCharacterEditMode(character)`.
- First dry-run evidence showed `Character::setRace(...)` alone updates the runtime race and race-derived limb HP, but can leave stale editor appearance data behind. A Skeleton-to-Scorchlander test entered the editor with skeleton-like editor behavior until explicit appearance reset was added.

Build and narrow tests passed locally on 2026-05-02. Runtime in-game behavior still needs the manual test script near the end of this file.

## Terminology

Kenshi/FCS terminology is slightly counterintuitive:

| Player-facing term | FCS/runtime record |
| --- | --- |
| Race | `RACE_GROUP` |
| Subrace | `RACE` |

Example: `Hive` is the race group; `Hive Prince`, `Hive Soldier Drone`, and `Hive Worker Drone` are subrace records.

## Goal

Desired player-facing flow:

1. A mod author attaches one dialogue action in FCS.
2. Runtime offers the player a race choice, then a subrace choice, plus a way to back out.
3. Runtime mutates the selected player character to the chosen race/subrace.
4. Runtime opens the normal character editor immediately, relying on the editor/appearance pipeline to reset invalid appearance data for the new race instead of hand-editing limbs, meshes, sliders, and attachments.

## Short Answer

The core mutation/editor path looks feasible enough for a TDD spike:

- `Character::setRace(GameData* r)` exists.
- `Character::getRace()` and `Character::myRace` exist.
- `Character::getAppearanceData()` and `Character::setAppearanceData(GameDataCopyStandalone*)` exist.
- `AppearanceManager::createAppearanceData(GameData* race)`, `cleanValidateAppearanceData(GameData*)`, `resetAll(GameData*, bool)`, and race editor data APIs exist.
- `ForgottenGUI::showCharacterEditor(...)` exists and accepts a character list, edit mode, and optional race filter.
- `CharacterEditWindow` explicitly supports `EDIT_MIDGAME`, race/subrace navigation, appearance-data changes, and race limits.
- `PlayerInterface::activateCharacterEditMode(Character*)` also exists and is likely the vanilla plastic-surgeon entry point.
- User-tested behavior with the NPC stub `momoso` shows the editor already rebuilds mismatched saved character-edit data for the character's current race: `momoso` is a Scorchlander in save data but has Greenlander saved character-edit data; after visiting the plastic surgeon, he becomes Scorchlander-looking and irreversibly loses the old Greenlander appearance.

The preferred "generate the whole dialogue tree dynamically from playable race/subrace groups" path is not proven from headers alone. There are APIs that might enable dynamic dialogue construction (`GameDataContainer::createNewData`, `DialogDataManager::getData`, `DialogChoiceList::add`, `Dialogue::runCustomDialog`, `Dialogue::addConversation`), but using them at runtime would couple the extension to internal dialogue data structures and line-selection behavior. It should be treated as a later experiment, not the first implementation.

The safer first product shape is a direct FCS action:

- Author adds a dialogue action that directly references an existing vanilla/modded `RACE` subrace record.
- Dialogue action mutates the resolved player character to that race/subrace.
- Runtime immediately invokes the existing character editor.
- Mod authors manually build dialogue choices at first.
- No extra race wrapper records are needed.

## Evidence

### Dialogue Actions Are A Good Trigger Point

`StatModification_Extension` already proves the pattern:

- Hook `Dialogue::_doActions`.
- Read custom action records from `DialogLineData::getGameData()->objectReferences`.
- Resolve the dialogue participant with `dlg->getSpeaker(dialogLine->speaker, dialogLine, false)` or a documented variant.
- Mutate runtime state.
- Call the original `_doActions`.

Relevant files:

- `StatModification_Extension/src/StatModification_Extension.cpp`
- `StatModification_Extension/src/Actions.cpp`
- `StatModification_Extension/src/Targets.cpp`
- `DialogueIdentityProbe/FINDINGS.md`

This suggests race mutation should be wired as another dialogue action rather than by trying to intercept the plastic surgeon's built-in action directly.

### Runtime Targeting Is Mostly Already Solved

`DialogueIdentityProbe/FINDINGS.md` gives the strongest current targeting evidence:

- `dlg->getSpeaker(...)` is meaningful runtime speaker resolution.
- `T_TARGET_IF_PLAYER` can resolve the player side and fail null in non-player contexts.
- `T_TARGET_WITH_RACE` uses the line's `target race` list and can route to a matching target-side character.
- `_doActions` is the right evidence point for "the line actually executed."

Implication: the future action should be speaker-first like StatModification. For plastic-surgeon/player use, document an authoring pattern such as `speaker = T_TARGET_IF_PLAYER` plus a plain race-change action, or `speaker = T_ME` plus a `... other ...` variant if the NPC line owns the action.

### Race Mutation Has An Exposed API

KenshiLib headers expose:

- `Character::setRace(GameData* r)` at `deps/KenshiLib/Include/kenshi/Character.h`.
- `Character::getRace() const`.
- `RaceData::getRaceData(GameData*)`.
- `RaceData::isRelatedRace(...)` and `RaceData::isSpecificRace(GameData* subrace)`.
- `RaceData::raceGroup`.

The unknown is whether `setRace(...)` alone updates every live system needed for a player character. It likely updates `Character::myRace`; it may not rebuild appearance, body, medical/body-part assumptions, stats modifiers, inventory restrictions, or UI. That is exactly why the first spike should log before/after state and then use the editor/appearance APIs.

### Appearance Reset Is Already Observed In Vanilla Editor Flow

KenshiLib headers expose `AppearanceManager` methods that line up with the requested workaround:

- `AppearanceManager::getInstance()`.
- `createAppearanceData(GameData* race)`.
- `cleanValidateAppearanceData(GameData* appearanceData)`.
- `resetAll(GameData* appearanceData, bool clearAttachments)`.
- `randomiseAll(...)`, `randomBody(...)`, `randomAnimations(...)`.
- `getEditorData(..., bool playableOnly, const Ogre::vector<GameDataReference>::type* filter)`.

`Character` also exposes:

- `getAppearanceData()`.
- `setAppearanceData(GameDataCopyStandalone* data)`.
- `getAppearance()`.

The important user-tested behavior:

- NPC stub `momoso` is a Scorchlander in save data.
- His saved character-edit appearance data is always Greenlander.
- When he goes to the plastic surgeon, the editor changes his appearance to Scorchlander.
- The previous Greenlander appearance is irreparably lost.

Interpretation: the character editor already treats race/appearance mismatch as something to validate/reset against the current saved/runtime race. That means the first implementation should not manually mutate appearance data. The desired sequence should initially be:

1. Mutate race/subrace.
2. Open the existing character editor.
3. Let vanilla editor initialization do the destructive appearance reset.

The `AppearanceManager` reset APIs remain useful as diagnostic fallback only. Using them in the product path would add unnecessary surface area unless the runtime race-change sequence fails to reproduce the observed plastic-surgeon behavior.

### Character Editor Invocation Is Exposed

KenshiLib headers expose two candidate entry points:

- `ForgottenGUI::showCharacterEditor(const lektor<Character*> characters, CharacterEditMode mode, const Ogre::vector<GameDataReference>::type* races)`.
- `PlayerInterface::activateCharacterEditMode(Character* character)`.

`ForgottenGUI::showCharacterEditor(...)` is especially promising because:

- `CharacterEditMode` includes `EDIT_MIDGAME`.
- The `CharacterEditWindow` constructor takes the same character list, mode, and optional race filter.
- `CharacterEditWindow` has `raceLimits`, `currentRaceGroupIndex`, `currentSubRaceIndex`, `races`, `racesGroups`, and `raceAppearanceData`.
- `CharacterEditWindow` has private methods for `prevRace`, `nextRace`, `prevSubRace`, `nextSubRace`, `updateRace`, `changeAppearanceData`, `resetAppearance`, `updateLiveObject`, and `confirmButton`.

`PlayerInterface::activateCharacterEditMode(Character*)` may be closer to the vanilla plastic-surgeon path. It is worth testing first if the goal is "do what the surgeon already does."

### Playable Race/Subrace Enumeration Exists

`AppearanceManager::getEditorData(...)` appears designed for this:

```text
getEditorData(racesGroups, raceAppearanceData, playableOnly, filter)
```

It returns:

- A map of `RaceGroupData*` to `FastArray<GameData*>`.
- A map of race `GameData*` to gender-specific appearance data.
- A `playableOnly` switch.
- An optional race filter.

This likely gives the exact "playable race groups and playable subraces under each race" data needed for either a custom UI or dynamic dialogue generation.

User FCS observation supports using `RACE_GROUP` records as the source of truth for editor-visible playable races:

- `Fishman` is marked playable and has editor limits, but does not appear in character edit because it is not inside a race group.
- `Hive Prince` appears under the `Hive` `RACE_GROUP`, alongside `Hive Soldier Drone` and `Hive Worker Drone`.
- Therefore dynamic enumeration, if attempted, should start from race groups and then enumerate races inside each group. It should not scan every `RACE` where `playable = true`.

The limitation: `RaceGroupData` is only forward-declared in the available headers. The editor can store and compare pointers, but code may not be able to read group names directly unless names live in the grouped `GameData` records or a missing/private header exists elsewhere.

## Dynamic Dialogue Tree Feasibility

Dynamic dialogue is possible-looking but unproven.

Promising APIs:

- `GameDataContainer::createNewData(itemType type, ...)`
- `GameData::add(...)`, `addToList(...)`, `objectReferences`
- `DialogDataManager::getData(GameData*)`
- `DialogChoiceList::add(GameData* conversation, DialogLineData* parent)`
- `Dialogue::runCustomDialog(GameData* dialog)`
- `Dialogue::addConversation(GameData* _con, EventTriggerEnum t)`
- `Dialogue::triggerNextLine(DialogLineData* previousLine)`

Risks:

- Dialogue construction is an internal runtime structure, not a documented compatibility surface.
- `DialogLineData` constructors/setup methods are protected/private; external code can ask `DialogDataManager` for parsed data, but creating correct `GameData` at runtime may be fragile.
- Dynamic child insertion must respect line selection, player reply GUI, parent pointers, locks, repeat state, speaker roles, and target race/subrace gates.
- The existing dialogue identity work showed root vs child behavior differs; dynamically emitting a tree could accidentally hit root fallback behavior or wrong speaker presentation.
- Runtime-created FCS `GameData` may not have stable source containers, string IDs, mod ownership, or save behavior unless created carefully.

Recommendation: dynamic dialogue generation is only worth pursuing if it is easy enough for the first POC. For a real mod-author API, explicit FCS-authored dialogue plus direct race action records are likely better: less runtime magic, more readable data, and easier compatibility.

## Recommended First Implementation Shape

Create a small standalone extension rather than expanding `StatModification_Extension`.

Public contract:

- Dialogue action keys:
  - `change race: RACE (0)`
  - `change other race: RACE (0)`
- No custom race-definition record type in pass 1.
- `val0` is unused unless a later pass finds a clear need.
- The referenced `GameData*` must be an existing `RACE` subrace record.
- Runtime should allow any explicitly referenced `RACE` subrace record. If the subrace is not editor-visible through a `RACE_GROUP`, log a warning/diagnostic but respect the mod author's intent.
- Supported behavior should require, or at least strongly recommend, that the referenced `RACE` has an editor limits file. Opening the character editor without editor limits may work, but it is untested and may cause editor reset/UI side effects.

Initial behavior:

1. Hook `Dialogue::_doActions`.
2. Find the custom action reference.
3. Resolve the target character with existing StatModification-style target helpers.
4. Validate target is non-null, human, and optionally player-controlled.
5. Validate referenced `GameData*` is type `RACE`.
6. Check whether the referenced subrace appears to have an editor limits file; log a warning if not.
7. Optionally log whether the referenced subrace is present in an editor-visible `RACE_GROUP`.
8. Log current race and target race.
9. Call `character->setRace(targetRace)`.
10. Open the character editor.
11. Let the editor reset appearance data for the new race.
12. Call the original `_doActions`.

A later pass can add:

- Squad variants if useful.
- A custom FCS helper for a friendlier race picker if FCS Extended needs help.
- Dynamic generated dialogue or a custom MyGUI race picker.

## Suggested TDD Spikes

### Spike 1: Logging-Only Probe

Hook `_doActions` and, for a marked action, log:

- resolved speaker/other/target identity
- current `Character::getRace()->data`
- current `Character::getAppearanceData()` race-ish fields if discoverable
- target race reference
- whether the target is player-controlled
- whether the target is human

Expected outcome: action reliably identifies the intended player character and target race.

### Spike 2: Editor Invocation Only

For a marked action, do not mutate race. Just call either:

- `PlayerInterface::activateCharacterEditMode(character)`, or
- `gui->showCharacterEditor(characterList, EDIT_MIDGAME, raceFilter)`

Expected outcome: editor opens for the selected/resolved character from dialogue. This separates "can open editor here?" from "did race mutation work?"

### Spike 3: Minimal Race Mutation

Before opening editor:

- call `character->setRace(targetRace)`.
- log before/after `getRace()`.
- open editor.

Expected outcome: if the editor resets mismatched appearance as hoped, this may already satisfy the product goal.

### Spike 4: Explicit Appearance Reset Fallback

Only run this if minimal race mutation plus editor opening fails to reproduce the `momoso` plastic-surgeon behavior:

- create target-race appearance data through `AppearanceManager::createAppearanceData(targetRace)`, or reset/validate existing data.
- assign via `character->setAppearanceData(...)`.
- then open editor.

Expected outcome: editor starts with valid target-race defaults. If the minimal path works, skip this for the product implementation.

### Spike 5: Editor-Visible Race Enumeration Diagnostics

Call `AppearanceManager::getEditorData(..., true, nullptr)` and log:

- race group count
- subrace count per group
- race `GameData` names/string IDs
- whether playable-but-ungrouped races such as Fishman are excluded

Expected outcome: confirms whether the runtime can enumerate the same race groups and subraces the editor uses. Use this for diagnostics and any future dynamic UI/dialogue design, not as a hard validation gate for explicitly authored `RACE` action references.

## Open Questions

- Does `Character::setRace(GameData*)` update all live `RaceData`-dependent systems, or only `myRace`?
- Does the character's saved-state race live solely through `Character::myRace`, or also in `GameData` fields that need mutation?
- Confirm that the subrace to pass to `Character::setRace(...)` is the `RACE` `GameData*`, with player-facing race represented by `RaceData::raceGroup`.
- Does `PlayerInterface::activateCharacterEditMode(...)` internally pass race limits from the plastic surgeon dialogue/action?
- Does `ForgottenGUI::showCharacterEditor(...)` work safely from inside `_doActions`, or should the plugin defer opening by one update tick?
- Will mutating race during active dialogue confuse the dialogue target race/subrace gates for subsequent child lines?
- Is it safer to end the dialogue before opening the editor?
- Does vanilla editor reset after `setRace(...)` preserve name, faction colors, gender, age, equipment visibility, and portrait state?
- Do race-specific body parts/medical state need explicit refresh after changing species?
- Can `AppearanceManager::getEditorData(..., true, nullptr)` identify playable-but-ungrouped subraces such as Fishman for warnings/diagnostics before mutation?
- Which `GameData` field stores the editor limits file path for a `RACE`, and does the editor fail, fallback, or behave oddly when it is missing?

## Non-Goals For First Pass

- Do not manually edit every limb/body part slider.
- Do not manually mutate appearance data unless the vanilla editor reset path fails after race mutation.
- Do not build dynamic dialogue generation first unless it proves trivially easy in the first POC.
- Do not require custom race wrapper records in pass 1.
- Do not promise supported behavior for subraces with no editor limits file until tested.
- Do not make `StatModification_Extension` depend on race-changing behavior.
- Do not make runtime behavior depend on a C# FCS helper.
- Do not treat internal C++ helper layout as public API.

## Confidence

- High: dialogue action hook and target resolution are a good trigger mechanism.
- High: the editor and race/appearance APIs exist in KenshiLib headers.
- High: vanilla plastic-surgeon editor entry can destructively reset mismatched saved appearance data to the character's current race, based on the `momoso` observation.
- Medium-high: race mutation plus editor entry will produce the desired defaulted appearance without extra reset.
- Medium-low: dynamic dialogue tree injection is practical and maintainable as the first implementation.
- Unknown: exact save-state persistence and medical/body consistency after in-place species changes.

## Next AI: Give The User Concrete Test Steps

When continuing this work, do not stop at implementation notes. Give the user a short, concrete test script they can run in-game.

The test guidance should cover:

1. Which plugin/mod files to copy into the Kenshi mod folder.
2. Which mod to enable in RE_Kenshi.
3. Which FCS dialogue line/action to create or edit.
4. Which target character/race pair to test first.
5. What should happen immediately when the dialogue action fires.
6. What to verify after accepting or cancelling the character editor.
7. Which `RE_Kenshi.log` lines to look for if it fails.

Suggested first manual test:

1. Use a normal player character with an obvious current subrace, such as Greenlander.
2. Add a dialogue action that references an existing editor-supported subrace, such as Scorchlander or Hive Prince.
3. Trigger the dialogue.
4. Confirm the log shows before/after race data and editor invocation.
5. Confirm the character editor opens.
6. Confirm the editor resets appearance for the new subrace.
7. Save, reload, and confirm the changed subrace and appearance persist.

Suggested edge tests:

- A grouped/editor-supported subrace, e.g. `Hive Prince`.
- A playable but ungrouped subrace with editor limits, e.g. `Fishman`, if available in the test data.
- A subrace with no editor limits file, if one exists or can be made in a throwaway test mod; this should be treated as unsupported/unknown and checked carefully for editor side effects.
