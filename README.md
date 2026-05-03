## KenshiLib Examples
This repo contains example KenshiLib plugins.

Requires [RE_Kenshi 0.3.1+](https://www.nexusmods.com/kenshi/mods/847?tab=files) and optionally [FCS extended 1.0.3.0+](https://www.nexusmods.com/kenshi/mods/1825).

## Examples
### Hello World
A basic plugin that prints "Hello world!" to RE_Kenshi's debug log.

[Readme](HelloWorld/README.md).

### Kill Button
Adds a UI window with a large button that kills the selected character. Showcases how to use MyGUI to safely create working UI elements.

[Readme](KillButton/README.md).

### Dialogue Extensions
Adds new dialogue conditions and effects. Also shows accessing custom GameData properties. This mod also has a corresponding FCS_extended plugin `Dialogue_FCS`.

[Readme](Dialogue/README.md).

### World States Variables
Adds integer variables defined in the FCS, modified via dialogue events, that can be checked in world states. This mod shows how to use custom GameData types and save persistent state. This mod also has a corresponding FCS_extended plugin `WorldStates_FCS`.

[Readme](WorldStates/README.md).

### Plugin import / Plugin export
Showcases communication between plugins/how plugins can export new APIs, as well as the `PreloadPlugin` system for early plugin loading.

[Readme](PluginExport/Readme.md).

### Character Highlight
Source code for the character highlight mod, shows how to set material/shader properties.

## Compiling

Requires Visual Studio 2019 or newer and the Visual C++ 2010 x64 compilers. KenshiLib plugins MUST be compiled using the Visual Studio 2010 compiler. Copies of Visual Studio 2010 can be found on the [Wayback Machine](https://archive.org/search?query=visual+studio+2010).

![Image](https://github.com/user-attachments/assets/fd4db477-d0dc-4449-99c9-8b343c95a5a1)

Other dependencies can be found here: https://github.com/BFrizzleFoShizzle/KenshiLib_Examples_deps

Open the project and compile in RELEASE mode. DEBUG is currently broken.

**If you have issues, a more detailed setup guide [can be found here](https://github.com/weisspure/re_kenshi-working-solution/blob/main/README.md).**
