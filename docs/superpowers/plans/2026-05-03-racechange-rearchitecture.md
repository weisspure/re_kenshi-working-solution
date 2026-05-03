# RaceChange Rearchitecture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split RaceChange runtime mechanics into focused action directories while preserving behavior and strengthening TDD documentation.

**Architecture:** Keep `ActionCore`, `FcsData`, `Logging`, and `Targets` as shared support. Move runtime mutation into `src/actions/inventory`, `src/actions/appearance`, and `src/actions/animal`; leave `RaceActions.cpp` as dialogue orchestration with guard clauses and named path functions.

**Tech Stack:** C++ with VC++ 2010-compatible syntax, Boost included unit tests in `RaceChange_Tests`, RE_Kenshi/KenshiLib runtime headers.

---

## File Structure

- Modify: `RaceChange_Extension/src/ActionCore.h`
- Modify: `RaceChange_Extension/src/ActionCore.cpp`
- Modify: `RaceChange_Extension/src/FcsData.h`
- Modify: `RaceChange_Extension/src/FcsData.cpp`
- Modify: `RaceChange_Extension/src/RaceActions.cpp`
- Modify: `RaceChange_Extension/RaceChange_Extension.vcxproj`
- Modify: `RaceChange_Tests/Tests/ActionCoreTests.cpp`
- Create: `RaceChange_Extension/src/actions/inventory/InventoryActions.h`
- Create: `RaceChange_Extension/src/actions/inventory/InventoryActions.cpp`
- Create: `RaceChange_Extension/src/actions/appearance/AppearanceActions.h`
- Create: `RaceChange_Extension/src/actions/appearance/AppearanceActions.cpp`
- Create: `RaceChange_Extension/src/actions/animal/AnimalRaceActions.h`
- Create: `RaceChange_Extension/src/actions/animal/AnimalRaceActions.cpp`

---

### Task 1: Add Pure Path-Policy Tests

**Files:**
- Modify: `RaceChange_Tests/Tests/ActionCoreTests.cpp`
- Modify: `RaceChange_Extension/src/ActionCore.h`
- Modify: `RaceChange_Extension/src/ActionCore.cpp`

- [ ] **Step 1: Write failing tests for race-change path policy**

Append to `RaceChange_Tests/Tests/ActionCoreTests.cpp`:

```cpp
BOOST_AUTO_TEST_CASE(animal_intent_with_template_uses_animal_replacement_path)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_PATH_ANIMAL_REPLACEMENT, SelectRaceChangePath(RACE_CHANGE_INTENT_ANIMAL, true));
}

BOOST_AUTO_TEST_CASE(animal_intent_without_template_falls_back_to_in_place_full_inventory_path)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY, SelectRaceChangePath(RACE_CHANGE_INTENT_ANIMAL, false));
}

BOOST_AUTO_TEST_CASE(humanoid_intent_with_animal_template_uses_in_place_full_inventory_pivot)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY, SelectRaceChangePath(RACE_CHANGE_INTENT_HUMANOID, true));
}

BOOST_AUTO_TEST_CASE(humanoid_intent_without_animal_template_uses_armour_only_in_place_path)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_PATH_IN_PLACE_ARMOUR_ONLY, SelectRaceChangePath(RACE_CHANGE_INTENT_HUMANOID, false));
}

BOOST_AUTO_TEST_CASE(unsupported_intent_has_no_race_change_path)
{
	BOOST_CHECK_EQUAL(RACE_CHANGE_PATH_NONE, SelectRaceChangePath(RACE_CHANGE_INTENT_UNSUPPORTED, true));
	BOOST_CHECK_EQUAL(RACE_CHANGE_PATH_NONE, SelectRaceChangePath(RACE_CHANGE_INTENT_UNSUPPORTED, false));
}
```

- [ ] **Step 2: Run tests and confirm expected failure**

Run:

```powershell
RaceChange_Tests\run_tests.bat
```

Expected: compile fails because `RACE_CHANGE_PATH_*` and `SelectRaceChangePath` are not defined.

- [ ] **Step 3: Add minimal path policy API**

Add to `RaceChange_Extension/src/ActionCore.h` after `RaceChangeIntent`:

