#pragma once

#include <chrono>
long long timestamp_now() {
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}


enum socket_event_category {
	c_create,
	c_send, 
	c_recieve, 
	c_info,
};
enum socket_event_type {
	// transfer: send
	t_send,
	t_send_to,
	t_wsa_send,
	t_wsa_send_to,
	t_wsa_send_msg,
	// transfer: recieve
	t_recv,
	t_recv_from,
	t_wsa_recv,
	t_wsa_recv_from,
};

socket_log_entry* LogSocketEvent(socket_event_type type, const char* label) {

}

class socket_log_entry_data {
	union {

	};
};

class socket_log_entry {
	socket_event_category category;
	socket_event_type type;
	const char* name; 
	long long timestamp;
	char* callstack; // NOTE: has ownership over this char* // MUST CLEANUP
	socket_log_entry_data data;
};

const int throughput_track_count = 120;
class ThroughputLog {
	float data[throughput_track_count];
	long long last_update_timestamp;
};

class SocketLogs {
	SOCKET s;
	int state;
	vector<socket_log_entry> events;
};