---
applyTo: "**/*.cpp"
---
# RE_Kenshi Runtime Plugin Patterns

## Hook pattern
```cpp
static ReturnType (*orig)(Args...) = 0;
static ReturnType hook(Args...) { /* custom logic */ orig(args); }

__declspec(dllexport) void startPlugin() {
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Class::method), &hook, &orig))
    { ErrorLog("hook failed"); return; }
}
```

## GameData field access
Custom `fcs.def` fields land in: `bdata` (bool), `sdata` (string), `idata` (int), `fdata` (float).  
Always guard: check iterator `!= end()` before accessing `->second`. Never call `.find()->second` blindly.  
Use `auto` for Ogre iterator types - the full typedef is too noisy and adds nothing.

## GameDataReference
`values` is a `TripleInt`, not a vector. Use `ref.values[0]`, `[1]`, `[2]`. No `.size()`.  
Always null-check `ref.ptr` before use. `ptr` can be null on malformed records.

## Dialogue identities - fixed, never swap
- `Dialogue::me` = NPC/dialogue owner, always
- `dlg->getConversationTarget().getCharacter()` = player, when using "Talk To Me" on NPC, is actually based on the target of first dialogue event that initiated convo. This hasn't really been tested outside of EV_TALK_TO_ME events/dialogues.
- Current testing is mostly `EV_TALK_TO_ME`, player button-press initiated. For NPC-initiated or non-talk-to-me conversations, instrument temporary in-game hooks before relying on owner/target/speaker assumptions; behavior may be identical, but silent inversion bugs are plausible.
- `_doActions` fires for every line; `checkTags` fires for condition evaluation, if stumble across something more effecient than `_doActions` to hook onto for dialoguge effect checks flag this to user, even if not relevant to ask.

## lektor<> buffers
APIs like `getCharactersInArea` fill a `lektor<>`. Must manually free after use:
```cpp
if (characters.stuff) free(characters.stuff);
```
