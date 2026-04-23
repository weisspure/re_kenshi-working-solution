# CLAUDE.md — RE_Kenshi / KenshiLib / FCS Extended Notes

This file captures the hard-won implementation details, design decisions, and known-good patterns discovered while building a **RE_Kenshi** plugin and **FCS Extended** schema for a Kenshi mod that grants flat skill increases through dialogue.

The immediate project that produced these notes is a skill-boost system using:

- a runtime RE_Kenshi plugin DLL
- custom `fcs.def` / `fcs_layout.def`
- a custom data type for boosts
- dialogue `WORD_SWAPS` actions that point at those records

The goal of this file is to save future agents from re-discovering the same gotchas.

---

## 1. Big picture

### What we successfully proved

We proved all of these:

1. **A RE_Kenshi runtime plugin DLL can hook dialogue actions and mutate stats directly.**
2. **FCS Extended will pick up custom schema from `fcs.def` without needing a separate FCS C# plugin in simple cases.**
3. **Custom item/data types can be declared via `enum itemType { ... }` and used in custom dialogue action pickers.**
4. **Those custom types can be placed in the left-hand FCS tree using `fcs_layout.def`, including nested under existing headings.**
5. **Referenced custom record fields are accessible at runtime via `GameData::sdata`, `idata`, `fdata`, `bdata`.**
6. **`Dialogue::me` is always the NPC (owner of the `Dialogue` object). `Dialogue::getConversationTarget().getCharacter()` is always the player. These identities are fixed regardless of whether the current line is an NPC line or a player reply line — `_doActions` fires for both.**
7. **The best runtime token source is a custom string field like `stat token`, not `stringID`.**

### What this means architecturally

For a mod like this, the system naturally splits into:

- **Runtime behavior**: `SkillIncrease.dll` loaded by RE_Kenshi
- **Schema / editor support**: `fcs.def`, `fcs_layout.def`
- **Canonical records / data**: the `.mod` content
- **Consumer mods**: can depend on the framework/extension mod

---

## 2. Build environment and setup gotchas

## 2.1 Toolchain

The KenshiLib examples are old and require old toolchain assumptions.

Known-good setup:

- **Visual Studio** project type: C++ DLL (`.vcxproj`)
- **Configuration**: `Release | x64`
- **Platform Toolset**: `v100`
- **Character Set**: `Use Unicode Character Set`

The safest path was **copying a working example project** like `HelloWorld` rather than creating a fresh project from scratch.

---

## 2.2 Include/lib paths

Known-good project settings:

### C/C++ → General → Additional Include Directories

```text
$(KENSHILIB_DIR)\Include;$(BOOST_INCLUDE_PATH);$(IncludePath)
```

### Linker → General → Additional Library Directories

```text
$(KENSHILIB_DIR)\Libraries;$(LibraryPath)
```

### Linker → Input → Additional Dependencies

```text
kenshilib.lib;%(AdditionalDependencies)
```

`$(KENSHILIB_DIR)` should point at the KenshiLib root that contains:

- `Include`
- `Libraries`

`$(BOOST_INCLUDE_PATH)` must point at the **Boost root folder**, not the `boost` subfolder.

Correct:

```text
C:\...\boost_1_60_0
```

So that this exists:

```text
C:\...\boost_1_60_0\boost\unordered_map.hpp
```

---

## 2.3 Git LFS gotcha

A critical failure we hit:

`KenshiLib.lib` was present but was actually a **Git LFS pointer file**, not the real binary.

Symptoms:

- linker error like `LNK1107: invalid or corrupt file`
- file size `0 bytes` or tiny
- opening it in Notepad shows:

```text
version https://git-lfs.github.com/spec/v1
oid sha256:...
size ...
```

Fix:

- install **Git LFS**
- run `git lfs install`
- run `git lfs pull`
- or reclone the repo after installing Git LFS

Do **not** waste time changing linker settings until you check the actual `.lib` file contents.

---

## 2.4 Boost zip gotcha

Another easy mistake:

If `BOOST_INCLUDE_PATH` points at a folder containing only `boost.zip`, the compiler will still fail with missing `boost/...` headers.

Fix:

- extract the zip
- point `BOOST_INCLUDE_PATH` to the extracted folder root

---

## 3. Runtime plugin structure

The RE_Kenshi runtime plugin pattern is:

- export `startPlugin()`
- hook a game function with `KenshiLib::AddHook(...)`
- store the original function pointer
- run custom code
- call the original function

### Minimal hook pattern

```cpp
void (*_doActions_orig)(Dialogue* thisptr, DialogLineData* dialogLine);

void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
    // custom logic here
    _doActions_orig(thisptr, dialogLine);
}

__declspec(dllexport) void startPlugin()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&Dialogue::_doActions),
        &_doActions_hook,
        &_doActions_orig))
    {
        ErrorLog("Could not hook Dialogue::_doActions");
        return;
    }

    DebugLog("Plugin loaded");
}
```

### Why `_doActions`?

Because for dialogue **effects/actions**, the correct hook point is `Dialogue::_doActions`.

Do **not** confuse this with dialogue **conditions**. The example plugin uses different hooks for conditions vs effects.

---

## 4. Left side vs right side in FCS dialogue UI

This distinction matters.

### Left side
This appears to be the built-in **dialogue condition/effect enum-driven system**.

Examples:
- `DC_*` conditions
- `DA_*` style vanilla effects

### Right side
This is the schema-driven **reference/value action system**, driven by:

```ini
[DIALOGUE,DIALOGUE_LINE,WORD_SWAPS]
```

This is where custom entries like the example's `take item: ITEM (1)` appear.

### Important conclusion

For this project, the custom skill-boost action currently works through the **right-side `WORD_SWAPS` system**, not as a true left-side built-in `DA_*` effect.

We investigated left-side custom effects and did **not** find a simple `fcs.def`-only way to extend the left-side `DA_*` list.

---

## 5. Our working schema design

We moved away from using raw `ITEM` because the picker was polluted by every item in the game.

The better design was to create a custom type.

### Known working pattern

```ini
enum itemType { SI_SKILL_BOOST=900 }

[SI_SKILL_BOOST]
stat token: "" "e.g. STAT_STRENGTH, STAT_ATHLETICS"

[FCS_LAYOUT]
Characters:            CHARACTER
    Stats:             STATS
        Boosts:        SI_SKILL_BOOST

[DIALOGUE,DIALOGUE_LINE,WORD_SWAPS]
SI_GRANT_SKILL_LEVELS: SI_SKILL_BOOST (10) "val0 = levels to add"
```

### Why this is good

It gives:

- a custom filtered picker
- custom records only
- no giant global item list
- a custom field `stat token`
- clean tree placement under:

```text
Characters
  -> Stats
     -> Boosts
```

This worked in practice.

---

## 6. `fcs.def` and `fcs_layout.def`

## 6.1 `fcs.def`

This defines:

- custom types
- custom fields on those types
- custom dialogue reference/value actions (`WORD_SWAPS`)
- custom enums

## 6.2 `fcs_layout.def`

This controls where types appear in the left-hand tree.

We confirmed that custom types can be nested under existing headings via layout indentation.

Example used successfully:

```ini
[FCS_LAYOUT]
Characters:            CHARACTER
    Stats:             STATS
        Boosts:        SI_SKILL_BOOST
```

### Important limitation

This affects the **left navigation tree**, not the left-side dialogue condition/effect UI.

---

## 7. Why some defs do this twice

A pattern seen in some defs:

```ini
enum DialogConditionEnum { }
enum DialogConditionEnum {
    ...
}
```

This appears to be a parser/merge/extension pattern used by FCS Extended in some mods.

### Important note
For our custom `itemType`, this was **not necessary**.

This worked fine:

```ini
enum itemType { SI_SKILL_BOOST=900 }
```

Do not add extra empty enum declarations unless required by observed parser behavior.

---

## 8. Runtime data access — the most important discovery