```cpp
/** High-level execution path selected after intent and template evidence are known. */
enum RaceChangePath
{
	RACE_CHANGE_PATH_NONE = 0,
	RACE_CHANGE_PATH_IN_PLACE_ARMOUR_ONLY,
	RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY,
	RACE_CHANGE_PATH_ANIMAL_REPLACEMENT
};
```

Add to `ActionCore.h` after `RaceChangeIntentToString`:

```cpp
/** Select the runtime path without touching Kenshi game state. */
RaceChangePath SelectRaceChangePath(RaceChangeIntent intent, bool targetRaceHasAnimalTemplate);

/** Convert a path enum to a stable lowercase diagnostic string. */
const char* RaceChangePathToString(RaceChangePath path);
```

Add to `RaceChange_Extension/src/ActionCore.cpp`:

```cpp
RaceChangePath SelectRaceChangePath(RaceChangeIntent intent, bool targetRaceHasAnimalTemplate)
{
	if (intent == RACE_CHANGE_INTENT_UNSUPPORTED)
		return RACE_CHANGE_PATH_NONE;

	if (intent == RACE_CHANGE_INTENT_ANIMAL && targetRaceHasAnimalTemplate)
		return RACE_CHANGE_PATH_ANIMAL_REPLACEMENT;

	if (intent == RACE_CHANGE_INTENT_ANIMAL)
		return RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY;

	if (targetRaceHasAnimalTemplate)
		return RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY;

	return RACE_CHANGE_PATH_IN_PLACE_ARMOUR_ONLY;
}

const char* RaceChangePathToString(RaceChangePath path)
{
	switch (path)
	{
	case RACE_CHANGE_PATH_IN_PLACE_ARMOUR_ONLY:
		return "in-place-armour-only";
	case RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY:
		return "in-place-full-inventory";
	case RACE_CHANGE_PATH_ANIMAL_REPLACEMENT:
		return "animal-replacement";
	default:
		return "none";
	}
}
```

- [ ] **Step 4: Run tests and confirm pass**

Run:

```powershell
RaceChange_Tests\run_tests.bat
```

Expected: build succeeds; 15 test cases pass.

- [ ] **Step 5: Commit**

```powershell
git add RaceChange_Tests\Tests\ActionCoreTests.cpp RaceChange_Extension\src\ActionCore.h RaceChange_Extension\src\ActionCore.cpp
git commit -m "Add RaceChange path policy tests"
```

---

### Task 2: Move FCS Diagnostic Helpers Into `FcsData`

**Files:**
- Modify: `RaceChange_Extension/src/FcsData.h`
- Modify: `RaceChange_Extension/src/FcsData.cpp`
- Modify: `RaceChange_Extension/src/RaceActions.cpp`

- [ ] **Step 1: Add declarations to `FcsData.h`**

Add after `FindReferences`:

```cpp
/** Read a string field from GameData, returning an empty string for null or missing keys. */
std::string GetStringField(GameData* data, const std::string& key);

/** Describe all object reference lists on a record for action-scan diagnostics. */
std::string DescribeObjectReferenceKeys(GameData* data);

/** Describe the first reference for a field key, including values and resolved pointer. */
std::string DescribeFirstReference(GameData* data, const std::string& key);
```

- [ ] **Step 2: Move implementations to `FcsData.cpp`**

Move the current `GetStringField`, `DescribeObjectReferenceKeys`, and `DescribeFirstReference` implementations from `RaceActions.cpp` into `FcsData.cpp`. Add:

```cpp
#include "Logging.h"
```

near the top of `FcsData.cpp` so `IntToString` and `DescribeGameData` remain available.

- [ ] **Step 3: Remove moved static helpers from `RaceActions.cpp`**

Delete the `static std::string GetStringField`, `static std::string DescribeObjectReferenceKeys`, and `static std::string DescribeFirstReference` blocks from `RaceActions.cpp`. Keep call sites unchanged because `RaceActions.cpp` already includes `FcsData.h`.

- [ ] **Step 4: Build and test**

Run:

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

Expected: both succeed.

- [ ] **Step 5: Commit**

