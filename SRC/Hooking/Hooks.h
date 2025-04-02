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
    auto var = send_func_ptr(s, buf, len, flags);
    int actual_bytes_sent = var==SOCKET_ERROR ? 0:var;
    SocketLogs* log;
    auto event = LogSocketEvent(s, t_send, "send()", &log, var==SOCKET_ERROR ? WSAGetLastError():0);
    log->send_log.log(actual_bytes_sent);
    log->total_send_log.log(actual_bytes_sent);
    global_io_send_log.log(actual_bytes_sent);
    event->send.flags = flags;
    event->send.buffer = vector<char>(buf, buf + len);
    log_malloc(len);

    LogParamsEntry("send()", {
        param_entry{"socket", (uint64_t)s},
        param_entry{"len", (uint64_t)len},
        param_entry{"|", put_data_into_num(buf, len)},
        param_entry{"-", put_data_into_num(buf + 8, len - 8)},
        param_entry{"-", put_data_into_num(buf + 16, len - 16)},
    });
    return var;
}

typedef int (WSAAPI* sendto_func)(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen);
sendto_func sendto_func_ptr = NULL;
int hooked_sendto(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
    auto var = sendto_func_ptr(s, buf, len, flags, to, tolen);
    int actual_bytes_sent = var==SOCKET_ERROR ? 0:var;
    SocketLogs* log;
    auto event = LogSocketEvent(s, t_send_to, "sendto()", &log, var==SOCKET_ERROR ? WSAGetLastError():0);
    log->sendto_log.log(actual_bytes_sent);
    log->total_send_log.log(actual_bytes_sent);
    global_io_send_log.log(actual_bytes_sent);
    event->sendto.flags = flags;
    event->sendto.buffer = vector<char>(buf, buf + len);
    event->sendto.to = vector<char>((char*)to, (char*)(to) + tolen);
    log_malloc(len);

    LogParamsEntry("sendto()", {
        param_entry{"socket", (uint64_t)s},
        param_entry{"len", (uint64_t)len},
        param_entry{"addr_len", (uint64_t)tolen},
        param_entry{"|", put_data_into_num((char*)to, tolen)},
        param_entry{"|", put_data_into_num(buf, len)},
        param_entry{"-", put_data_into_num(buf + 8, len - 8)},
        param_entry{"-", put_data_into_num(buf + 16, len - 16)},
        });
    return var;
}

typedef int (WSAAPI* WSAsend_func)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
WSAsend_func WSAsend_func_ptr = NULL;
int hooked_WSAsend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    auto var = WSAsend_func_ptr(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_send, "wsasend()", &log, var==SOCKET_ERROR ? WSAGetLastError():0);
    if (lpNumberOfBytesSent)
        event->wsasend.bytes_sent = *lpNumberOfBytesSent;
    else event->wsasend.bytes_sent = 0;
    event->wsasend.buffer = vector<vector<char>>();
    int total_size = 0;
    for (unsigned int i = 0; i < dwBufferCount; i++) {
        event->wsasend.buffer.push_back(vector<char>(lpBuffers[i].buf, lpBuffers[i].buf + lpBuffers[i].len));
        total_size += lpBuffers[i].len;
    }
    log_malloc(total_size);
    log->wsasend_log.log(event->wsasend.bytes_sent);
    log->total_send_log.log(event->wsasend.bytes_sent);
    global_io_send_log.log(event->wsasend.bytes_sent);
    event->wsasend.flags = dwFlags;
    event->wsasend.completion_routine = lpCompletionRoutine;

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
    return var;
}

typedef int (WSAAPI* WSAsendto_func)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const sockaddr* lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
WSAsendto_func WSAsendto_func_ptr = NULL;
int hooked_WSAsendto(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const sockaddr* lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    auto var = WSAsendto_func_ptr(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iTolen, lpOverlapped, lpCompletionRoutine);

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_send_to, "wsasendto()", &log, var==SOCKET_ERROR ? WSAGetLastError():0);

    if (lpNumberOfBytesSent)
        event->wsasendto.bytes_sent = *lpNumberOfBytesSent;
    else event->wsasendto.bytes_sent = 0;
    event->wsasendto.buffer = vector<vector<char>>();
    int total_size = 0;
    for (unsigned int i = 0; i < dwBufferCount; i++) {
        event->wsasendto.buffer.push_back(vector<char>(lpBuffers[i].buf, lpBuffers[i].buf + lpBuffers[i].len));
        total_size += lpBuffers[i].len;
    }
    log_malloc(total_size);
    log->wsasendto_log.log(event->wsasendto.bytes_sent);
    log->total_send_log.log(event->wsasendto.bytes_sent);
    global_io_send_log.log(event->wsasendto.bytes_sent);
    event->wsasendto.flags = dwFlags;
    event->wsasendto.completion_routine = lpCompletionRoutine;
    event->wsasendto.to = vector<char>((char*)lpTo, (char*)(lpTo) + iTolen);

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
    return var;
}

