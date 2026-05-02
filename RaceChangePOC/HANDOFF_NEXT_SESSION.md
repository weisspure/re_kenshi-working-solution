# Race Change Extension Next-Session Handoff

Use this prompt to continue in a fresh session.

```text
We are in C:\Git\KenshiLib_Examples.

Use the Superpowers plugin first. This is implementation work, so use:

- superpowers:using-superpowers
- superpowers:test-driven-development
- the repo-local kenshi-tdd skill
- superpowers:systematic-debugging if any hook/editor/race behavior surprises us
- superpowers:verification-before-completion before claiming done

Read only the relevant docs first:

- AGENTS.md
- RaceChangePOC/FINDINGS.md
- .github/instructions/re-kenshi-runtime.instructions.md
- .github/instructions/fcs-schema.instructions.md
- StatModification_Extension/README.md
- DialogueIdentityProbe/FINDINGS.md

Task:

Build a small, separate RE_Kenshi proof-of-concept extension for dialogue-driven race/subrace change. Do not modify StatModification_Extension behavior except by copying patterns if useful.

Terminology:

- Player-facing "Race" = FCS/runtime `RACE_GROUP`.
- Player-facing "Subrace" = FCS/runtime `RACE`.
- Example: `Hive` is the race group; `Hive Prince`, `Hive Soldier Drone`, and `Hive Worker Drone` are subraces.

Product goal:

From a dialogue action, resolve the intended player character, mutate their race/subrace to a referenced `RACE` GameData record, then immediately open Kenshi's existing character editor so the editor creates or validates race-appropriate appearance data.

Important observed behavior:

- NPC stub `momoso` is a Scorchlander in save data but always has Greenlander saved character-edit data.
- When `momoso` goes to the plastic surgeon, the editor changes his appearance to Scorchlander and irreparably discards the Greenlander appearance.
- Therefore, do not manually mutate appearance data in the first product path. The intended first path is race mutation, then vanilla editor entry, letting the editor perform its existing destructive reset.

Implementation preference:

1. First prove a manual FCS action path.
2. Do not start with dynamic dialogue generation unless it proves trivially easy for the first POC; if it is not easy, skip it.
3. Use runtime logs heavily.
4. Keep the public contract clear for mod authors.
5. Do not require extra race wrapper records for pass 1.

Suggested project shape:

- New plugin folder, e.g. `RaceChange_Extension/`.
- New mod package folder under it with `fcs.def`, `RE_Kenshi.json`, and possibly `FCS_extended.json` later.
- Hook `Dialogue::_doActions`.
- Add direct dialogue action keys like `change race: RACE (0)` and maybe `change other race: RACE (0)`.
- Do not add a custom `RACE_CHANGE_TARGET` record type in pass 1.
- Reuse the StatModification target-resolution style rather than inventing new semantics.
- Allow any explicitly referenced `RACE` subrace record. User FCS observation: Fishman is marked playable and has editor limits, but does not appear in character edit because it is not in a race group. That should be a warning/diagnostic, not a hard reject: if a mod author intends Fishman surgery, they get Fishman surgery.
- Treat an editor limits file as required/recommended for supported behavior. If the referenced subrace has no editor limits file, warn that behavior is untested and may be unstable, but do not assume we know it fails until the POC tests it.

TDD/probe order:

1. Logging-only action:
   - resolve speaker/other/target
   - log current race
   - log referenced target race
   - warn/log if referenced race is not in an editor-visible race group
   - warn/log if referenced race has no editor limits file
   - log player/human checks

2. Editor invocation only:
   - with no race mutation, invoke either `PlayerInterface::activateCharacterEditMode(character)` or `gui->showCharacterEditor(...)`
   - determine which matches vanilla plastic-surgeon behavior better
   - if direct invocation from `_doActions` is unsafe, defer by one update tick

3. Minimal mutation:
   - call `Character::setRace(targetRace)`
   - log before/after `getRace()`
   - open editor
   - confirm the editor resets appearance data like the `momoso` plastic-surgeon observation

4. Explicit appearance reset only if needed:
   - test `AppearanceManager::createAppearanceData(targetRace)`
   - test `AppearanceManager::cleanValidateAppearanceData(...)`
   - test `AppearanceManager::resetAll(..., clearAttachments)`
   - assign via `Character::setAppearanceData(...)`
   - skip this for product code if the minimal path works

5. Playable race enumeration:
   - call `AppearanceManager::getEditorData(..., true, nullptr)`
   - log race group count, subrace count, names/string IDs
   - confirm playable-but-ungrouped races such as Fishman are excluded
   - use this only for diagnostics and future dynamic UI/dialogue design, not as a hard gate for explicit FCS action references

Important APIs found during POC:

- `Character::setRace(GameData* r)`
- `Character::getRace() const`
- `Character::getAppearanceData()`
- `Character::setAppearanceData(GameDataCopyStandalone*)`
- `RaceData::getRaceData(GameData*)`
- `RaceData::isSpecificRace(GameData*)`
- `AppearanceManager::getInstance()`
- `AppearanceManager::createAppearanceData(GameData* race)`
- `AppearanceManager::cleanValidateAppearanceData(GameData*)`
- `AppearanceManager::resetAll(GameData*, bool)`
- `AppearanceManager::getEditorData(..., bool playableOnly, const Ogre::vector<GameDataReference>::type* filter)`
- `ForgottenGUI::showCharacterEditor(const lektor<Character*> characters, CharacterEditMode mode, const Ogre::vector<GameDataReference>::type* races)`
- `PlayerInterface::activateCharacterEditMode(Character*)`
- global `gui` from `kenshi/Globals.h`

Open questions to answer with logs/tests:

- Does `setRace` alone persist and update live race state?
- Is a Kenshi subrace the `RACE` record passed to `setRace`, while player-facing race is `RaceData::raceGroup` / `RACE_GROUP`?
- Can `change race: RACE (0)` be used directly with vanilla/modded subrace records, avoiding custom wrapper records? Expected: yes, based on existing `take item: ITEM (1)` dialogue action pattern.
- Can `AppearanceManager::getEditorData(..., true, nullptr)` serve as the authoritative editor-visible race/subrace set?
- Which `RACE` field stores the editor limits file path, and what actually happens if it is missing when the editor opens?
- Does editor entry reset mismatched appearance data automatically after `setRace`, matching the `momoso` plastic-surgeon observation?
- Is explicit `AppearanceManager` reset unnecessary? Prefer yes unless logs prove otherwise.
- Which editor invocation path is closest to the plastic surgeon?
- Does opening the editor inside `_doActions` require deferred execution?

Verification:

- Build the new plugin using the existing Visual Studio/MSBuild patterns.
- If C++ project membership changes, regenerate `compile_commands.json`.
- Use runtime RE_Kenshi logs as the main behavioral evidence.
- Keep findings updated in `RaceChangePOC/FINDINGS.md` or a new project-local findings file.

Before handing back to the user, provide concrete in-game next test steps:

1. Files to copy into the Kenshi mod folder.
2. Mod to enable.
3. Exact FCS action to add, e.g. `change race` referencing a `RACE` subrace.
4. First safe target race/subrace pair to test.
5. Expected editor behavior.
6. Persistence checks after save/reload.
7. Log lines to inspect in `RE_Kenshi.log`.
```