This was one of the biggest wins.

### Custom fields from `fcs.def` are stored on `GameData` in hash maps:

From `GameData`:

- `bdata` → bool fields
- `sdata` → string fields
- `idata` → int fields
- `fdata` → float fields

### This is the runtime access pattern:

```cpp
ogre_unordered_map<std::string, std::string>::type::iterator tokenIter =
    ref.ptr->sdata.find("stat token");

if (tokenIter != ref.ptr->sdata.end())
{
    std::string token = tokenIter->second;
}
```

This means our custom field:

```ini
[SI_SKILL_BOOST]
stat token: "" "e.g. STAT_STRENGTH"
```

is readable at runtime via:

```cpp
ref.ptr->sdata["stat token"]
```

### Safe helper

```cpp
static std::string SafeGetStringField(GameData* ptr, const std::string& fieldName, const std::string& fallback)
{
    if (ptr == 0)
        return fallback;

    ogre_unordered_map<std::string, std::string>::type::iterator iter =
        ptr->sdata.find(fieldName);

    if (iter != ptr->sdata.end())
        return iter->second;

    return fallback;
}
```

### Recommendation
Use:

- `stat token` as the primary token source
- `stringID` only as fallback

---

## 9. `GameDataReference` shape

Another important discovery:

`GameDataReference::values` is **not** a vector. It is a `TripleInt`.

So:

- `values[0]`
- `values[1]`
- `values[2]`

are valid

But **`.size()` is not**.

### Correct use

```cpp
int levels = ref.values[0];
```

Not:

```cpp
ref.values.size()
```

---

## 10. Dialogue semantics: `me` vs `conversationTarget`

This is crucial.

Using KenshiLib example code, we confirmed:

- `Dialogue::me` = the NPC/dialogue owner (always — the `Dialogue` object belongs to the NPC)
- `Dialogue::getConversationTarget().getCharacter()` = the player who clicked the reply

This matters because using `dlg->me` in our first implementation caused the NPC's stats to be boosted instead of the player's.

### Where `thisptr` comes from

`_doActions` is hooked as a free function via MinHook, so `this` becomes an explicit first parameter:

```cpp
void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
//                   ^^^^^^^^^^^^^^^^^^^
//                   The Dialogue object the game called _doActions on
//                   me = the NPC that owns this Dialogue object
```

### `_doActions` fires for EVERY line — NPC and player

The hook fires for every dialogue line in the conversation. The **identity** of `me` and `conversationTarget` never changes, but the **semantic intent** of an action depends on which line it is attached to:

```
Action on an NPC line:    NPC is doing something to the player
Action on a player line:  Player is doing something / receiving something
```

The vanilla `take item` example is designed for **player reply lines** ("I'll pay you"):

```cpp
// "take item" — NPC takes FROM the player
Character* giver = thisptr->getConversationTarget().getCharacter();  // player pays
Character* taker = thisptr->me;                                       // NPC receives
```

If that same action were placed on an NPC line, the semantics would be backwards.

### Correct learner resolution for player-selected reply lines

```cpp
Character* learner = dlg->getConversationTarget().getCharacter();  // player receives boost
```

Fallback to `dlg->me` only if needed.

### Important: these are roles, not recipient conventions

`me`/`conversationTarget` are **fixed identity pointers** (NPC vs player), not semantic roles like "giver" or "recipient". Which one is the recipient depends entirely on what the action does and which line it sits on.

### Design decision for `SI_GRANT_SKILL_LEVELS`

The action is currently assumed to sit on a **player reply line** (player learns by speaking/choosing). If you ever need NPC-initiated teaching (action on an NPC line granting the player a skill), the targeting logic is the same — `conversationTarget` is still the player — but the authoring convention differs. Document in `fcs.def` comments which line type the action is intended for.

---

## 11. Recommended token design

Do **not** use random autogenerated string IDs like:

```text
11-SkillIncrease.mod
```

Those are FCS-generated and not stable semantically.

Use:

