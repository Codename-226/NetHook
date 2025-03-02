#include "../pch.h"
using namespace std;
#include <vector>
#include "Loggers/EventLog.h"
#include "Hooking/Hooks.h"



// Forward declaration for DetourMessageBoxW
extern "C" int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);



void Main() {
    InitEventLog();
    LoadHooks();
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved){
    switch (ul_reason_for_call){
    case DLL_PROCESS_ATTACH:
        Main();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        UnloadHooks();
        break;
    }
    return TRUE;
}
