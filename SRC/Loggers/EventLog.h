#pragma once
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>


char* buffer = 0;
int pages_allocated = 1;
const int page_size = 0xffff; // NOTE: should be large enough to fit the largest possible log entry (probably 128 bytes?)
int used = 0;

void InitEventLog() {
	buffer = (char*)malloc(pages_allocated * page_size);
	buffer[0] = '\0';
}
void ResizeEventLog() {
	char* new_buffer = (char*)malloc((pages_allocated+1) * page_size);
	memcpy(new_buffer, buffer, pages_allocated * page_size);

	free(buffer);

	buffer = new_buffer;
	buffer[used] = '\0';
	pages_allocated += 1;
}
inline void CheckAdjustEventLog(int size_of_next_content) {
	if (pages_allocated * page_size <= used + size_of_next_content)
		ResizeEventLog();
}


void LogTimestamp() {
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%H:%M:%S");
	auto str = oss.str();
	memcpy(buffer + used, str.c_str(), 8);
	buffer[used + 8] = ' ';
	buffer[used + 9] = '|';
	buffer[used + 10] = ' ';
	used += 11;
}

class param_entry { public: const char* desc; uint64_t data; };
void LogParamsEntry(const char* label, vector<param_entry> params) {
	if (!buffer) return;

	int label_size = (int)strlen(label);

	int total_size = 8 + 3 + 1+label_size+1 + 1;
	for (int i = 0; i < params.size(); i++)
		total_size += 1 + (int)strlen(params[i].desc) + 1 + 16;
	total_size += 1 + 1; // newline character + null terminator

	// verify buffer has enough room, if not then increase size
	CheckAdjustEventLog(total_size);

	// write timestamp
	LogTimestamp();
	buffer[used] = '[';
	used += 1;

	// write label
	memcpy(buffer + used, label, label_size);
	buffer[used + label_size    ] = ']';
	buffer[used + label_size + 1] = ':';
	used += label_size + 2;

	// write params
	for (int i = 0; i < params.size(); i++) {
		// write desc
		buffer[used] = ' ';
		int desc_len = strlen(params[i].desc);
		memcpy(buffer + used + 1, params[i].desc, desc_len);
		used += 1 + desc_len;
		buffer[used] = ' ';
		used += 1;

		// write value
		sprintf(buffer + used, "%016llx", params[i].data);
		used += 16;
	}


	// finish line
	buffer[used]   = '\n';
	buffer[used+1] = '\0';
	used += 1;

	// example output // HH:MM:SS - [LABEL]: param1 0000000000000001 param2 0000000000000002
}

void LogEntry(const char* entry) {
	if (!buffer) return;

	int label_size = (int)strlen(entry);
	int total_size = 8 + 3 + label_size + 2; // newline + null terminator at the end

	// verify buffer has enough room, if not then increase size
	CheckAdjustEventLog(total_size);

	// write timestamp
	LogTimestamp();

	// write label
	memcpy(buffer + used, entry, label_size);
	used += label_size;


	// finish line
	buffer[used]   = '\n';
	buffer[used+1] = '\0';
	used += 1;
}