```powershell
git add RaceChange_Extension\src\FcsData.h RaceChange_Extension\src\FcsData.cpp RaceChange_Extension\src\RaceActions.cpp
git commit -m "Move RaceChange FCS diagnostic helpers"
```

---

### Task 3: Extract Inventory Actions

**Files:**
- Create: `RaceChange_Extension/src/actions/inventory/InventoryActions.h`
- Create: `RaceChange_Extension/src/actions/inventory/InventoryActions.cpp`
- Modify: `RaceChange_Extension/src/RaceActions.cpp`
- Modify: `RaceChange_Extension/RaceChange_Extension.vcxproj`

- [ ] **Step 1: Create header**

Create `RaceChange_Extension/src/actions/inventory/InventoryActions.h`:

```cpp
#pragma once

#include <string>
#include <vector>

class Character;
class Inventory;
class Item;

/** Describe a runtime item and its inventory relationship for support logs. Null item returns "item=null". */
std::string DescribeItemState(Item* item, Inventory* inventory);

/** Remove every item found in inventory sections before animal replacement or full-inventory pivot paths. */
std::vector<Item*> RemoveAllInventoryItemsBeforeRaceChange(Character* character);

/** Restore previously removed inventory items with dropOnFail=true and destroyOnFail=false. */
void RestoreRemovedInventoryItemsAfterRaceChange(Character* character, const std::vector<Item*>& removedItems);

/** Remove equipped armour before humanoid in-place race mutation, preserving item ownership. */
std::vector<Item*> RemoveArmourBeforeRaceChange(Character* character);

/** Restore armour removed by RemoveArmourBeforeRaceChange after inventory sections are validated. */
void RestoreRemovedArmourAfterRaceChange(Character* character, const std::vector<Item*>& removedItems);

/** Drop evacuated items to the ground for animal replacement or animal fallback policy. */
void DropAllItems(Character* character, const std::vector<Item*>& items);
```

- [ ] **Step 2: Create implementation by moving code**

Create `RaceChange_Extension/src/actions/inventory/InventoryActions.cpp` with:

```cpp
#include "InventoryActions.h"

#include "../../Logging.h"

#include <kenshi/Character.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>

#include <cstdlib>
```

Then move these exact helpers from `RaceActions.cpp` into the new file and remove `static` from each signature:

- `DescribeItemState`
- `RemoveAllInventoryItemsBeforeRaceChange`
- `RestoreRemovedInventoryItemsAfterRaceChange`
- `RemoveArmourBeforeRaceChange`
- `RestoreRemovedArmourAfterRaceChange`
- `DropAllItems`

- [ ] **Step 3: Update `RaceActions.cpp` includes**

Add:

```cpp
#include "actions/inventory/InventoryActions.h"
```

Remove direct includes only needed by moved code if unused after extraction:

```cpp
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
```

Keep `<vector>` because orchestration still stores removed item lists.

- [ ] **Step 4: Update `.vcxproj` membership**

In `RaceChange_Extension/RaceChange_Extension.vcxproj`, add under `<ClCompile>` item group:

```xml
<ClCompile Include="src\actions\inventory\InventoryActions.cpp" />
```

Add under `<ClInclude>` item group:

```xml
<ClInclude Include="src\actions\inventory\InventoryActions.h" />
```

- [ ] **Step 5: Build and test**

Run:

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

Expected: both succeed.

- [ ] **Step 6: Commit**

```powershell
git add RaceChange_Extension\src\actions\inventory\InventoryActions.h RaceChange_Extension\src\actions\inventory\InventoryActions.cpp RaceChange_Extension\src\RaceActions.cpp RaceChange_Extension\RaceChange_Extension.vcxproj
git commit -m "Extract RaceChange inventory actions"
```

---

### Task 4: Extract Appearance Actions

**Files:**
- Create: `RaceChange_Extension/src/actions/appearance/AppearanceActions.h`
- Create: `RaceChange_Extension/src/actions/appearance/AppearanceActions.cpp`
- Modify: `RaceChange_Extension/src/RaceActions.cpp`
- Modify: `RaceChange_Extension/RaceChange_Extension.vcxproj`

- [ ] **Step 1: Create header**