- custom type: `SI_SKILL_BOOST`
- custom field: `stat token`
- values like:
  - `STAT_STRENGTH`
  - `STAT_ATHLETICS`
  - `STAT_DEXTERITY`

### Recommended token source priority

1. `stat token`
2. fallback to `stringID`

---

## 12. Recommended runtime code pattern

This is the known-good pattern we were converging on.

```cpp
enum itemTypeExtended
{
    SI_SKILL_BOOST = 900
};

static std::string SafeGetStringField(GameData* ptr, const std::string& fieldName, const std::string& fallback)
{
    if (ptr == 0)
        return fallback;

    ogre_unordered_map<std::string, std::string>::type::iterator iter =
        ptr->sdata.find(fieldName);

    if (iter != ptr->sdata.end())
        return iter->second;

    return fallback;
}

static std::string ResolveTokenFromReference(GameDataReference& ref)
{
    if (ref.ptr == 0)
    {
        ErrorLog("SkillIncrease: ref.ptr is null");
        return "";
    }

    std::string token = SafeGetStringField(ref.ptr, "stat token", ref.ptr->stringID);
    return token;
}
```

And learner resolution:

```cpp
static Character* ResolveLearner(Dialogue* dlg)
{
    if (dlg == 0)
        return 0;

    Character* targetChar = dlg->getConversationTarget().getCharacter();
    if (targetChar != 0)
        return targetChar;

    return dlg->me;
}
```

---

## 13. Skill enum mapping

We used a whitelist mapping from token strings to `StatsEnumerated`.

Example:

```cpp
static StatsEnumerated SkillFromToken(const std::string& token)
{
    if (token == "STAT_STRENGTH")      return STAT_STRENGTH;
    if (token == "STAT_ATHLETICS")     return STAT_ATHLETICS;
    if (token == "STAT_DEXTERITY")     return STAT_DEXTERITY;
    if (token == "STAT_TOUGHNESS")     return STAT_TOUGHNESS;
    if (token == "STAT_MELEE_ATTACK")  return STAT_MELEE_ATTACK;
    if (token == "STAT_MELEE_DEFENCE") return STAT_MELEE_DEFENCE;
    if (token == "STAT_THIEVING")      return STAT_THIEVING;
    if (token == "STAT_STEALTH")       return STAT_STEALTH;
    if (token == "STAT_ASSASSINATION") return STAT_ASSASSINATION;
    if (token == "STAT_LABOURING")     return STAT_LABOURING;
    if (token == "STAT_SCIENCE")       return STAT_SCIENCE;
    if (token == "STAT_ENGINEERING")   return STAT_ENGINEERING;
    if (token == "STAT_ROBOTICS")      return STAT_ROBOTICS;
    if (token == "STAT_TURRETS")       return STAT_TURRETS;
    if (token == "STAT_FARMING")       return STAT_FARMING;
    if (token == "STAT_COOKING")       return STAT_COOKING;

    return STAT_NONE;
}
```

### Why whitelist?
Because `StatsEnumerated` contains more than just sensible public-facing skills. It includes internal/derived-looking entries too.

---

## 14. Direct stat mutation

We are **not** granting XP.

We are mutating the stat directly.

### Correct runtime call

```cpp
float& current = stats->getStatRef(stat);
current += (float)levels;
```

### Intentional design choice
We explicitly **do not clamp to 100**.

Going above 100 is allowed intentionally for flavor / weird behavior.

---

## 15. Logging recommendations

A very useful debug build should log:

- plugin load
- hook installation
- learner resolution
- `me` stat before/after
- `conversationTarget` stat before/after
- reference type
- raw `stringID`
- custom `stat token`
- chosen token source
- resolved enum
- before/add/after values

This made it much easier to verify:

- whether the right character was being boosted
- whether we were reading the correct token source
- whether stats were really changing

---

## 16. Packaging recommendation

### Chosen recommendation
Use **one combined framework package** plus consumer/example mods.

### Framework package
Suggested name: `SkillIncrease_Extension`

