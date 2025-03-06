#pragma once
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>


const int page_size = 0xffff; // NOTE: should be large enough to fit the largest possible log entry (probably 128 bytes?)
class LogData {
public:
	char* buffer = 0;
	int pages_allocated = 1;
	int used = 0;
};
LogData logs = {};
extern "C" __declspec(dllexport) void* GetLogDataPtr() { return &logs; }

void InitEventLog() {
	logs.buffer = (char*)malloc(logs.pages_allocated * page_size);
	logs.buffer[0] = '\0';
}
void ResizeEventLog() {
	char* new_buffer = (char*)malloc((logs.pages_allocated+1) * page_size);
	memcpy(new_buffer, logs.buffer, logs.pages_allocated * page_size);

	free(logs.buffer);

	logs.buffer = new_buffer;
	logs.buffer[logs.used] = '\0';
	logs.pages_allocated += 1;
}
inline void CheckAdjustEventLog(int size_of_next_content) {
	if (logs.pages_allocated * page_size <= logs.used + size_of_next_content)
		ResizeEventLog();
}


void LogTimestamp() {
	auto t = std::time(nullptr);
	tm tm = {};
	localtime_s(&tm, &t);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%H:%M:%S");
	auto str = oss.str();
	memcpy(logs.buffer + logs.used, str.c_str(), 8);
	logs.buffer[logs.used + 8] = ' ';
	logs.buffer[logs.used + 9] = '|';
	logs.buffer[logs.used + 10] = ' ';
	logs.used += 11;
}

class param_entry { public: const char* desc; uint64_t data; };
void LogParamsEntry(const char* label, vector<param_entry> params) {
	if (!logs.buffer) return;

	int label_size = (int)strlen(label);

	int total_size = 8 + 3 + 1+label_size+1 + 1;
	for (int i = 0; i < params.size(); i++)
		total_size += 1 + (int)strlen(params[i].desc) + 1 + 16;
	total_size += 1 + 1; // newline character + null terminator

	// verify buffer has enough room, if not then increase size
	CheckAdjustEventLog(total_size);

	// write timestamp
	LogTimestamp();
	logs.buffer[logs.used] = '[';
	logs.used += 1;

	// write label
	memcpy(logs.buffer + logs.used, label, label_size);
	logs.buffer[logs.used + label_size    ] = ']';
	logs.buffer[logs.used + label_size + 1] = ':';
	logs.used += label_size + 2;

	// write params
	for (int i = 0; i < params.size(); i++) {
		// write desc
		logs.buffer[logs.used] = ' ';
		logs.used += 1;
		int desc_len = strlen(params[i].desc);
		memcpy(logs.buffer + logs.used, params[i].desc, desc_len);
		logs.used += desc_len;
		logs.buffer[logs.used] = ' ';
		logs.used += 1;

		// write value
		sprintf_s(logs.buffer + logs.used, 17, "%016llx", params[i].data);
		logs.used += 16;
	}


	// finish line
	logs.buffer[logs.used]   = '\n';
	logs.buffer[logs.used+1] = '\0';
	logs.used += 1;

	// example output // HH:MM:SS - [LABEL]: param1 0000000000000001 param2 0000000000000002
}

void LogEntry(const char* entry) {
	if (!logs.buffer) return;

	int label_size = (int)strlen(entry);
	int total_size = 8 + 3 + label_size + 2; // newline + null terminator at the end

	// verify buffer has enough room, if not then increase size
	CheckAdjustEventLog(total_size);

	// write timestamp
	LogTimestamp();

	// write label
	memcpy(logs.buffer + logs.used, entry, label_size);
	logs.used += label_size;


	// finish line
	logs.buffer[logs.used]   = '\n';
	logs.buffer[logs.used+1] = '\0';
	logs.used += 1;
}