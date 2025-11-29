#pragma once
// EXTENSION!!!




typedef void* (*ns_send_func)(void* session, char* packet, unsigned long long packet_size, int is_reliable, int param_5);
ns_send_func ns_send_ptr = NULL;
void* hooked_ns_send(void* session, char* packet, unsigned long long packet_size, int is_reliable, int param_5) {
    //LogEntry("hooked_ns_send1()", t_generic_log);
    auto result = ns_send_ptr(session, packet, packet_size, is_reliable, param_5);

    SocketLogs* log;
    auto event = LogSocketEvent(-2, t_ns_send, "ns_send()", &log, 0);

    event->ns_send.buffer = vector<char>(packet, packet + packet_size);
    event->ns_send.reliable = is_reliable;
    event->ns_send.param5 = param_5;

    auto s1 = ((char****)session);
    auto s2 = s1[12];
    auto s3 = s2[24];

    event->ns_send.username = string(s3[41]);
    event->ns_send.password = string(s3[45]);
    // then get the server list
    auto struct_start = (unsigned long long)(s3[22]);
    auto struct_end = (unsigned long long)(s3[23]);

    unsigned long long count = (struct_end - struct_start) / 120;
     
    event->ns_send.servers = {};
    for (int i = 0; i < count; i++) {
        unsigned long long offset = i * 120;

        char* url      = *((char**)(struct_start + offset + 0x00));
        char* username = *((char**)(struct_start + offset + 0x38));
        char* password = *((char**)(struct_start + offset + 0x58));

        event->ns_send.servers.push_back({string(url), string(username), string(password)});
    }

    log->send_log.log(packet_size);
    log->total_send_log.log(packet_size);

    // do not log IO transaction
    LogParamsEntry("ns_send()", {}, t_ns_send);
    return result;
}

typedef void* (*wrtc_send_func)(void* param_1, char* buffer_struct);
wrtc_send_func wrtc_send_ptr = NULL;
void* hooked_wrtc_send(void* param_1, char* buffer_struct) {
    //LogEntry("hooked_wrtc_send1()", t_generic_log);
    auto result = wrtc_send_ptr(param_1, buffer_struct);

    SocketLogs* log;
    auto event = LogSocketEvent(-2, t_wrtc_send, "wrtc_send()", &log, 0);

    auto buffer_ptr = ((char***)buffer_struct)[0][3];
    auto buffer_size = ((int**)buffer_struct)[0][2];
    event->wrtc_send.buffer = vector<char>(buffer_ptr, buffer_ptr + buffer_size);

    log->sendto_log.log(buffer_size);
    log->total_send_log.log(buffer_size);

    LogParamsEntry("wrtc_send()", {}, t_wrtc_send);
    return result;
}



typedef void* (*signal_send_func)(void* param_1, long long code, void* guid, char* buffer);
signal_send_func signal_send_ptr = NULL;
void* hooked_signal_send(void* param_1, long long code, void* guid, char* buffer) {
    //LogEntry("hooked_signal_send1()", t_generic_log);
    auto result = signal_send_ptr(param_1, code, guid, buffer);

    SocketLogs* log;
    auto event = LogSocketEvent(-2, t_signal_send, "signal_send()", &log, 0);

    // if size is less than 16, then it supposedly encodes the string in the first 16 bytes used for pointers
    long long size = ((long long*)buffer)[3];
    char* buffer_actual = buffer;
    if (size > 15) buffer_actual = ((char**)buffer)[0]; // note: in observed structures, this size variable is 1 greater than it should be ??? so theres 2 null terminators instead of 1??
    
    event->signal_send.buffer = vector<char>(buffer_actual, buffer_actual + size);
    event->signal_send.code = (char)code;
    event->signal_send.guid = *(GUID*)guid;

    log->wsasend_log.log(size);
    log->total_send_log.log(size);

    LogParamsEntry("signal_send()", {}, t_signal_send);
    return result;
}

typedef void* (*signal_recv_func)(void* param_1, void* label_str, void* response_str);
signal_recv_func signal_recv_ptr = NULL;
void* hooked_signal_recv(void* param_1, void* label_str, void* response_str) {
    //LogEntry("hooked_signal_recv1()", t_generic_log);
    auto result = signal_recv_ptr(param_1, label_str, response_str);

    SocketLogs* log;
    auto event = LogSocketEvent(-2, t_signal_recv, "signal_recv()", &log, 0);

    int label_alloc_size = ((int*)label_str)[6];
    if (label_alloc_size > 15)
         event->signal_recv.label_str = string(*(char**)label_str);
    else event->signal_recv.label_str = string((char*)label_str);

    int response_alloc_size = ((int*)response_str)[6];
    if (response_alloc_size > 15)
        event->signal_recv.response_str = string(*(char**)response_str);
    else event->signal_recv.response_str = string((char*)response_str);

    log->recv_log.log(event->signal_recv.response_str.length());
    log->total_recv_log.log(event->signal_recv.response_str.length());

    LogParamsEntry("signal_recv()", {}, t_signal_recv);
    return result;
}



void* module_base(const wchar_t* dllName) {
    HMODULE hModule = GetModuleHandleW(dllName);
    if (hModule) return (void*)hModule;
    LogEntry("MCC network DLL not found!!!.", t_error_log);
    return 0;
}
bool LoadHooks_MCC_Webrtc() {
    // hook 
    void* simplenetworkbase = module_base(L"simplenetworklibrary-x64-release.dll");
    LogParamsEntry("simplenetwork module base debugged", { {"simplenetwork base", (uint64_t)simplenetworkbase}}, t_generic_log);

    if (!HookMacro(((char*)simplenetworkbase + 0x0E41C4), &hooked_ns_send, &ns_send_ptr)) return false;
    LogParamsEntry("ns_send hooked", { {"ns_send ptr", (uint64_t)simplenetworkbase + 0x0E41C4} }, t_generic_log);

    if (!HookMacro(((char*)simplenetworkbase + 0x1BD8D8), &hooked_wrtc_send, &wrtc_send_ptr)) return false;
    LogParamsEntry("wrtc_send hooked", { {"wrtc_send ptr", (uint64_t)simplenetworkbase + 0x1BD8D8} }, t_generic_log);

    if (!HookMacro(((char*)simplenetworkbase + 0x0D4130), &hooked_signal_send, &signal_send_ptr)) return false;
    LogParamsEntry("signal_send hooked", { {"signal_send ptr", (uint64_t)simplenetworkbase + 0x0D4130} }, t_generic_log);

    if (!HookMacro(((char*)simplenetworkbase + 0x0D3768), &hooked_signal_recv, &signal_recv_ptr)) return false;
    LogParamsEntry("signal_recv hooked", { {"signal_recv ptr", (uint64_t)simplenetworkbase + 0x0D3768} }, t_generic_log);


    return true;
}