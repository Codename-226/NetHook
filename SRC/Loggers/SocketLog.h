#pragma once

#include <chrono>
#include <WinSock2.h>
long long nanoseconds() {
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}
long long seconds() {
	auto now = std::chrono::system_clock::now();
	// Convert time_point to time_t, which represents the time in seconds since epoch
	return (long long)std::chrono::system_clock::to_time_t(now);
}

inline char* callstack() {
	log_malloc(0);
	return 0;
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
enum socket_state {
	s_closed,
	s_open,
	s_unknown,
};

socket_log_entry_data* LogSocketEvent(SOCKET s, socket_event_type type, const char* label, SocketLogs** output_container) {

	SocketLogs* log_container = 0;
	// find or create socket log struct
	auto it = logged_sockets.find(s);
	if (it != logged_sockets.end()) {
		log_container = it->second;
	} else {
		log_malloc(sizeof(SocketLogs));
		log_container = new SocketLogs();
		log_container->s = s;
		logged_sockets[s] = log_container;
	}

	// create log event stuff
	log_malloc(sizeof(socket_log_entry));
	socket_log_entry* new_socket_event = new socket_log_entry();
	new_socket_event->type = type;
	new_socket_event->name = label;
	new_socket_event->timestamp = nanoseconds();
	new_socket_event->callstack = callstack();
	switch (type) {
	case t_send:
	case t_send_to:
	case t_wsa_send:
	case t_wsa_send_to:
	case t_wsa_send_msg:
		new_socket_event->category = c_send;
		break;
	case t_recv:
	case t_recv_from:
	case t_wsa_recv:
	case t_wsa_recv_from:
		new_socket_event->category = c_recieve;
		break;
	}
	// append
	log_container->events.push_back(new_socket_event);

	// output container if requested
	if (output_container) *output_container = log_container;

	// return data ptr for us to fill in externally
	return &new_socket_event->data;
}



class socentry_send {
public:
	int flags;
	vector<char> buffer;
};
class socentry_sendto {
public:
	int flags;
	vector<char> buffer;
	vector<char> to;
};
class socentry_wsasend {
public:
	int flags;
	int bytes_sent;
	void* completion_routine;
	vector<vector<char>> buffer;
};
class socentry_wsasendto {
public:
	int flags;
	int bytes_sent;
	void* completion_routine;
	vector<vector<char>> buffer;
	vector<char> to;
};
class socentry_wsasendmsg {
public:
	int flags;
	int msg_flags;
	int bytes_sent;
	void* completion_routine;
	vector<vector<char>> buffer;
	vector<char> control_buffer;
	vector<char> to;
};


class socket_log_entry_data {
public:
	union {
		socentry_send send;
		socentry_sendto sendto;
		socentry_wsasend wsasend;
		socentry_wsasendto wsasendto;
		socentry_wsasendmsg wsasendmsg;
	};
};

class socket_log_entry {
public:
	socket_event_category category;
	socket_event_type type;
	const char* name; 
	long long timestamp;
	char* callstack; // NOTE: has ownership over this char* // MUST CLEANUP
	socket_log_entry_data data;
};

const int io_history_count = 120;
class IOLog {
	float data[io_history_count] = {0};
	long long last_update_timestamp = 0;

	void log(float value) {
		long long current_timestamp = seconds();
		long long steps_since_last_update = current_timestamp - last_update_timestamp;
		if (steps_since_last_update > io_history_count) steps_since_last_update = io_history_count;

		// shift all entries back by X amount
		for (int i = steps_since_last_update; i < io_history_count; i++)
			data[i - steps_since_last_update] = data[i];

		// then reset values of all progressed values?
		for (int i = io_history_count - steps_since_last_update; i < io_history_count; i++)
			data[i] = 0.0f;

		// set new timestamp
		last_update_timestamp = current_timestamp;

		// then log value to latest slot
		data[io_history_count - 1] += value;

	}
	void add_to(float* output_buffer, long long timestamp) { // NOTE: size must = io_history_count // NOTE: this is called from other threads, so this is a readonly function
		long long steps_since_last_update = timestamp - last_update_timestamp;

		if (steps_since_last_update >= 0)
			// shift all entries back by X amount
			for (int i = steps_since_last_update; i < io_history_count; i++)
				output_buffer[i] += data[i - steps_since_last_update];
		else
			// shift all entries back by X amount
			for (int i = 0; i < io_history_count + steps_since_last_update; i++)
				output_buffer[i-steps_since_last_update] += data[i];
	}
};

class SocketLogs {
public:
	SOCKET s = 0;
	socket_state state = s_unknown;
	vector<socket_log_entry*> events = {};
	IOLog send_log = {};
	IOLog sendto_log = {};
	IOLog wsasend_log = {};
	IOLog wsasendto_log = {};
	IOLog wsasendmsg_log = {};
};


unordered_map<SOCKET, SocketLogs*> logged_sockets = {};
IOLog global_io_send_log = {};
IOLog global_io_recv_log = {};