typedef int (WSAAPI* WSAsendmsg_func)(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags, LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
WSAsendmsg_func WSAsendmsg_func_ptr = NULL;
int hooked_WSAsendmsg(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags, LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    auto var = WSAsendmsg_func_ptr(s, lpMsg, dwFlags, lpNumberOfBytesSent, lpOverlapped, lpCompletionRoutine);

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_send_to, "wsasendmsg()", &log, var==SOCKET_ERROR ? WSAGetLastError():0);

    if (lpNumberOfBytesSent)
        event->wsasendmsg.bytes_sent = *lpNumberOfBytesSent;
    else event->wsasendmsg.bytes_sent = 0;
    event->wsasendmsg.buffer = vector<vector<char>>();
    int total_size = 0;
    for (unsigned int i = 0; i < lpMsg->dwBufferCount; i++) {
        event->wsasendmsg.buffer.push_back(vector<char>(lpMsg->lpBuffers[i].buf, lpMsg->lpBuffers[i].buf + lpMsg->lpBuffers[i].len));
        total_size += lpMsg->lpBuffers[i].len;
    }
    log_malloc(total_size);
    log->wsasendmsg_log.log(event->wsasendmsg.bytes_sent);
    log->total_send_log.log(event->wsasendmsg.bytes_sent);
    global_io_send_log.log(event->wsasendmsg.bytes_sent);
    event->wsasendmsg.flags = dwFlags;
    event->wsasendmsg.msg_flags = dwFlags;
    event->wsasendmsg.completion_routine = lpCompletionRoutine;
    event->wsasendmsg.to = vector<char>((char*)lpMsg->name, (char*)(lpMsg->name) + lpMsg->namelen);
    event->wsasendmsg.control_buffer = vector<char>(lpMsg->Control.buf, lpMsg->Control.buf + lpMsg->Control.len);

    LogParamsEntry("WSAsendmsg()", { param_entry{"socket", (uint64_t)s}, });
    return var;
}





typedef int (WSAAPI* recv_func)(SOCKET s, char* buf, int len, int flags);
recv_func recv_func_ptr = NULL;
int hooked_recv(SOCKET s, char* buf, int len, int flags) {
    auto len_recieved = recv_func_ptr(s, buf, len, flags);
    if (len_recieved <= 0) return 0; // NOTE: IGNORES ERRORS
    
    SocketLogs* log;
    auto event = LogSocketEvent(s, t_recv, "recv()", &log, len_recieved==SOCKET_ERROR ? WSAGetLastError():0);
    
    int actual_bytes_recieved = len_recieved>=0 ? len_recieved:0;
    event->recv.flags = flags;
    event->recv.buffer = vector<char>(buf, buf + actual_bytes_recieved);
    log->recv_log.log(actual_bytes_recieved);
    log->total_recv_log.log(actual_bytes_recieved);
    global_io_recv_log.log(actual_bytes_recieved);
    log_malloc(actual_bytes_recieved);

    LogParamsEntry("recv()", { param_entry{"socket", (uint64_t)s}, });
    return len_recieved;
}

typedef int (WSAAPI* recv_from_func)(SOCKET s, char* buf, int len, int flags, sockaddr* from, int* fromlen);
recv_from_func recv_from_func_ptr = NULL;
int hooked_recv_from(SOCKET s, char* buf, int len, int flags, sockaddr* from, int* fromlen) {
    auto len_recieved = recv_from_func_ptr(s, buf, len, flags, from, fromlen);
    if (len_recieved <= 0) return 0; // NOTE: IGNORES ERRORS

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_recv_from, "recv_from()", &log, len_recieved==SOCKET_ERROR ? WSAGetLastError():0);

    int actual_bytes_recieved = len_recieved >= 0 ? len_recieved : 0;
    event->recvfrom.flags = flags;
    event->recvfrom.buffer = vector<char>(buf, buf + actual_bytes_recieved);
    if (fromlen && from)
         event->recvfrom.from = vector<char>((char*)from, (char*)(from) + *fromlen);
    else event->recvfrom.from = vector<char>();
    log->recvfrom_log.log(actual_bytes_recieved);
    log->total_recv_log.log(actual_bytes_recieved);
    global_io_recv_log.log(actual_bytes_recieved);
    log_malloc(actual_bytes_recieved);

    LogParamsEntry("recv_from()", { param_entry{"socket", (uint64_t)s}, });
    return len_recieved;
}

