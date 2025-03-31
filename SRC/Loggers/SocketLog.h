#pragma once

#include <chrono>
long long nanoseconds() {
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}
long long seconds() {
	auto now = std::chrono::system_clock::now();
	return (long long)std::chrono::system_clock::to_time_t(now);
}
long long IO_history_timestamp() {
	// currently once per second
	//auto now = std::chrono::system_clock::now();
	//return (long long)std::chrono::system_clock::to_time_t(now);
	// 10 times a second
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 100;
	// ???????? times a second
	//auto now = std::chrono::high_resolution_clock::now();
	//return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}
std::string nanosecondsToTimestamp(long long nanoseconds) {
	auto seconds = std::chrono::seconds(nanoseconds / 1'000'000'000);
	std::chrono::system_clock::time_point timePoint(seconds);
	std::time_t timeT = std::chrono::system_clock::to_time_t(timePoint);
	std::stringstream timestamp;

	tm time;
	localtime_s(&time, &timeT);
	timestamp << std::put_time(&time, "%H:%M:%S");
	return timestamp.str();
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
class socentry_recv {
public:
	int flags;
	vector<char> buffer;
};
class socentry_recvfrom {
public:
	int flags;
	vector<char> buffer;
	vector<char> from;
};
class socentry_wsarecv {
public:
	int flags;
	int out_flags;
	int bytes_recived;
	void* completion_routine;
	vector<vector<char>> buffer;
};
class socentry_wsarecvfrom {
public:
	int flags;
	int out_flags;
	int bytes_recived;
	void* completion_routine;
	vector<vector<char>> buffer;
	vector<char> from;
};


class socket_log_entry_data {
public:
	union {
		socentry_send send;
		socentry_sendto sendto;
		socentry_wsasend wsasend;
		socentry_wsasendto wsasendto;
		socentry_wsasendmsg wsasendmsg;

		socentry_recv recv;
		socentry_recvfrom recvfrom;
		socentry_wsarecv wsarecv;
		socentry_wsarecvfrom wsarecvfrom;
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
	int error_code;
};

const int io_history_count = 120;
class IOLog {
public:
	float data[io_history_count] = {0};
	long long last_update_timestamp = 0;
	long long total = 0;
	int total_logs = 0;

	// only accessed by the UI window, to cache stuff for display
	float cached_data[io_history_count] = { 0 };
	long long cached_timestamp = 0;
	float cached_peak = 0.0f;
	float cached_average = 0.0f;
	float cached_total = 0.0f;

	void log(long long value) {
		total += value;
		total_logs++;

		long long current_timestamp = IO_history_timestamp();
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
	// NOTE: since the adding part isn't even used, we just copy
	void add_to(float* output_buffer, long long timestamp) { // NOTE: size must = io_history_count // NOTE: this is called from other threads, so this is a readonly function
		long long steps_since_last_update = timestamp - last_update_timestamp;

		// determine how many steps behind this buffer is
		if (steps_since_last_update == 0)
			memcpy(output_buffer, data, io_history_count);
		else if (steps_since_last_update > 0) {
			if (steps_since_last_update < io_history_count) {
				memcpy(output_buffer, (char*)(data) + (steps_since_last_update*4), (io_history_count - steps_since_last_update) * 4);
				memset((char*)(output_buffer) + ((io_history_count - steps_since_last_update)*4), 0, steps_since_last_update * 4);
			} 
			else memset(output_buffer, 0, io_history_count * 4);
		}
		// i hope that this isn't possible
		else if (steps_since_last_update < 0) {
			steps_since_last_update = abs(steps_since_last_update);
			if (steps_since_last_update < io_history_count) {
				memcpy((char*)(output_buffer) + (steps_since_last_update*4), data, (io_history_count - steps_since_last_update) * 4);
				memset(output_buffer, 0, steps_since_last_update * 4);
			}
			else memset(output_buffer, 0, io_history_count * 4);
		}
		//if (steps_since_last_update >= 0)
		//	// shift all entries back by X amount
		//	for (int i = steps_since_last_update; i < io_history_count; i++)
		//		output_buffer[i-steps_since_last_update] = data[i];
		//else
		//	// shift all entries back by X amount
		//	for (int i = 0; i < io_history_count + steps_since_last_update; i++)
		//		output_buffer[i-steps_since_last_update] = data[i];
	}
	void refresh_cache(long long timestamp) {
		if (timestamp == cached_timestamp) return;

		cached_timestamp = timestamp;
		add_to(cached_data, cached_timestamp);

		// regenerate cached evaluted stats
		cached_peak = 0.0f;
		cached_total = 0.0f;
		for (int i = 0; i < io_history_count; i++) {
			float current_val = cached_data[i];
			cached_total += current_val;
			if (cached_peak < current_val)
				cached_peak = current_val;
		}
		cached_average = cached_total /(float)io_history_count;
	}
};

const int socket_custom_label_len = 256;
class SocketLogs {
public:
	SOCKET s = 0;
	bool hidden = false;
	char custom_label[socket_custom_label_len] = {0};
	long long timestamp = 0;
	socket_state state = s_unknown;
	vector<socket_log_entry*> events = {};
	IOLog total_send_log = {};
	IOLog total_recv_log = {};

	IOLog send_log = {};
	IOLog sendto_log = {};
	IOLog wsasend_log = {};
	IOLog wsasendto_log = {};
	IOLog wsasendmsg_log = {};

	IOLog recv_log = {};
	IOLog recvfrom_log = {};
	IOLog wsarecv_log = {};
	IOLog wsarecvfrom_log = {};
};




unordered_map<SOCKET, SocketLogs*> logged_sockets = {};
IOLog global_io_send_log = {};
IOLog global_io_recv_log = {};


socket_log_entry_data* LogSocketEvent(SOCKET s, socket_event_type type, const char* label, SocketLogs** output_container, int error_code) {

	SocketLogs* log_container = 0;
	// find or create socket log struct
	auto it = logged_sockets.find(s);
	if (it != logged_sockets.end()) {
		log_container = it->second;
	} else {
		log_malloc(sizeof(SocketLogs));
		log_container = new SocketLogs();
		log_container->timestamp = nanoseconds(); 
		log_container->s = s;
		logged_sockets[s] = log_container;
	}

	// create log event stuff
	log_malloc(sizeof(socket_log_entry));
    
	socket_log_entry* new_socket_event = (socket_log_entry*)malloc(sizeof(socket_log_entry));
	memset(new_socket_event, 0, sizeof(socket_log_entry));
	new_socket_event->type = type;
	new_socket_event->name = label;
	new_socket_event->timestamp = nanoseconds();
	new_socket_event->callstack = callstack();
	new_socket_event->error_code = error_code;
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