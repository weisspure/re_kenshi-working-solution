#include <Debug.h>

#include <kenshi/TitleScreen.h>
#include <kenshi/Kenshi.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/Damages.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Window.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Delegate.h>

#include <MinHook/include/MinHook.h>
#include <core/Functions.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void OnButtonPress(MyGUI::WidgetPtr sender)
{
    Character* selectedCharacter = Kenshi::GetGameWorld().player->selectedCharacter.getCharacter();
    if (selectedCharacter)
    {
        Damages damage(100,100,100,100,0);
        for (int i = 0; i < selectedCharacter->medical.anatomy.size(); ++i)
            selectedCharacter->medical.anatomy[i]->applyDamage(damage);
    }
}

// Title screen constructor hook
TitleScreen* (*TitleScreen_orig)(TitleScreen*) = NULL;
TitleScreen* TitleScreen_hook(TitleScreen* thisptr)
{
    // Call title screen constructor
    TitleScreen* titleScreen = TitleScreen_orig(thisptr);

    // create UI
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    MyGUI::Window* window = gui->createWidgetReal<MyGUI::Window>("Kenshi_WindowCX", 0.25, 0.25, 0.30, 0.33, MyGUI::Align::Center, "Window", "KillWindow");
    window->setCaption("Kill Button");
    MyGUI::Button* killButton = window->getClientWidget()->createWidgetReal<MyGUI::Button>("Kenshi_Button1", 0.1, 0.1, 0.8, 0.8, MyGUI::Align::Center, "KillButton");
    killButton->setCaption("Kill");
    killButton->eventMouseButtonClick += MyGUI::newDelegate(OnButtonPress);

    return titleScreen;
}

__declspec(dllexport) void startPlugin()
{
    if (MH_Initialize() != MH_OK)
    {
        ErrorLog("KillButton: Could not initialize MinHook");
        return;
    }

    void* titleScreenConstructorPtr = (void*)GetRealAddress(&TitleScreen::_CONSTRUCTOR);
    if (MH_CreateHook(titleScreenConstructorPtr, &TitleScreen_hook, reinterpret_cast<LPVOID*>(&TitleScreen_orig)) != MH_OK)
    {
        ErrorLog("KillButton: Could not create hook");
        return;
    }
    
    if (MH_EnableHook(titleScreenConstructorPtr) != MH_OK)
    {
        ErrorLog("KillButton: Could not enable hook");
        return;
    }
}