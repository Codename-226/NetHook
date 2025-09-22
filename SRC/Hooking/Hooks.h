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
    }, t_send);
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
        }, t_send_to);
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
            }, t_wsa_send);
    else
        LogParamsEntry("WSAsend()", {
            param_entry{"socket", (uint64_t)s},
            param_entry{"buffers", (uint64_t)dwBufferCount},
        }, t_wsa_send);
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
        }, t_wsa_send_to);
    else
        LogParamsEntry("WSAsendto()", {
            param_entry{"socket", (uint64_t)s},
            param_entry{"buffers", (uint64_t)dwBufferCount},
            param_entry{"addr_len", (uint64_t)iTolen},
            param_entry{"|", put_data_into_num((char*)lpTo, iTolen)},
        }, t_wsa_send_to);
    return var;
}

typedef int (WSAAPI* WSAsendmsg_func)(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags, LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
WSAsendmsg_func WSAsendmsg_func_ptr = NULL;
int hooked_WSAsendmsg(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags, LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    auto var = WSAsendmsg_func_ptr(s, lpMsg, dwFlags, lpNumberOfBytesSent, lpOverlapped, lpCompletionRoutine);

    SocketLogs* log;
    auto event = LogSocketEvent(s, t_wsa_send_msg, "wsasendmsg()", &log, var==SOCKET_ERROR ? WSAGetLastError():0);

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

    LogParamsEntry("WSAsendmsg()", { param_entry{"socket", (uint64_t)s}, }, t_wsa_send_msg);
    return var;
}





typedef int (WSAAPI* recv_func)(SOCKET s, char* buf, int len, int flags);
recv_func recv_func_ptr = NULL;
int hooked_recv(SOCKET s, char* buf, int len, int flags) {
    auto len_recieved = recv_func_ptr(s, buf, len, flags);
    if (len_recieved <= 0) return len_recieved; // NOTE: ignores errors!!!!
    
    SocketLogs* log;
    auto event = LogSocketEvent(s, t_recv, "recv()", &log, len_recieved==SOCKET_ERROR ? WSAGetLastError():0);
    
    int actual_bytes_recieved = len_recieved>=0 ? len_recieved:0;
    event->recv.flags = flags;
    event->recv.buffer = vector<char>(buf, buf + actual_bytes_recieved);
    log->recv_log.log(actual_bytes_recieved);
    log->total_recv_log.log(actual_bytes_recieved);
    global_io_recv_log.log(actual_bytes_recieved);
    log_malloc(actual_bytes_recieved);

    LogParamsEntry("recv()", { param_entry{"socket", (uint64_t)s}, }, t_recv);
    return len_recieved;
}

typedef int (WSAAPI* recv_from_func)(SOCKET s, char* buf, int len, int flags, sockaddr* from, int* fromlen);
recv_from_func recv_from_func_ptr = NULL;
int hooked_recv_from(SOCKET s, char* buf, int len, int flags, sockaddr* from, int* fromlen) {
    auto len_recieved = recv_from_func_ptr(s, buf, len, flags, from, fromlen);
    if (len_recieved <= 0) return len_recieved; // NOTE: ignores errors!!!!

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

    LogParamsEntry("recv_from()", { param_entry{"socket", (uint64_t)s}, }, t_recv_from);
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

    LogParamsEntry("wsa_recv()", { param_entry{"socket", (uint64_t)s}, }, t_wsa_recv);
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

    LogParamsEntry("wsa_recv_from()", { param_entry{"socket", (uint64_t)s}, }, t_wsa_recv_from);
    return error;
}




typedef HINTERNET(*win_http_open_func)(LPCWSTR pszAgentW, DWORD dwAccessType, LPCWSTR pszProxyW, LPCWSTR pszProxyBypassW, DWORD dwFlags);
win_http_open_func win_http_open_ptr = NULL;
HINTERNET hooked_win_http_open(LPCWSTR pszAgentW, DWORD dwAccessType, LPCWSTR pszProxyW, LPCWSTR pszProxyBypassW, DWORD dwFlags) {
    auto result = win_http_open_ptr(pszAgentW, dwAccessType, pszProxyW, pszProxyBypassW, dwFlags);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)result, t_http_open, "win_http_open()", &log, 0);

    log->source_type = st_WinHttp;
    event->http_open.agent = LPCWSTRToString(pszAgentW);
    event->http_open.access_type = dwAccessType;
    event->http_open.proxy = LPCWSTRToString(pszProxyW);
    event->http_open.proxy_bypass = LPCWSTRToString(pszProxyBypassW);
    event->http_open.flags = dwFlags;
    LogParamsEntry("win_http_open()", { param_entry{"hInternet", (uint64_t)result}, }, t_http_open);
    return result;
}

