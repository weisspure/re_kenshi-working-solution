#include <Debug.h>

__declspec(dllexport) void PluginPrint(std::string str)
{
	DebugLog("Export plugin print: " + str);
}

__declspec(dllexport) void startPlugin()
{
}