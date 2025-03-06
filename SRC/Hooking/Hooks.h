#pragma once
#include "../MinHook/MinHook.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")


template <typename T>
inline bool HookMacro(LPVOID src_func, LPVOID override_func, T** src_func_call_ptr ) {
    // Create a hook in disabled state.
    if (MH_CreateHook(src_func, override_func, (void**)(src_func_call_ptr)) != MH_OK) 
        return false;
    if (MH_EnableHook(src_func) != MH_OK)
        return false;
    return true;
}

typedef int (WSAAPI* send_func)(SOCKET, const char*, int, int);
send_func send_func_ptr = NULL;
int WSAAPI hooked_send(SOCKET s, const char* buf, int len, int flags) {

    LogEntry("send hook triggered");

    return send_func_ptr(s, buf, len, flags);
}




bool LoadHooks() {
    if (MH_Initialize() != MH_OK) return false;

    if (!HookMacro(&send, &hooked_send, &send_func_ptr)) return false;

    LogParamsEntry("send func hooked", { param_entry{"prev_address", (uint64_t)&send}, param_entry{"override_address", (uint64_t)&hooked_send}, param_entry{"trampoline_address", (uint64_t)&send_func_ptr} });


    return true;
}
bool UnloadHooks() {

    return MH_Uninitialize();
}