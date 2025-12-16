#pragma once
#include <tlhelp32.h>
#include <psapi.h>
#include <winternl.h>

std::vector<HANDLE> suspended_threads = {}; // ResumeThread(hThread);
bool is_process_suspended = false;
static bool force_get_socket_id = false;


typedef NTSTATUS(NTAPI* NtGetNextThread_t)(
    HANDLE ProcessHandle,
    HANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    ULONG HandleAttributes,
    ULONG Flags,
    PHANDLE NewThreadHandle
    );

typedef NTSTATUS(NTAPI* NtQueryInformationThread_t)(
    HANDLE ThreadHandle,
    THREADINFOCLASS ThreadInformationClass,
    PVOID ThreadInformation,
    ULONG ThreadInformationLength,
    PULONG ReturnLength
   );
bool IsGraphicsThread(HANDLE hProcess, HANDLE hThread) {
    void* startAddress = nullptr;
    static auto NtQueryInformationThread = (NtQueryInformationThread_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread");
    if (!NtQueryInformationThread) return false;

    NTSTATUS status = NtQueryInformationThread(hThread, (THREADINFOCLASS)9, &startAddress, sizeof(startAddress), nullptr);
    if (status != 0 || !startAddress) return false;

    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQueryEx(hProcess, startAddress, &mbi, sizeof(mbi))) return false;

    char moduleName[MAX_PATH] = {};
    if (GetModuleFileNameA((HMODULE)mbi.AllocationBase, moduleName, MAX_PATH)) {
        std::string mod(moduleName);
        //LogParamsEntry("thread parent module", { param_entry{ mod.c_str(), (uint64_t)0},}, t_generic_log);
        if (mod.find("d3d") != std::string::npos || mod.find("dxgi") != std::string::npos || mod.find("nvwgf2") != std::string::npos) {
            LogParamsEntry("match", { param_entry{ mod.c_str(), (uint64_t)0},}, t_error_log);
            return true;
        }
        return false;
    }
    return false;
}
float last_time_factor = 0.0f;
void FreezeProcess() {
    if (is_process_suspended) return;
    is_process_suspended = true;
    last_time_factor = time_factor;
    IO_UpdateTimeFactor(0.0f);
    LogEntry("Process is now paused", t_generic_log);

    suspended_threads.clear();
    HANDLE hProcess = GetCurrentProcess();
    DWORD currentPID = GetCurrentProcessId();
    DWORD currentThreadID = GetCurrentThreadId();

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        //std::cerr << "Failed to create snapshot." << std::endl;
        return;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(snapshot, &te)) {
        //std::cerr << "Failed to get first thread." << std::endl;
        CloseHandle(snapshot);
        return;
    }
    int owned_threadds = 0;
    int our_threads = 0;
    int successful_threads = 0;
    do {
        if (te.th32OwnerProcessID == currentPID) {
            owned_threadds++;
            if (te.th32ThreadID == currentThreadID) {
                our_threads++;
                continue;
            }
            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
            if (hThread) {
                if (IsGraphicsThread(hProcess, hThread)) {
                    successful_threads++;
                    continue;
                }
                DWORD suspendCount = SuspendThread(hThread);
                if (suspendCount == (DWORD)-1) {
                    LogParamsEntry("Failed to suspend thread ID", { param_entry{"ID", (uint64_t)te.th32ThreadID}, }, t_error_log);
                    CloseHandle(hThread);
                }
                else {
                    suspended_threads.emplace_back(hThread);
                }
            }
            else {
                LogParamsEntry("Failed to open thread ID", { param_entry{"ID", (uint64_t)te.th32ThreadID}, }, t_error_log);
            }
        }
        
    } while (Thread32Next(snapshot, &te));

    LogParamsEntry("thread info", { 
        param_entry{"owned", (uint64_t)owned_threadds},
        param_entry{"ours", (uint64_t)our_threads},
        param_entry{"skipped", (uint64_t)successful_threads},
    }, t_generic_log);
    CloseHandle(snapshot);
}
void ResumeProcess() {
    if (!is_process_suspended) return;
    is_process_suspended = false;
    IO_UpdateTimeFactor(last_time_factor);
    LogEntry("Process is now resumed", t_generic_log);

    for (HANDLE hThread : suspended_threads) {
        if (hThread) {
            DWORD result = ResumeThread(hThread);
            if (result == (DWORD)-1) {
                LogParamsEntry("Failed to resume thread", { param_entry{"ID", (uint64_t)hThread}, }, t_error_log);
            }
        }
    }

    // Optional: Close handles if you're done with them
    for (HANDLE hThread : suspended_threads) {
        if (hThread) {
            CloseHandle(hThread);
        }
    }
}