typedef int (WSAAPI* wsa_recv_func)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
wsa_recv_func wsa_recv_func_ptr = NULL;
int hooked_wsa_recv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    int in_flags = 0;
    if (lpFlags) in_flags = *lpFlags;
    auto error = wsa_recv_func_ptr(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
    if (!lpNumberOfBytesRecvd || *lpNumberOfBytesRecvd == 0) return error; // NOTE: prevents log from collecting recv errors

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_recv, "wsa_recv()", &log, error==SOCKET_ERROR ? WSAGetLastError():0);

    event->wsarecv.flags = in_flags;
    if (lpFlags)
         event->wsarecv.out_flags = *lpFlags;
    else event->wsarecv.out_flags = 0;
    if (lpNumberOfBytesRecvd)
         event->wsarecv.bytes_recived = *lpNumberOfBytesRecvd;
    else event->wsarecv.bytes_recived = 0;

    event->wsarecv.buffer = vector<vector<char>>();
    int total_size = 0;
    for (unsigned int i = 0; i < dwBufferCount; i++) {
        event->wsarecv.buffer.push_back(vector<char>(lpBuffers[i].buf, lpBuffers[i].buf + lpBuffers[i].len));
        total_size += lpBuffers[i].len;
    }
    log_malloc(total_size);
    log->wsarecv_log.log(event->wsarecv.bytes_recived);
    log->total_recv_log.log(event->wsarecv.bytes_recived);
    global_io_recv_log.log(event->wsarecv.bytes_recived);
    event->wsarecv.completion_routine = lpCompletionRoutine;

    LogParamsEntry("wsa_recv()", { param_entry{"socket", (uint64_t)s}, });
    return error;
}
typedef int (WSAAPI* wsa_recv_from_func)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, sockaddr* lpFrom, LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
wsa_recv_from_func wsa_recv_from_func_ptr = NULL;
int hooked_wsa_recv_from(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, sockaddr* lpFrom, LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    int in_flags = 0;
    if (lpFlags) in_flags = *lpFlags;
    auto error = wsa_recv_from_func_ptr(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);
    if (!lpNumberOfBytesRecvd || *lpNumberOfBytesRecvd == 0) return error; // NOTE: prevents log from collecting recv errors

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_recv_from, "wsa_recv_from()", &log, error == SOCKET_ERROR ? WSAGetLastError() : 0);

    event->wsarecvfrom.flags = in_flags;
    if (lpFlags)
        event->wsarecvfrom.out_flags = *lpFlags;
    else event->wsarecvfrom.out_flags = 0;
    if (lpNumberOfBytesRecvd)
        event->wsarecvfrom.bytes_recived = *lpNumberOfBytesRecvd;
    else event->wsarecvfrom.bytes_recived = 0;

    if (lpFrom && lpFromlen)
        event->recvfrom.from = vector<char>((char*)lpFrom, (char*)(lpFrom)+*lpFromlen);
    else event->recvfrom.from = vector<char>();

    event->wsarecvfrom.buffer = vector<vector<char>>();
    int total_size = 0;
    for (unsigned int i = 0; i < dwBufferCount; i++) {
        event->wsarecvfrom.buffer.push_back(vector<char>(lpBuffers[i].buf, lpBuffers[i].buf + lpBuffers[i].len));
        total_size += lpBuffers[i].len;
    }
    log_malloc(total_size);
    log->wsarecvfrom_log.log(event->wsarecvfrom.bytes_recived);
    log->total_recv_log.log(event->wsarecvfrom.bytes_recived);
    global_io_recv_log.log(event->wsarecvfrom.bytes_recived);
    event->wsarecvfrom.completion_routine = lpCompletionRoutine;

    LogParamsEntry("wsa_recv_from()", { param_entry{"socket", (uint64_t)s}, });
    return error;
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


    if (!HookMacro(&recv, &hooked_recv, &recv_func_ptr)) return false;
    LogEntry("recv hooked");

    if (!HookMacro(&recvfrom, &hooked_recv_from, &recv_from_func_ptr)) return false;
    LogEntry("recvfrom hooked");

    if (!HookMacro(&WSARecv, &hooked_wsa_recv, &wsa_recv_func_ptr)) return false;
    LogEntry("wsarecv hooked");

    if (!HookMacro(&WSARecvFrom, &hooked_wsa_recv_from, &wsa_recv_from_func_ptr)) return false;
    LogEntry("wsarecvfrom hooked");



    return true;
}
bool UnloadHooks() {

    return MH_Uninitialize();
}