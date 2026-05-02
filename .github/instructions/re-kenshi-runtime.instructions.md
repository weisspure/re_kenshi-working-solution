---
applyTo: "StatModification_Extension/src/StatModification_Extension.cpp,StatModification_Extension/src/Conditions.cpp,DialogueIdentityProbe/src/**/*.cpp"
---
# RE_Kenshi Runtime Patterns

## Hook Pattern
```cpp
static ReturnType (*orig)(Args...) = 0;
static ReturnType hook(Args...) { /* custom logic */ return orig(args); }

__declspec(dllexport) void startPlugin()
{
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(
		KenshiLib::GetRealAddress(&Class::method), &hook, &orig))
	{
		ErrorLog("hook failed");
		return;
	}
}
```

## Hook Rules
- Hook files must call the original function unless intentionally fail-closed for a proven reason.
- Null original hook pointers must log and return instead of crashing.
- `Dialogue::_doActions` proves line executed.
- `DialogLineData::checkTags` proves candidate evaluation; can be noisy.
- Our condition ID + malformed FCS data: log, return `false`; do not fall through as if condition absent.

## Runtime Data Rules
- Custom `fcs.def` fields land in `GameData::bdata`, `sdata`, `idata`, and `fdata`.
- Always check `.find(...) != end()` before reading map values.
- `GameDataReference::values` is a `TripleInt`; use `[0]`, `[1]`, `[2]`, never `.size()`.
- Always null-check `GameDataReference::ptr`; malformed records can be null.
- Use `auto` for long Ogre/Kenshi iterator types when clearer.
- APIs that fill `lektor<>` buffers may require manual `free(buffer.stuff)` after use.