__addr convert_addr_struct(sockaddr* addr) {
    __addr thingo = {};
    if (!addr) return thingo;
    char str[INET6_ADDRSTRLEN];  // 46 bytes
    if (addr->sa_family == AF_INET) {
        sockaddr_in* ipv4 = (sockaddr_in*)addr;
        thingo.sin_family = ipv4->sin_family;
        thingo.sin_port = ipv4->sin_port;
        thingo.IP = inet_ntop(AF_INET, &ipv4->sin_addr, str, INET_ADDRSTRLEN);
    }
    else if (addr->sa_family == AF_INET6) {
        sockaddr_in6* ipv6 = (sockaddr_in6*)addr;
        thingo.sin_family = ipv6->sin6_family;
        thingo.sin_port = ipv6->sin6_port;
        thingo.sin6_flowinfo = ipv6->sin6_flowinfo;
        thingo.sin6_scope_id = ipv6->sin6_scope_id;
        thingo.IP = inet_ntop(AF_INET6, &ipv6->sin6_addr, str, INET6_ADDRSTRLEN);
    }
    return thingo;
}
bool try_name_log_from_address(string IP, SocketLogs* log) {
    // and now we can apply a name to our socket
    string socket_source_name = CheckHost(IP);
    if (!socket_source_name.empty()) {
        size_t len = socket_source_name.size();
        if (len < sizeof(log->custom_label)) {
            memcpy(log->custom_label, socket_source_name.c_str(), len + 1); // include null terminator
        }
        else {
            // Truncate safely and null-terminate
            memcpy(log->custom_label, socket_source_name.c_str(), sizeof(log->custom_label) - 1);
            log->custom_label[sizeof(log->custom_label) - 1] = '\0';
        }
        return true;
    }
    return false;
}


bool SockenJockey(bool b, SOCKET s, SocketLogs* logs) {

    sockaddr_storage addr; // Use sockaddr_storage for IPv4/IPv6 flexibility
    int addr_len = sizeof(addr);

    if (!getpeername(s, (sockaddr*)(&addr), &addr_len)) {

        auto var = convert_addr_struct((sockaddr*)(&addr)).IP;
        if (!b) {
        dont_resolve:
            // screw it, we can make a function for this some other time.
            int name_len = var.size();
            if (name_len > sizeof(logs->custom_label)) name_len = sizeof(logs->custom_label) - 1;
            memcpy(logs->custom_label, var.c_str(), name_len);
            logs->custom_label[name_len] = '\0';
            return true;
        }
        else {
            if (try_name_log_from_address(var, logs)) {
                LogParamsEntry("resolved socket IP and found cached hostname", { { var.c_str(),0} }, t_generic_log);
                return true;
            }
            else { // we try to get hostname
                char hostname[NI_MAXHOST] = {};
                char servname[NI_MAXSERV] = {};
                if (!getnameinfo((sockaddr*)(&addr), addr_len, hostname, sizeof(hostname), servname, sizeof(servname), NI_NUMERICSERV)) {
                    LogParamsEntry("resolved socket hostname", { { hostname,0}, {servname,0} }, t_generic_log);
                    int name_len = strlen(hostname);
                    if (name_len > sizeof(logs->custom_label)) name_len = sizeof(logs->custom_label) - 1;

                    memcpy(logs->custom_label, hostname, name_len);
                    logs->custom_label[name_len] = '\0';
                    return true;
                }
                else goto dont_resolve;
            }
        }
    }
}

void TryWriteSocketAddresses(bool resolve_address_to_hostname) {
    int total_valid = 0;
    int total_success = 0;
    for (const auto& [sock, logs] : logged_sockets) {
        if (logs && logs->source_type == st_Socket) {
            total_valid++;
            total_success += SockenJockey(resolve_address_to_hostname, sock, logs);
        }
    }
    LogParamsEntry("socket names have been set to their addresses", { { "total sockets", (uint64_t)total_valid}, {"succeeded", (uint64_t)total_success}}, t_generic_log);
}
void check_resolve_sockey(SOCKET s, SocketLogs* logs) {
    if (force_get_socket_id)
        SockenJockey(true, s, logs);
}

bool CopyToClipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) {
        LogEntry("Failed to open clipboard", t_error_log);
        return false;
    }

    if (!EmptyClipboard()) {
        LogEntry("Failed to empty clipboard", t_error_log);
        CloseClipboard();
        return false;
    }

    // Allocate global memory for the string (+1 for null terminator)
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (!hGlob) {
        LogEntry("Clipboard GlobalAlloc failed", t_error_log);
        CloseClipboard();
        return false;
    }

    // Copy string into the allocated memory
    char* buffer = static_cast<char*>(GlobalLock(hGlob));
    memcpy(buffer, text.c_str(), text.size() + 1);
    GlobalUnlock(hGlob);

    // Set clipboard data
    if (!SetClipboardData(CF_TEXT, hGlob)) {
        LogEntry("SetClipboardData failed", t_error_log);
        GlobalFree(hGlob);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

std::string GuidToString(const GUID& guid) {
    wchar_t guidString[39]; // 38 chars + null terminator
    if (StringFromGUID2(guid, guidString, 39)) {
        char charGuidString[39];
        size_t converted = 0;
        errno_t err = wcstombs_s(
            &converted,              // number of bytes converted
            charGuidString,          // destination buffer
            sizeof(charGuidString),  // size of destination buffer
            guidString,              // source wide string
            _TRUNCATE                // convert until buffer full
        );
        if (err == 0) {
            return std::string(charGuidString);
        }
    }
    return "failed guid convert";
}