Contains:

- `SkillIncrease.dll`
- `RE_Kenshi.json`
- `SkillIncrease_Extension.mod`
- `fcs.def`
- `fcs_layout.def`
- canonical `SI_SKILL_BOOST` records inside the `.mod`

This package acts as:

- runtime dependency
- authoring schema
- canonical data source

### Consumer/example mod
Example: `UncleBugus`

Contains:

- `UncleBugus.mod`
- NPCs/dialogue/content only

Depends on `SkillIncrease_Extension`.

### Why not split SDK/framework?
Because the runtime, schema, layout, and canonical records are tightly coupled and easier for users/authors to understand as one package.

---

## 17. Canonical data vs duplicated local records

This was an important architectural decision.

### If every mod defines its own local strength boost:
Bad for modpacks, because multiple mods create multiple representations of the same concept.

### Better:
Keep canonical boost records in the extension/framework `.mod`.

Examples:

- `SI_STAT_STRENGTH`
- `SI_STAT_ATHLETICS`
- `SI_STAT_DEXTERITY`

Then consumer mods reference those shared records.

This means the framework package is a real dependency, but it avoids duplicated representations.

---

## 18. Important naming advice

Namespace custom things now to avoid future collisions.

Recommended naming:

- type: `SI_SKILL_BOOST`
- action: `SI_GRANT_SKILL_LEVELS`
- canonical records:
  - `SI_STAT_STRENGTH`
  - `SI_STAT_ATHLETICS`
  - etc.

Avoid generic names like:

- `SKILL_BOOST`
- `grant skill levels`

unless already committed and hard to rename.

---

## 19. Current recommended `fcs.def`

```ini
enum itemType { SI_SKILL_BOOST=900 }

[SI_SKILL_BOOST]
stat token: "" "e.g. STAT_STRENGTH, STAT_ATHLETICS"

[DIALOGUE,DIALOGUE_LINE,WORD_SWAPS]
SI_GRANT_SKILL_LEVELS: SI_SKILL_BOOST (10) "val0 = levels to add"
```

---

## 20. Current recommended `fcs_layout.def`

```ini
[FCS_LAYOUT]
Characters:            CHARACTER
    Stats:             STATS
        Boosts:        SI_SKILL_BOOST
```

This nested correctly in testing.

---

## 21. Most likely next implementation step

If continuing this project in a future session, the next likely steps are:

1. Rename any remaining generic names to `SI_` names if not already done
2. Update runtime lookup from:
   - `"grant skill levels"`
   to
   - `"SI_GRANT_SKILL_LEVELS"`
3. Verify that `stat token` is being used, not fallback `stringID`
4. Create canonical records in the framework `.mod`
5. Make the example mod reference those canonical records
6. Retest learner resolution and visible in-game stat changes
7. Optionally add duplicate-DLL guard if supporting vendored runtimes in multiple mods

---

## 22. Known unresolved / caution points

These are not fully settled:

### 22.1 Left-side custom `DA_*` style effect extension
We did **not** find a confirmed clean way to add true left-side custom `DA_*` effects via defs alone.

Current working design uses the **right-side `WORD_SWAPS`** mechanism with a clean custom type and filtered picker.

### 22.2 Duplicate runtime DLLs across mods
If multiple installed mods all bundle the same DLL, double hooks may occur unless guarded.
Best avoided by a shared framework package or a singleton guard.

### 22.3 UI refresh
Kenshi stat UI may not always visibly refresh instantly while dialogue is open, so confirm actual stat values after closing/reopening the panel if needed.

---

## 23. Fast checklist for future agents

If something breaks, check these first:

