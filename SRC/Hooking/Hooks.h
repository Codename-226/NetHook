#pragma once
#include <winsock2.h>
#include "../vcpkg_installed/x64-windows/include/MinHook.h"


template <typename T>
inline bool HookMacro(LPVOID src_func, LPVOID override_func, T** src_func_call_ptr ) {
    // Create a hook for MessageBoxW, in disabled state.
    if (MH_CreateHook(src_func, override_func, reinterpret_cast<LPVOID*>(&src_func_call_ptr)) != MH_OK) 
        return 0;
    // Enable the hook for MessageBoxW.
    if (MH_EnableHook(src_func) != MH_OK)
        return 0;
}

typedef int (WSAAPI* send_func)(SOCKET, const char*, int, int);
send_func send_func_ptr = NULL;
int WSAAPI hooked_send(_In_ SOCKET s, _In_reads_bytes_(len) const char FAR* buf, _In_ int len, _In_ int flags) {



    return send_func_ptr(s, buf, len, flags);
}




bool LoadHooks() {
    if (MH_Initialize() != MH_OK) return 0;

    if (!HookMacro(&send, &hooked_send, &send_func_ptr)) return 0;


}
bool UnloadHooks() {

    MH_Uninitialize();
}