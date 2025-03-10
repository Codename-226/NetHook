#pragma once


int ram_allocated = 0;

void log_malloc(int byte_count) {
	ram_allocated += byte_count;
}