typedef BOOL(*win_http_close_handle_func)(HINTERNET hInternet);
win_http_close_handle_func win_http_close_handle_ptr = NULL;
BOOL hooked_win_http_close_handle(HINTERNET hInternet) {
    //LogParamsEntry("begin win_http_close_handle()", {});
    auto result = win_http_close_handle_ptr(hInternet);

    // this should convert whatever the hell this is (hinternet/hconnection/hrequest) into its directly related parent
    auto session = get_unknown_parent(hInternet);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)session, t_http_close, "win_http_close_handle()", &log, 0);
    log->source_type = st_WinHttp;

    LogParamsEntry("win_http_close_handle()", {}, t_http_close);
    return result;
}

typedef HINTERNET(*win_http_connect_func)(HINTERNET hSession, LPCWSTR pswzServerName, INTERNET_PORT nServerPort, DWORD dwReserved);
win_http_connect_func win_http_connect_ptr = NULL;
HINTERNET hooked_win_http_connect(HINTERNET hSession, LPCWSTR pswzServerName, INTERNET_PORT nServerPort, DWORD dwReserved) {
    //LogParamsEntry("begin win_http_connect()", {});
    auto result = win_http_connect_ptr(hSession, pswzServerName, nServerPort, dwReserved);

    http_session_connection_paired(hSession, result);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)hSession, t_http_connect, "win_http_connect()", &log, 0);
    log->source_type = st_WinHttp;

    event->http_connect.server_name = LPCWSTRToString(pswzServerName);
    event->http_connect.server_port = nServerPort;
    event->http_connect.reserved = nServerPort;

    int name_length = event->http_connect.server_name.size();
    //if (!log->custom_label[0] && pswzServerName) {
    if (pswzServerName) {
        memcpy(log->custom_label, event->http_connect.server_name.c_str(), name_length >= socket_custom_label_len ? socket_custom_label_len-1 : name_length);
    }

    LogParamsEntry("win_http_connect()", {}, t_http_connect);
    return result;
}

typedef HINTERNET(*win_http_open_request_func)(HINTERNET hConnect, LPCWSTR pwszVerb, LPCWSTR pwszObjectName, LPCWSTR pwszVersion, LPCWSTR pwszReferrer, LPCWSTR* ppwszAcceptTypes, DWORD dwFlags);
win_http_open_request_func win_http_open_request_ptr = NULL;
HINTERNET hooked_win_http_open_request(HINTERNET hConnect, LPCWSTR pwszVerb, LPCWSTR pwszObjectName, LPCWSTR pwszVersion, LPCWSTR pwszReferrer, LPCWSTR* ppwszAcceptTypes, DWORD dwFlags) {
    //LogParamsEntry("begin win_http_open_request()", {});
    auto result = win_http_open_request_ptr(hConnect, pwszVerb, pwszObjectName, pwszVersion, pwszReferrer, ppwszAcceptTypes, dwFlags);

    auto session = get_connection_parent(hConnect);
    http_request_connection_paired(result, hConnect);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)session, t_http_open_request, "win_http_open_request()", &log, 0);
    log->source_type = st_WinHttp;

    event->http_open_request.verb = LPCWSTRToString(pwszVerb);
    event->http_open_request.object_name = LPCWSTRToString(pwszObjectName);
    event->http_open_request.version = LPCWSTRToString(pwszVersion);
    event->http_open_request.referrer = LPCWSTRToString(pwszReferrer);
    event->http_open_request.flags = dwFlags;

    event->http_open_request.accept_types = vector<string>();
    if (ppwszAcceptTypes) {
        while (*ppwszAcceptTypes) {
            event->http_open_request.accept_types.push_back(LPCWSTRToString(*ppwszAcceptTypes));
            ppwszAcceptTypes++; // this should move the pointer by 8 bytes
    }}


    LogParamsEntry("win_http_open_request()", {}, t_http_open_request);
    return result;
}

typedef BOOL(*win_http_add_request_headers_func)(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwModifiers);
win_http_add_request_headers_func win_http_add_request_headers_ptr = NULL;
BOOL hooked_win_http_add_request_headers(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwModifiers) {
    //LogParamsEntry("begin win_http_add_request_headers()", {});
    auto result = win_http_add_request_headers_ptr(hRequest, lpszHeaders, dwHeadersLength, dwModifiers);

    auto session = get_request_parent(hRequest);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)session, t_http_add_request_headers, "win_http_add_request_headers()", &log, result ? 0 : GetLastError());
    log->source_type = st_WinHttp;

    event->http_add_request_headers.headers = LPCWSTRToString(lpszHeaders, dwHeadersLength);
    event->http_add_request_headers.modifiers = dwModifiers;

    LogParamsEntry("win_http_add_request_headers()", {}, t_http_add_request_headers);
    return result;
}

