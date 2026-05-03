
## Import/Export Plugins
These plugins showcase exporting/importing functions from other plugins.

When RE_Kenshi loads a plugin, that plugin's folder is added to the DLL linking/loading search path, allowing plugins from one mod to link against plugins in another mod. This allows for communication/RPC between plugins/mods. Plugin folders are added to the search path as the plugin is loaded, meaning the `PluginExport` has to be loaded before the `PluginImport` in order for the two to link against each other. 

In order to ensure the `PluginExport` is loaded first, it is loaded as a preload plugin by using `"PreloadPlugins"` instead of `"Plugins"` in it's `RE_Kenshi.json` config file. Preload plugins are loaded directly after RE_Kenshi and KenshiLib have finished initializing, which should be before Kenshi opens it's launcher window, and before other Ogre plugins are loaded. Preload plugins are always-on as they are loaded before users enable/disable mods in the launcher, i.e. if a mod defines a preload plugin, the preload plugin will always load, but the rest of the mod will only load if the mod is enabled in the launcher.

## Install steps:

Compile.

Copy `KenshiLib_Examples/PluginExport/PluginExport/` to `[Kenshi install dir]/mods/PluginExport/`

Copy `KenshiLib_Examples/PluginImport/PluginImport/` to `[Kenshi install dir]/mods/PluginImport/`

Copy `KenshiLib_Examples/x64/PluginExport/PluginExport.dll` to `[Kenshi install dir]/mods/PluginExport/PluginExport.dll`

Copy `KenshiLib_Examples/x64/PluginImport/PluginImport.dll` to `[Kenshi install dir]/mods/PluginImport/PluginImport.dll`

Run RE_Kenshi and enable the mods via Kenshi's `Mods` tab.
