#define _HAS_STD_BYTE 0

// forward declares
void LoadConfigs(); void SaveConfigs();



#include "../pch.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
//#pragma comment(lib, "winhttp.lib") // done in project settings

using namespace std;
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
//#include <Windows.h>
#include <winhttp.h>

#include <objbase.h>   // StringFromGUID2
//#include <atlstr.h>

#include "Hooking/LooseStructs.h"
#include "Loggers/EventLog.h"
#include "Loggers/DumpLog.h"
#include "Loggers/PerformanceLog.h"
#include "Loggers/SocketLog.h"
#include "Loggers/HTTPLog.h";
#include "Hooking/DebugUtils.h"
#include "Hooking/Hooks.h"

#include "GUI/Window.h"
#include "SaveState.h"

#include <thread>

void Main() {
    LoadConfigs(); // make sure that our preferences are loaded before we actually do anything that outputs to log.
    InitEventLog();
    LoadHooks();

    std::thread(injected_window_main).detach();
    LogEntry("Finished Init", t_generic_log);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved){
    if (GetEnvironmentVariableA("DLL_DO_NOT_INIT", 0, 0)) return TRUE;

    switch (ul_reason_for_call){
    case DLL_PROCESS_ATTACH:
        LogEntry("Process Attach", t_generic_log);
        Main();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        LogEntry("Process Detach", t_generic_log);
        UnloadHooks();
        break;
    }
    return TRUE;
}
