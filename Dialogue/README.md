## Dialogue Extensions plugin
This plugin adds new dialogue conditions and effects. Also shows accessing GameData information created in the FCS. This mod also has a corresponding FCS_extended plugin.

Conditions that require accessing tags or character data are implemented in `DialogLineData::checkTags()`.

## Added conditions/effects

DC_IS_SLEEPING - checks if the target character is sleeping.

DC_HAS_SHORT_TERM_TAG - checks if the speaker has a short term tag (`CharacterPerceptionTags_ShortTerm`) for the target character.

DC_IS_ALLY_BECAUSE_OF_DISGUISE - checks if the target is considered an ally because of a disguise, i.e. the target is disguised, is an ally, and would not be an ally if not disguised.

DC_STAT_LEVEL_UNMODIFIED - checks the unmodified stat level of the target.

DC_STAT_LEVEL_MODIFIED - checks the stat level of the target taking into account stat modifiers.

DC_WEAPON_LEVEL - checks the weapon level of the character's currently equipped weapon, taken from the weapon manufacturer model info defined in the FCS.

DC_ARMOUR_LEVEL - checks whether the character has at least one piece of armour with a level satisfying the comparison operator. E.g. armour level > 20 means at least one equipped armour item with a level above 20, armour level < 20 means at least one equipped armour item with a level below 20. 

"take item" - takes a specified number of items from the target character (if they have them) and gives it to the speaker. Items will be placed in the spearker's inventory if there's room, or dropped on the ground if their inventory has insufficient space.

"destroy item" - destroys a specified number of items from the target character (if they have them).

## Install steps:

Compile.

Copy `KenshiLib_Examples/Dialogue/DialogueExtensions/` to `[Kenshi install dir]/mods/DialogueExtensions/`

Copy `KenshiLib_Examples/x64/Release/Dialogue.dll` to `[Kenshi install dir]/mods/DialogueExtensions/Dialogue.dll`

Copy `KenshiLib_Examples/Dialogue_FCS/bin/Release/net4.8/Dialogue_FCS.dll` to `[Kenshi install dir]/mods/DialogueExtensions/Dialogue_FCS.dll`

![Image](https://github.com/user-attachments/assets/cd5c09fd-643d-42d2-8eab-37680645df9e)

Run RE_Kenshi and enable the mod via Kenshi's `Mods` tab.
