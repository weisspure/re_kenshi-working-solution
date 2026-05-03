# RE_Kenshi Runtime Patterns

Use for: runtime hooks in StatModification and DialogueIdentityProbe C++ files.

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
- Call original hook unless intentionally fail-closed for a proven reason.
- If original hook pointer is null, log and return instead of crashing.
- `Dialogue::_doActions` proves line execution.
- `DialogLineData::checkTags` proves candidate evaluation and can be noisy.
- For our condition ID + malformed FCS data: log and return `false`; do not fall through as if condition absent.

## Runtime Data Rules
- Custom `fcs.def` fields land in `GameData::bdata`, `sdata`, `idata`, and `fdata`.
- Check `.find(...) != end()` before reading map values.
- `GameDataReference::values` is `TripleInt`; use `[0]`, `[1]`, `[2]`, never `.size()`.
- Null-check `GameDataReference::ptr`; malformed references can be null.
- Use `auto` for long Ogre/Kenshi iterator types when it improves readability.
- APIs filling `lektor<>` buffers may require manual `free(buffer.stuff)` after use.
