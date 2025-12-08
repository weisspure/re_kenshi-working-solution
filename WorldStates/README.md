## World States Variables plugin
This plugin adds integer variables defined in the FCS, modified via dialogue events, that can be checked in world states. This mod also has a corresponding FCS_extended plugin `WorldStates_FCS`.

Note that variables can be negative.

This plugin showcases working with custom GameData types and saving/loading custom persistent game state.

Dialogue conditions are implemented in `DialogLineData::checkTags()` in order to access the underlying GameData object for the dialogue event.

Because there is no easy way to get the corresponding GameData for a WorldEventQueryState object, we hook `WorldEventStateQuery::getFromData()` and create our own mapping. This then allows us to access our custom World State conditions in `WorldEventStateQuery::isTrue()`.

Updated Variable GameData values are saved to `quick.save` by hooking the faction relations saving function, and loaded back in by hooking the game's platoon loading functions. The game stores separate GameData copies for both save data and global game state, data has to be copied between these two in order for the in-game dialogue conditions and world state GameData objects to point to the correct variable GameData objects, and to ensure any loaded save game state overrides variable state inherited from mods.

## Added dialogue + world state conditions/effects

variable equals - checks if a referenced variable is equal to some value

variable greater than - checks if a referenced variable is greater than some value

variable less than - checks if a referenced variable is less than some value

set variable - sets a referenced variable to a specified value

add to variable - adds a specified value to a referenced variable. Subtraction can be done by adding a negative value

## Install steps:

Compile.

Copy `KenshiLib_Examples/WorldStates/WorldStates/` to `[Kenshi install dir]/mods/WorldStates/`

Copy `KenshiLib_Examples/x64/Release/WorldStates.dll` to `[Kenshi install dir]/mods/WorldStates/WorldStates.dll`

Copy `KenshiLib_Examples/WorldStates_FCS/bin/Release/net4.8/WorldStates_FCS.dll` to `[Kenshi install dir]/mods/WorldStates/WorldStates_FCS.dll`

![Image](https://github.com/user-attachments/assets/cd5c09fd-643d-42d2-8eab-37680645df9e)

Run RE_Kenshi and enable the mod via Kenshi's `Mods` tab.