typedef BOOL(*win_http_send_request_func)(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext);
win_http_send_request_func win_http_send_request_ptr = NULL;
BOOL hooked_win_http_send_request(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext) {
    //LogParamsEntry("begin win_http_send_request()", {});
    auto result = win_http_send_request_ptr(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);

    auto session = get_request_parent(hRequest);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)session, t_http_send_request, "win_http_send_request()", &log, result ? 0 : GetLastError());
    log->source_type = st_WinHttp;

    event->http_send_request.headers = LPCWSTRToString(lpszHeaders, dwHeadersLength);
    event->http_send_request.optional_data = BytesToStringOrHex((char*)lpOptional, dwOptionalLength);
    event->http_send_request.total_length = dwTotalLength;
    event->http_send_request.context = dwContext;

    // log IO transaction
    int data_sent = dwHeadersLength + dwOptionalLength;
    log_malloc(data_sent);
    log->sendto_log.log(data_sent);
    log->total_send_log.log(data_sent);
    global_http_send_log.log(data_sent);

    LogParamsEntry("win_http_send_request()", {}, t_http_send_request);
    return result;
}

typedef BOOL(*win_http_write_data_func)(HINTERNET hRequest, LPCVOID lpBuffer, DWORD dwNumberOfBytesToWrite, LPDWORD lpdwNumberOfBytesWritten);
win_http_write_data_func win_http_write_data_ptr = NULL;
BOOL hooked_win_http_write_data(HINTERNET hRequest, LPCVOID lpBuffer, DWORD dwNumberOfBytesToWrite, LPDWORD lpdwNumberOfBytesWritten) {
    //LogParamsEntry("begin win_http_write_data()", {});
    auto result = win_http_write_data_ptr(hRequest, lpBuffer, dwNumberOfBytesToWrite, lpdwNumberOfBytesWritten);

    auto session = get_request_parent(hRequest);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)session, t_http_write_data, "win_http_write_data()", &log, result ? 0 : GetLastError());
    log->source_type = st_WinHttp;

    event->http_write_data.data = BytesToStringOrHex((char*)lpBuffer, dwNumberOfBytesToWrite);
    event->http_write_data.bytes_written = lpdwNumberOfBytesWritten ? *lpdwNumberOfBytesWritten : 0;

    // log IO transaction
    log_malloc(dwNumberOfBytesToWrite);
    log->send_log.log(dwNumberOfBytesToWrite);
    log->total_send_log.log(dwNumberOfBytesToWrite);
    global_http_send_log.log(dwNumberOfBytesToWrite);

    LogParamsEntry("win_http_write_data()", {}, t_http_write_data);
    return result;
}

typedef BOOL(*win_http_read_data_func)(HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead);
win_http_read_data_func win_http_read_data_ptr = NULL;
BOOL hooked_win_http_read_data(HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead) {
    //LogParamsEntry("begin win_http_read_data()", {});
    auto result = win_http_read_data_ptr(hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
    if (!lpBuffer) return result; // no log if no data

    auto session = get_request_parent(hRequest);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)session, t_http_read_data, "win_http_read_data()", &log, result ? 0 : GetLastError());
    log->source_type = st_WinHttp;

    int size_in_read_buffer = dwNumberOfBytesToRead;
    if (lpdwNumberOfBytesRead) size_in_read_buffer = *lpdwNumberOfBytesRead;

    event->http_read_data.data = BytesToStringOrHex((char*)lpBuffer, size_in_read_buffer);
    event->http_read_data.bytes_to_read = dwNumberOfBytesToRead;

    // log IO transaction
    log_malloc(size_in_read_buffer);
    log->recv_log.log(size_in_read_buffer);
    log->total_recv_log.log(size_in_read_buffer);
    global_http_recv_log.log(size_in_read_buffer);

    LogParamsEntry("win_http_read_data()", {}, t_http_read_data);
    return result;
}

typedef BOOL(*win_http_query_headers_func)(HINTERNET hRequest, DWORD dwInfoLevel, LPCWSTR pwszName, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex);
win_http_query_headers_func win_http_query_headers_ptr = NULL;
BOOL hooked_win_http_query_headers(HINTERNET hRequest, DWORD dwInfoLevel, LPCWSTR pwszName, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex) {
    //LogParamsEntry("begin win_http_query_headers()", {});
    int index_in = lpdwIndex ? *lpdwIndex : 0;
    auto result = win_http_query_headers_ptr(hRequest, dwInfoLevel, pwszName, lpBuffer, lpdwBufferLength, lpdwIndex);
    if (!lpBuffer || !lpdwBufferLength || !*lpdwBufferLength) return result; // skip if we have nothing to log??

    auto session = get_request_parent(hRequest);

    SocketLogs* log;
    auto event = LogSocketEvent((SOCKET)session, t_http_query_headers, "win_http_query_headers()", &log, result ? 0 : GetLastError());
    log->source_type = st_WinHttp;

    event->http_query_headers.info_level = dwInfoLevel;
    event->http_query_headers.name = LPCWSTRToString(pwszName);
    event->http_query_headers.buffer = BytesToStringOrHex((char*)lpBuffer, *lpdwBufferLength);
    event->http_query_headers.index_in = index_in;
    event->http_query_headers.index_out = lpdwIndex ? *lpdwIndex : 0;

    // log IO transaction
    log_malloc(*lpdwBufferLength);
    log->recvfrom_log.log(*lpdwBufferLength);
    log->total_recv_log.log(*lpdwBufferLength);
    global_http_recv_log.log(*lpdwBufferLength);

    LogParamsEntry("win_http_query_headers()", {}, t_http_query_headers);
    return result;
}


