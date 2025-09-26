#pragma once
#include <tlhelp32.h>
#include <psapi.h>
#include <winternl.h>

std::vector<HANDLE> suspended_threads = {}; // ResumeThread(hThread);
bool is_process_suspended = false;


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