Create `RaceChange_Extension/src/actions/appearance/AppearanceActions.h`:

```cpp
#pragma once

class Character;
class GameData;

/** Reset borrowed Kenshi appearance data for the new race. Returns false when runtime services or inputs are missing. */
bool ResetAppearanceDataForRace(Character* character, GameData* targetRace);

/** Rebuild race-derived inventory sections after a live race change. Null character is a no-op. */
void RefreshRaceDerivedInventory(Character* character);

/** Open the vanilla character editor for the final live character after race data and inventory sections are repaired. */
void OpenCharacterEditor(Character* character);
```

- [ ] **Step 2: Create implementation by moving code**

Create `RaceChange_Extension/src/actions/appearance/AppearanceActions.cpp` with:

```cpp
#include "AppearanceActions.h"

#include "../../FcsData.h"
#include "../../Logging.h"

#include <kenshi/AppearanceManager.h>
#include <kenshi/Character.h>
#include <kenshi/GameData.h>
#include <kenshi/Globals.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObject.h>
```

Move these helpers from `RaceActions.cpp` into the new file:

- `SetSingleRaceReference`
- `RefreshPlayerSelectionForCharacter`
- `OpenCharacterEditor`
- `ResetAppearanceDataForRace`
- `RefreshRaceDerivedInventory`

Keep `SetSingleRaceReference` and `RefreshPlayerSelectionForCharacter` `static` because they are private implementation details.

- [ ] **Step 3: Update `RaceActions.cpp` includes**

Add:

```cpp
#include "actions/appearance/AppearanceActions.h"
```

Remove direct includes only needed by moved code if unused after extraction:

```cpp
#include <kenshi/AppearanceManager.h>
#include <kenshi/PlayerInterface.h>
```

Keep `RootObject.h` until animal extraction is complete.

- [ ] **Step 4: Update `.vcxproj` membership**

Add:

```xml
<ClCompile Include="src\actions\appearance\AppearanceActions.cpp" />
<ClInclude Include="src\actions\appearance\AppearanceActions.h" />
```

to the existing compile/include item groups.

- [ ] **Step 5: Build and test**

Run:

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

Expected: both succeed.

- [ ] **Step 6: Commit**

```powershell
git add RaceChange_Extension\src\actions\appearance\AppearanceActions.h RaceChange_Extension\src\actions\appearance\AppearanceActions.cpp RaceChange_Extension\src\RaceActions.cpp RaceChange_Extension\RaceChange_Extension.vcxproj
git commit -m "Extract RaceChange appearance actions"
```

---

### Task 5: Extract Animal Race Actions

**Files:**
- Create: `RaceChange_Extension/src/actions/animal/AnimalRaceActions.h`
- Create: `RaceChange_Extension/src/actions/animal/AnimalRaceActions.cpp`
- Modify: `RaceChange_Extension/src/RaceActions.cpp`
- Modify: `RaceChange_Extension/RaceChange_Extension.vcxproj`

- [ ] **Step 1: Create header**

Create `RaceChange_Extension/src/actions/animal/AnimalRaceActions.h`:

```cpp
#pragma once

class Character;
class GameData;
class RootObject;

/** Return the current runtime race GameData for a character, or null when character/race data is missing. */
GameData* GetRaceGameData(Character* character);

/** Find the first ANIMAL_CHARACTER template referencing targetRace. Returns null for missing runtime data or no match. */
GameData* FindAnimalTemplateForRace(GameData* targetRace);

/** Spawn an animal from a template at the source character's position/faction/platoon. Returns null on runtime failure. */
RootObject* SpawnAnimalFromTemplate(Character* character, GameData* animalTemplate);

/** Transfer only the verified supported source state: name, stats, and common runtime fields. */
void TransferSupportedStateToSpawnedAnimal(Character* source, Character* dest);

/** Destroy the source character after replacement succeeds, logging null runtime failures. */
void DestroySourceAfterAnimalReplacement(Character* source, Character* spawnedCharacter);
```

- [ ] **Step 2: Create implementation by moving code**

Create `RaceChange_Extension/src/actions/animal/AnimalRaceActions.cpp` with:

```cpp
#include "AnimalRaceActions.h"

#include "../../Logging.h"

#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/Enums.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Platoon.h>
#include <kenshi/RaceData.h>
#include <kenshi/RootObject.h>
#include <kenshi/RootObjectFactory.h>

#include <cstdlib>
```

Move these helpers from `RaceActions.cpp`:

- `GetRaceGameData`
- `TransferNameToSpawnedAnimal`
- `TransferStatsToSpawnedAnimal`
- `TransferCommonRuntimeStateToSpawnedAnimal`
- `SpawnAnimalFromTemplate`
- `FindAnimalTemplateForRace`

Keep the three specific transfer helpers `static`. Add the public wrapper:

```cpp
void TransferSupportedStateToSpawnedAnimal(Character* source, Character* dest)
{
	if (source == 0 || dest == 0)
		return;

	TransferNameToSpawnedAnimal(source, dest);
	TransferStatsToSpawnedAnimal(source, dest);
	TransferCommonRuntimeStateToSpawnedAnimal(source, dest);
}
```

Add:

```cpp
void DestroySourceAfterAnimalReplacement(Character* source, Character* spawnedCharacter)
{
	if (ou == 0)
	{
		LogWarning(
			"could not destroy source character after animal replacement because ou is null"
			" | source={" +
			DescribeCharacter(source) + "}"
			" | spawned={" +
			DescribeCharacter(spawnedCharacter) + "}");
		return;
	}

	bool destroyed = ou->destroy(static_cast<RootObject*>(source), false, "RaceChange animal replacement");
	LogInfo(
		"destroyed source character after animal replacement"
		" | success=" +
		std::string(destroyed ? "true" : "false") +
		" | source={" + DescribeCharacter(source) + "}"
		" | spawned={" + DescribeCharacter(spawnedCharacter) + "}");
}
```

- [ ] **Step 3: Update `RaceActions.cpp` includes and call sites**

Add:

```cpp
#include "actions/animal/AnimalRaceActions.h"
```

Replace:

```cpp
TransferNameToSpawnedAnimal(character, spawnedCharacter);
TransferStatsToSpawnedAnimal(character, spawnedCharacter);
TransferCommonRuntimeStateToSpawnedAnimal(character, spawnedCharacter);
```

with:

```cpp
TransferSupportedStateToSpawnedAnimal(character, spawnedCharacter);
```

Replace the inline `ou->destroy` block with:

```cpp
DestroySourceAfterAnimalReplacement(character, spawnedCharacter);
```

Remove now-unused includes from `RaceActions.cpp`:

```cpp
#include <kenshi/CharStats.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Platoon.h>
#include <kenshi/RaceData.h>
#include <kenshi/RootObjectFactory.h>
#include <cstdlib>
```

- [ ] **Step 4: Update `.vcxproj` membership**

Add:

```xml
<ClCompile Include="src\actions\animal\AnimalRaceActions.cpp" />
<ClInclude Include="src\actions\animal\AnimalRaceActions.h" />
```

to the existing compile/include item groups.

- [ ] **Step 5: Build and test**

Run:

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

Expected: both succeed.

- [ ] **Step 6: Commit**

```powershell
git add RaceChange_Extension\src\actions\animal\AnimalRaceActions.h RaceChange_Extension\src\actions\animal\AnimalRaceActions.cpp RaceChange_Extension\src\RaceActions.cpp RaceChange_Extension\RaceChange_Extension.vcxproj
git commit -m "Extract RaceChange animal actions"
```

---

### Task 6: Reshape `ApplyRaceChangeRef` Into Orchestration

**Files:**
- Modify: `RaceChange_Extension/src/RaceActions.cpp`

- [ ] **Step 1: Add path helper functions above `ApplyRaceChangeRef`**

Add these helpers above `ApplyRaceChangeRef`:

```cpp
static bool ValidateRaceChangeReference(Character* character, GameData* targetRace, const std::string& actionKey)
{
	if (character == 0)
	{
		LogError("could not resolve target character | action=" + actionKey);
		return false;
	}

	if (targetRace == 0)
	{
		LogError("race reference is null | action=" + actionKey + " | character={" + DescribeCharacter(character) + "}");
		return false;
	}

	if ((int)targetRace->type != (int)RACE)
	{
		LogError(
			"wrong item type for race change action"
			" | action=" +
			actionKey +
			" | expected=" + IntToString((int)RACE) +
			" | got=" + IntToString((int)targetRace->type) +
			" | targetRace={" + DescribeGameData(targetRace) + "}");
		return false;
	}

	return true;
}

static bool RunAnimalReplacement(Character* character, GameData* targetRace, GameData* animalTemplate, const std::string& actionKey)
{
	std::vector<Item*> removedInventoryItems = RemoveAllInventoryItemsBeforeRaceChange(character);
	DropAllItems(character, removedInventoryItems);

	RootObject* spawned = SpawnAnimalFromTemplate(character, animalTemplate);
	if (spawned == 0)
	{
		LogError(
			"animal spawn failed; aborting animal transform"
			" | action=" +
			actionKey +
			" | character={" + DescribeCharacter(character) + "}"
			" | targetRace={" + DescribeGameData(targetRace) + "}"
			" | animalTemplate={" + DescribeGameData(animalTemplate) + "}");
		return false;
	}

	Character* spawnedCharacter = static_cast<Character*>(spawned);
	TransferSupportedStateToSpawnedAnimal(character, spawnedCharacter);
	ResetAppearanceDataForRace(spawnedCharacter, targetRace);
	RefreshRaceDerivedInventory(spawnedCharacter);
	OpenCharacterEditor(spawnedCharacter);
	DestroySourceAfterAnimalReplacement(character, spawnedCharacter);
	return true;
}

static void RunInPlaceRaceMutation(Character* character, GameData* targetRace, RaceChangePath path, RaceChangeIntent intent)
{
	std::vector<Item*> removedArmour;
	std::vector<Item*> removedInventoryItems;

	if (path == RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY)
		removedInventoryItems = RemoveAllInventoryItemsBeforeRaceChange(character);
	else
		removedArmour = RemoveArmourBeforeRaceChange(character);

	character->setRace(targetRace);
	LogInfo("changed race | character={" + DescribeCharacter(character) + "} | afterRace={" + DescribeGameData(GetRaceGameData(character)) + "}");

	ResetAppearanceDataForRace(character, targetRace);
	RefreshRaceDerivedInventory(character);

	if (intent == RACE_CHANGE_INTENT_ANIMAL)
		DropAllItems(character, removedInventoryItems);
	else if (path == RACE_CHANGE_PATH_IN_PLACE_FULL_INVENTORY)
		RestoreRemovedInventoryItemsAfterRaceChange(character, removedInventoryItems);
	else
		RestoreRemovedArmourAfterRaceChange(character, removedArmour);

	OpenCharacterEditor(character);
}
```

- [ ] **Step 2: Replace `ApplyRaceChangeRef` body with guard-clause orchestration**

Replace the body of `ApplyRaceChangeRef` with:

```cpp
{
	GameData* targetRace = ref.ptr;
	RaceChangeIntent intent = GetRaceChangeIntent(ref.values[0]);
	RaceChangeTargetRole role = GetRaceChangeActionRole(actionKey);
	if (role == RACE_CHANGE_ROLE_UNKNOWN)
	{
		LogError("unknown race change action key reached dispatcher | action=" + actionKey);
		return;
	}

	Character* character = ResolveRaceChangeTarget(dlg, dialogLine, role);
	if (!ValidateRaceChangeReference(character, targetRace, actionKey))
		return;

	if (intent == RACE_CHANGE_INTENT_UNSUPPORTED)
	{
		LogError(
			"unsupported race change intent value"
			" | action=" +
			actionKey +
			" | val0=" + IntToString(ref.values[0]) +
			" | character={" + DescribeCharacter(character) + "}"
			" | targetRace={" + DescribeGameData(targetRace) + "}");
		return;
	}

	LogRaceDiagnostics(targetRace);

	GameData* animalTemplate = FindAnimalTemplateForRace(targetRace);
	RaceChangePath path = SelectRaceChangePath(intent, animalTemplate != 0);
	GameData* beforeRace = GetRaceGameData(character);

	LogInfo(
		"changing race"
		" | action=" +
		actionKey +
		" | role=" + RaceChangeRoleToString(role) +
		" | intent=" + RaceChangeIntentToString(intent) +
		" | path=" + RaceChangePathToString(path) +
		" | character={" + DescribeCharacter(character) + "}"
		" | beforeRace={" + DescribeGameData(beforeRace) + "}"
		" | targetRace={" + DescribeGameData(targetRace) + "}");

	if (intent == RACE_CHANGE_INTENT_ANIMAL && animalTemplate == 0)
	{
		LogWarning(
			"animal intent had no ANIMAL_CHARACTER template; falling back to in-place mutation"
			" | action=" +
			actionKey +
			" | character={" + DescribeCharacter(character) + "}"
			" | targetRace={" + DescribeGameData(targetRace) + "}");
	}

	if (path == RACE_CHANGE_PATH_ANIMAL_REPLACEMENT)
	{
		RunAnimalReplacement(character, targetRace, animalTemplate, actionKey);
		return;
	}

	RunInPlaceRaceMutation(character, targetRace, path, intent);
}
```