//WINHTTPAPI BOOL WinHttpCreateUrl( LPURL_COMPONENTS lpUrlComponents, DWORD dwFlags, LPWSTR pwszUrl, LPDWORD pdwUrlLength);




bool LoadHooks() {
    if (MH_Initialize() != MH_OK) return false;

    if (!HookMacro(&send, &hooked_send, &send_func_ptr)) return false;
    LogEntry("send hooked", t_generic_log);
    //LogParamsEntry("send func hooked", { param_entry{"prev_address", (uint64_t)&send}, param_entry{"override_address", (uint64_t)&hooked_send}, param_entry{"trampoline_address", (uint64_t)&send_func_ptr} });

    if (!HookMacro(&sendto, &hooked_sendto, &sendto_func_ptr)) return false;
    LogEntry("sendto hooked", t_generic_log);

    if (!HookMacro(&WSASend, &hooked_WSAsend, &WSAsend_func_ptr)) return false;
    LogEntry("WSAsend hooked", t_generic_log);

    if (!HookMacro(&WSASendTo, &hooked_WSAsendto, &WSAsendto_func_ptr)) return false;
    LogEntry("WSAsendto hooked", t_generic_log);

    if (!HookMacro(&WSASendMsg, &hooked_WSAsendmsg, &WSAsendmsg_func_ptr)) return false;
    LogEntry("WSAsendmsg hooked", t_generic_log);

    if (!HookMacro(&recv, &hooked_recv, &recv_func_ptr)) return false;
    LogEntry("recv hooked", t_generic_log);

    if (!HookMacro(&recvfrom, &hooked_recv_from, &recv_from_func_ptr)) return false;
    LogEntry("recvfrom hooked", t_generic_log);

    if (!HookMacro(&WSARecv, &hooked_wsa_recv, &wsa_recv_func_ptr)) return false;
    LogEntry("wsarecv hooked", t_generic_log);

    if (!HookMacro(&WSARecvFrom, &hooked_wsa_recv_from, &wsa_recv_from_func_ptr)) return false;
    LogEntry("wsarecvfrom hooked", t_generic_log);

    // set hooks for all win HTTP stuff
    if (!HookMacro(&WinHttpOpen, &hooked_win_http_open, &win_http_open_ptr)) return false;
    LogEntry("http open hooked", t_generic_log);

    if (!HookMacro(&WinHttpCloseHandle, &hooked_win_http_close_handle, &win_http_close_handle_ptr)) return false; 
    LogEntry("http close handle hooked", t_generic_log);

    if (!HookMacro(&WinHttpConnect, &hooked_win_http_connect, &win_http_connect_ptr)) return false;
    LogEntry("http connect hooked", t_generic_log);

    if (!HookMacro(&WinHttpOpenRequest, &hooked_win_http_open_request, &win_http_open_request_ptr)) return false;
    LogEntry("http open request hooked", t_generic_log);
    
    if (!HookMacro(&WinHttpAddRequestHeaders, &hooked_win_http_add_request_headers, &win_http_add_request_headers_ptr)) return false;
    LogEntry("http add request headers hooked", t_generic_log);

    if (!HookMacro(&WinHttpSendRequest, &hooked_win_http_send_request, &win_http_send_request_ptr)) return false;
    LogEntry("http send request hooked", t_generic_log);

    if (!HookMacro(&WinHttpWriteData, &hooked_win_http_write_data, &win_http_write_data_ptr)) return false;
    LogEntry("http write data hooked", t_generic_log);

    if (!HookMacro(&WinHttpReadData, &hooked_win_http_read_data, &win_http_read_data_ptr)) return false;
    LogEntry("http read data hooked", t_generic_log);

    if (!HookMacro(&WinHttpQueryHeaders, &hooked_win_http_query_headers, &win_http_query_headers_ptr)) return false;
    LogEntry("http query headers hooked", t_generic_log);


    return true;
}
bool UnloadHooks() {

    return MH_Uninitialize();
}