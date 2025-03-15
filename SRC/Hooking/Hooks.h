#pragma once
#include "ParamHelper.h"
#include "../MinHook/MinHook.h"



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
    LogParamsEntry("send()", { 
        param_entry{"socket", (uint64_t)s},
        param_entry{"len", (uint64_t)len},
        param_entry{"|", put_data_into_num(buf, len)},
        param_entry{"-", put_data_into_num(buf+8, len-8)},
        param_entry{"-", put_data_into_num(buf+16, len-16)},
    });
    SocketLogs* log;
    auto event = LogSocketEvent(s, t_send, "send()", &log);
    log->send_log.log(len);
    log->total_send_log.log(len);
    global_io_send_log.log(len);
    event->send.flags = flags;
    event->send.buffer = vector<char>(buf, buf + len);
    log_malloc(len);

    return send_func_ptr(s, buf, len, flags);
}

typedef int (WSAAPI* sendto_func)(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen);
sendto_func sendto_func_ptr = NULL;
int hooked_sendto(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
    LogParamsEntry("sendto()", {
        param_entry{"socket", (uint64_t)s},
        param_entry{"len", (uint64_t)len},
        param_entry{"addr_len", (uint64_t)tolen},
        param_entry{"|", put_data_into_num((char*)to, tolen)},
        param_entry{"|", put_data_into_num(buf, len)},
        param_entry{"-", put_data_into_num(buf + 8, len - 8)},
        param_entry{"-", put_data_into_num(buf + 16, len - 16)},
    });
    SocketLogs* log;
    auto event = LogSocketEvent(s, t_send_to, "sendto()", &log);
    log->sendto_log.log(len);
    log->total_send_log.log(len);
    global_io_send_log.log(len);
    event->sendto.flags = flags;
    event->sendto.buffer = vector<char>(buf, buf + len);
    event->sendto.to = vector<char>((char*)to, (char*)(to) + tolen);
    log_malloc(len);
    log_malloc(tolen); 

    return sendto_func_ptr(s, buf, len, flags, to, tolen);
}

typedef int (WSAAPI* WSAsend_func)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
WSAsend_func WSAsend_func_ptr = NULL;
int hooked_WSAsend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    if (dwBufferCount > 0)
        LogParamsEntry("WSAsend()", {
            param_entry{"socket", (uint64_t)s},
            param_entry{"buffers", (uint64_t)dwBufferCount},
            param_entry{"len", (uint64_t)lpBuffers[0].len},
            param_entry{"|", put_data_into_num(lpBuffers[0].buf, lpBuffers[0].len)},
            param_entry{"-", put_data_into_num(lpBuffers[0].buf + 8, lpBuffers[0].len - 8)},
            param_entry{"-", put_data_into_num(lpBuffers[0].buf + 16, lpBuffers[0].len - 16)},
        });
    else
        LogParamsEntry("WSAsend()", {
            param_entry{"socket", (uint64_t)s},
            param_entry{"buffers", (uint64_t)dwBufferCount},
        });

    auto var = WSAsend_func_ptr(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_send, "wsasend()", &log);
    event->wsasend.buffer = vector<vector<char>>();
    int total_size = 0;
    for (int i = 0; i < dwBufferCount; i++) {
        event->wsasend.buffer.push_back(vector<char>(lpBuffers[i].buf, lpBuffers[i].buf + lpBuffers[i].len));
        log->wsasend_log.log(lpBuffers[i].len);
        log->total_send_log.log(lpBuffers[i].len);
        global_io_send_log.log(lpBuffers[i].len);
        total_size += lpBuffers[i].len;
    }
    log_malloc(total_size);
    event->wsasend.flags = dwFlags;
    event->wsasend.completion_routine = lpCompletionRoutine;
    if (lpNumberOfBytesSent) 
         event->wsasend.bytes_sent = *lpNumberOfBytesSent;
    else event->wsasend.bytes_sent = -1;

    return var;
}

