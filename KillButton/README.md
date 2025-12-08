## Kill Button plugin
This plugin adds a UI window with a large button that kills the selected character.

The main point of this plugin is to showcase how to safely create UI elements. Accessing Kenshi's UI from threads other than the UI thread is generally unsafe and can cause crashes. Because of this, the plugin hooks the title screen constructor so the kill button UI can be created synchronously on Kenshi's UI setup thread.

<img width="596" height="374" alt="Image" src="https://github.com/user-attachments/assets/8c14fc49-cca3-4d0c-8991-35f9606a22a9" />

## Install steps:

Compile.

Copy `KenshiLib_Examples/KillButton/KillButton/` to `[Kenshi install dir]/mods/KillButton/`

Copy `KenshiLib_Examples/x64/KillButton/KillButton.dll` to `[Kenshi install dir]/mods/KillButton/KillButton.dll`

![Image](https://github.com/user-attachments/assets/cd5c09fd-643d-42d2-8eab-37680645df9e)

Run RE_Kenshi and enable the mod via Kenshi's `Mods` tab.