- [ ] **Step 3: Remove obsolete branch variables and duplicate logs**

Delete the old `useFullInventoryPivot` block and inline animal branch. Keep `LogRaceDiagnostics` unchanged. Confirm no remaining reference to `useFullInventoryPivot`.

Run:

```powershell
rg -n "useFullInventoryPivot|TransferNameToSpawnedAnimal|TransferStatsToSpawnedAnimal|TransferCommonRuntimeStateToSpawnedAnimal" RaceChange_Extension\src\RaceActions.cpp
```

Expected: no matches.

- [ ] **Step 4: Build and test**

Run:

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

Expected: both succeed.

- [ ] **Step 5: Commit**

```powershell
git add RaceChange_Extension\src\RaceActions.cpp
git commit -m "Reshape RaceChange dispatcher orchestration"
```

---

### Task 7: Project Index Refresh And Runtime Verification Notes

**Files:**
- Modify: `RaceChange_Extension/TEST_PLAN.md`

- [ ] **Step 1: Refresh compile commands after source layout changes**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/generate_compile_commands.ps1
```

Expected: command exits 0. `compile_commands.json` may be generated or updated locally and is ignored.

- [ ] **Step 2: Add runtime verification reminder to `TEST_PLAN.md`**

Append under the existing manual matrix:

```md
## Refactor Regression Notes

After the actions-directory refactor, repeat these dry runs before claiming runtime safety:

- Humanoid to humanoid with armour equipped. Confirm armour is removed before `setRace`, inventory sections validate, armour restore uses `destroyOnFail=false`, and the editor opens for the same character.
- Humanoid to animal with `value[0] == 1`, armour equipped, and backpack inventory. Confirm all inventory is dropped, the animal template is spawned, supported state is transferred, the editor opens for the spawned character, and source destruction happens after replacement.
- Animal intent with no matching `ANIMAL_CHARACTER` template. Confirm fallback uses in-place mutation, evacuated inventory follows the full-inventory policy, and the source character is not destroyed.
```

- [ ] **Step 3: Build and test final state**

Run:

```powershell
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

Expected: both succeed.

- [ ] **Step 4: Commit**

```powershell
git add RaceChange_Extension\TEST_PLAN.md
git commit -m "Document RaceChange refactor regression checks"
```

---

## Final Verification

- [ ] Run:

```powershell
git status --short
RaceChange_Extension\build.bat
RaceChange_Tests\run_tests.bat
```

Expected:

- `git status --short` shows no unrelated uncommitted edits.
- Extension build succeeds.
- Test executable reports all test cases pass.

- [ ] Inspect module references:

```powershell
rg -n "InventoryActions|AppearanceActions|AnimalRaceActions|SelectRaceChangePath|RaceChangePathToString" RaceChange_Extension RaceChange_Tests
```

Expected: new modules are referenced by `RaceActions.cpp`, `.vcxproj`, and tests where applicable.

- [ ] Inspect remaining `RaceActions.cpp` helper bulk:

```powershell
rg -n "static .*\\(" RaceChange_Extension\src\RaceActions.cpp
```

Expected: only orchestration, action scanning, validation/logging helpers remain.