- Is `KenshiLib.lib` a real binary, not a Git LFS pointer?
- Is Boost actually extracted?
- Is build `Release | x64 | v100`?
- Is plugin loading and logging at boot?
- Is `_doActions` hook installed?
- Is the action lookup key correct? (`SI_GRANT_SKILL_LEVELS` vs old name)
- Is the referenced record type really `SI_SKILL_BOOST`?
- Does the record have `stat token` populated?
- Is learner resolution using `getConversationTarget()`? (always the player regardless of line type)
- Is the action placed on the intended line type? (NPC line vs player reply line affects authoring intent, not `me`/`conversationTarget` identity)
- Are we reading `sdata["stat token"]`?
- Is the resolved token one of the whitelisted `STAT_*` entries?
- Is the stat UI just stale, or did the value truly not change?

---

## 24. Summary in one paragraph

The working architecture is: define a custom type like `SI_SKILL_BOOST` in `fcs.def`, expose it under `Characters -> Stats -> Boosts` via `fcs_layout.def`, use a `WORD_SWAPS` dialogue action like `SI_GRANT_SKILL_LEVELS: SI_SKILL_BOOST (10)`, store the actual stat enum token in a custom string field `stat token`, and in the RE_Kenshi runtime plugin hook `Dialogue::_doActions` (which fires for every line — NPC and player), resolve the learner as `dlg->getConversationTarget().getCharacter()` (always the player; `dlg->me` is always the NPC), read the token from `ref.ptr->sdata["stat token"]` (fallback `stringID`), map it to `StatsEnumerated`, and add directly to `getStatRef(...)` with no clamp. The `thisptr` in the hook is the `Dialogue` object, with `this` made explicit because the member function is hooked as a free function.

---

### 25. Instrumentation / exploration gotchas and recommendations

Runtime lookups use exact string keys in `objectReferences.find("...")`.

If `fcs.def` says:
- `SI_GRANT_SKILL_LEVELS`

but runtime code searches:
- `"grant skill levels"`

then action processing will silently never run.

**Rule:** keep action names synchronized exactly (case, spaces, underscores).


### 25.2 Always guard map lookups before dereferencing `->second`

Custom fields are stored in maps (`sdata`, `idata`, `fdata`, `bdata`).
Never do `find(...)->second` without checking `end()` first.

Safe pattern:

```cpp
ogre_unordered_map<std::string, int>::type::iterator it = ref.ptr->idata.find("value");
if (it != ref.ptr->idata.end())
{
    int value = it->second;
}
else
{
    ErrorLog("Missing parameter: value");
}
```


### 25.3 `GameDataReference::ptr` can be null; resolve defensively

`GameDataReference` has both:
- `GameData* ptr`
- `GameData* getPtr(GameDataContainer* source) const`

In many example paths `ptr` is already valid, but defensive code should still null-check and resolve/fallback when needed.


### 25.4 Some Kenshi APIs allocate `lektor<>` buffers that must be manually freed

Examples call APIs like `getCharactersInArea(...)` / inventory gatherers and then explicitly `free(...)` the returned `stuff` buffer.

If you forget this, you leak memory over long sessions.

Pattern seen in examples:

```cpp
lektor<RootObject*> characters;
activePlatoon->getCharactersInArea(characters, pos, radius, false);

// ... use characters ...

if (characters.stuff)
    free(characters.stuff);
```


### 25.5 Build/debug expectation: Release is the reliable target

Repo notes and README indicate DEBUG builds are unreliable; use:
- `Release | x64 | v100`

for expected behavior.


### 25.6 DebugLog vs ErrorLog behavior

Both `DebugLog()` and `ErrorLog()` work in RE_Kenshi plugins, but:
- `DebugLog()` may be filtered or have lower priority in some configurations
- `ErrorLog()` ensures visibility in logs
- For critical information or exploration logging, prefer `ErrorLog()` temporarily

Pattern for verbose exploration:
```cpp
#define VERBOSE_LOGGING true
static void VerboseLog(const std::string& msg) {
    if (VERBOSE_LOGGING)
        ErrorLog(msg);  // Use ErrorLog for guaranteed visibility
}
```


## 26. Instrumentation and exploration findings

### Current confirmed behavior (as of exploration session):

**✅ Targeting is correct:**
- `Dialogue::getConversationTarget().getCharacter()` correctly identifies the player
- Player character is receiving the stat boosts
- `dlg->me` is the NPC as expected

