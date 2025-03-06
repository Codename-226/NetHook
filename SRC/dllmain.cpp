#include "../pch.h"
using namespace std;
#include <vector>
#include "Loggers/EventLog.h"
#include "Hooking/Hooks.h"

#include "GUI/Window.h"

#include <thread>


void Main() {
    InitEventLog();
    LoadHooks();

    //std::thread t1(injected_window_main);
    LogEntry("Finished Init");
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved){
    if (GetEnvironmentVariableA("DLL_DO_NOT_INIT", 0, 0)) return TRUE;

    switch (ul_reason_for_call){
    case DLL_PROCESS_ATTACH:
        LogEntry("Process Attach");
        Main();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        LogEntry("Process Detach");
        UnloadHooks();
        break;
    }
    return TRUE;
}
