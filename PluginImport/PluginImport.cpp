#include <PluginExport.h>

__declspec(dllexport) void startPlugin()
{
	PluginPrint("Hello from PluginImport!");
}