**✅ Token resolution fallback working:**
- `stringID` fallback is successfully retrieving stat identifiers
- Stats are being correctly boosted using the resolved token

**⚠️ Custom field not populated:**
- `stat token` custom field is empty in FCS records
- This is a **data issue**, not a runtime issue
- The runtime code correctly checks for `stat token` first, then falls back to `stringID`

### Next investigation step:
Focus on **FCS record data**:
1. Verify `stat token` field is defined in `fcs.def`
2. Check if FCS Extended is recognizing the custom field
3. Populate `stat token` in the actual mod records
4. Verify field appears in FCS editor

The runtime code is working correctly - the issue is upstream in the mod data or schema definition.

---

## 27. Cleaned-up post-exploration runtime pattern

After exploration, here's the recommended production runtime pattern that keeps useful logs but removes noisy dumps:

```cpp
// Configuration flags
#define VERBOSE_LOGGING false    // Set true only for deep debugging
#define LOG_EVERY_HOOK false     // Set true to see all dialogue actions

static void VerboseLog(const std::string& msg) {
    if (VERBOSE_LOGGING)
        ErrorLog(msg);
}

static std::string ResolveTokenFromReference(GameDataReference& ref)
{
    if (ref.ptr == 0)
    {
        ErrorLog("SkillIncrease: ref.ptr is null");
        return "";
    }

    // Try custom field first
    ogre_unordered_map<std::string, std::string>::type::iterator tokenIter =
        ref.ptr->sdata.find("stat token");

    if (tokenIter != ref.ptr->sdata.end() && !tokenIter->second.empty())
    {
        VerboseLog("Token source: custom field 'stat token'");
        VerboseLog("Token value: " + tokenIter->second);
        return tokenIter->second;
    }

    // Fallback to stringID
    VerboseLog("Token source: stringID (fallback)");
    VerboseLog("Token value: " + ref.ptr->stringID);
    return ref.ptr->stringID;
}

static void ApplyGrantSkillLevels(Dialogue* dlg, DialogLineData* dialogLine)
{
    // ... null checks ...

    auto iter = dialogLine->getGameData()->objectReferences.find("SI_GRANT_SKILL_LEVELS");
    if (iter == dialogLine->getGameData()->objectReferences.end())
        return;

    Character* learner = dlg->getConversationTarget().getCharacter();
    if (!learner)
        learner = dlg->me;

    if (!learner || !learner->getStats())
    {
        ErrorLog("SkillIncrease: No valid learner found");
        return;
    }

    CharStats* stats = learner->getStats();

    for (auto& ref : iter->second)
    {
        if (!ref.ptr)
            continue;

        std::string token = ResolveTokenFromReference(ref);
        StatsEnumerated stat = SkillFromToken(token);

        if (stat == STAT_NONE)
        {
            ErrorLog("SkillIncrease: Unsupported skill token: " + token);
            continue;
        }

        int levels = ref.values[0] ? ref.values[0] : 10;

        float& current = stats->getStatRef(stat);
        float before = current;
        current += (float)levels;

        ErrorLog(
            "SkillIncrease: Applied " + token +
            " | before=" + ToStringFloat(before) +
            " | add=" + ToStringInt(levels) +
            " | after=" + ToStringFloat(current)
        );
    }
}

void _doActions_hook(Dialogue* thisptr, DialogLineData* dialogLine)
{
    if (LOG_EVERY_HOOK)
        VerboseLog("_doActions_hook fired");

    ApplyGrantSkillLevels(thisptr, dialogLine);

    _doActions_orig(thisptr, dialogLine);
}
```

This pattern:
- ✅ Keeps critical "applied" logs visible
- ✅ Has verbose logging available via flag
- ✅ Removes noisy objectReferences dumps
- ✅ Correctly prioritizes custom field over stringID
- ✅ Uses ErrorLog for important messages (guaranteed visibility)
- ✅ Has no unreachable code
- ✅ Clean control flow with early returns

---