typedef int (WSAAPI* WSAsendto_func)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const sockaddr* lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
WSAsendto_func WSAsendto_func_ptr = NULL;
int hooked_WSAsendto(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const sockaddr* lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {

    if (dwBufferCount > 0)
        LogParamsEntry("WSAsendto()", {
            param_entry{"socket", (uint64_t)s},
            param_entry{"buffers", (uint64_t)dwBufferCount},
            param_entry{"len", (uint64_t)lpBuffers[0].len},
            param_entry{"addr_len", (uint64_t)iTolen},
            param_entry{"|", put_data_into_num((char*)lpTo, iTolen)},
            param_entry{"|", put_data_into_num(lpBuffers[0].buf, lpBuffers[0].len)},
            param_entry{"-", put_data_into_num(lpBuffers[0].buf + 8, lpBuffers[0].len - 8)},
            param_entry{"-", put_data_into_num(lpBuffers[0].buf + 16, lpBuffers[0].len - 16)},
        });
    else
        LogParamsEntry("WSAsendto()", {
            param_entry{"socket", (uint64_t)s},
            param_entry{"buffers", (uint64_t)dwBufferCount},
            param_entry{"addr_len", (uint64_t)iTolen},
            param_entry{"|", put_data_into_num((char*)lpTo, iTolen)},
        });

    auto var = WSAsendto_func_ptr(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iTolen, lpOverlapped, lpCompletionRoutine);

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_send_to, "wsasendto()", &log);
    event->wsasendto.buffer = vector<vector<char>>();
    int total_size = 0;
    for (int i = 0; i < dwBufferCount; i++) {
        event->wsasendto.buffer.push_back(vector<char>(lpBuffers[i].buf, lpBuffers[i].buf + lpBuffers[i].len));
        log->wsasendto_log.log(lpBuffers[i].len);
        log->total_send_log.log(lpBuffers[i].len);
        global_io_send_log.log(lpBuffers[i].len);
        total_size += lpBuffers[i].len;
    }
    log_malloc(total_size);
    event->wsasendto.flags = dwFlags;
    event->wsasendto.completion_routine = lpCompletionRoutine;
    event->wsasendto.to = vector<char>((char*)lpTo, (char*)(lpTo) + iTolen);
    if (lpNumberOfBytesSent)
        event->wsasendto.bytes_sent = *lpNumberOfBytesSent;
    else event->wsasendto.bytes_sent = -1;

    return var;
}

typedef int (WSAAPI* WSAsendmsg_func)(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags, LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
WSAsendmsg_func WSAsendmsg_func_ptr = NULL;
int hooked_WSAsendmsg(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags, LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {

    LogParamsEntry("WSAsendto()", {
        param_entry{"socket", (uint64_t)s},
    });

    auto var = WSAsendmsg_func_ptr(s, lpMsg, dwFlags, lpNumberOfBytesSent, lpOverlapped, lpCompletionRoutine);

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_send_to, "wsasendmsg()", &log);
    event->wsasendmsg.buffer = vector<vector<char>>();
    int total_size = 0;
    for (int i = 0; i < lpMsg->dwBufferCount; i++) {
        event->wsasendmsg.buffer.push_back(vector<char>(lpMsg->lpBuffers[i].buf, lpMsg->lpBuffers[i].buf + lpMsg->lpBuffers[i].len));
        log->wsasendmsg_log.log(lpMsg->lpBuffers[i].len);
        log->total_send_log.log(lpMsg->lpBuffers[i].len);
        global_io_send_log.log(lpMsg->lpBuffers[i].len);
        total_size += lpMsg->lpBuffers[i].len;
    }
    log_malloc(total_size);
    event->wsasendmsg.flags = dwFlags;
    event->wsasendmsg.msg_flags = dwFlags;
    event->wsasendmsg.completion_routine = lpCompletionRoutine;
    event->wsasendmsg.to = vector<char>((char*)lpMsg->name, (char*)(lpMsg->name) + lpMsg->namelen);
    event->wsasendmsg.control_buffer = vector<char>(lpMsg->Control.buf, lpMsg->Control.buf + lpMsg->Control.len);
    if (lpNumberOfBytesSent)
        event->wsasendmsg.bytes_sent = *lpNumberOfBytesSent;
    else event->wsasendmsg.bytes_sent = -1;

    return var;
}





typedef int (WSAAPI* recv_func)(SOCKET s, char* buf, int len, int flags);
recv_func recv_func_ptr = NULL;
int hooked_recv(SOCKET s, char* buf, int len, int flags) {
    LogParamsEntry("recv()", { param_entry{"socket", (uint64_t)s}, });

    auto len_recieved = recv_func_ptr(s, buf, len, flags);

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_recv, "recv()", &log);

    event->recv.flags = flags;
    event->recv.buffer = vector<char>(buf, buf + len_recieved);
    log->recv_log.log(len_recieved);
    log->total_send_log.log(len_recieved);
    global_io_send_log.log(len_recieved);
    log_malloc(len_recieved);


    return len_recieved;
}




bool LoadHooks() {
    if (MH_Initialize() != MH_OK) return false;

    if (!HookMacro(&send, &hooked_send, &send_func_ptr)) return false;
    LogEntry("send hooked");
    //LogParamsEntry("send func hooked", { param_entry{"prev_address", (uint64_t)&send}, param_entry{"override_address", (uint64_t)&hooked_send}, param_entry{"trampoline_address", (uint64_t)&send_func_ptr} });

    if (!HookMacro(&sendto, &hooked_sendto, &sendto_func_ptr)) return false;
    LogEntry("sendto hooked");

    if (!HookMacro(&WSASend, &hooked_WSAsend, &WSAsend_func_ptr)) return false;
    LogEntry("WSAsend hooked");

    if (!HookMacro(&WSASendTo, &hooked_WSAsendto, &WSAsendto_func_ptr)) return false;
    LogEntry("WSAsendto hooked");

    if (!HookMacro(&WSASendMsg, &hooked_WSAsendmsg, &WSAsendmsg_func_ptr)) return false;
    LogEntry("WSAsendmsg hooked");

    return true;
}
bool UnloadHooks() {

    return MH_Uninitialize();
}