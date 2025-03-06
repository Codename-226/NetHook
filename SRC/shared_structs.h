#pragma once
const int page_size = 0xffff; // NOTE: should be large enough to fit the largest possible log entry (probably 128 bytes?)
class LogData {
public:
	char* buffer = 0;
	int pages_allocated = 1;
	int used = 0;
};