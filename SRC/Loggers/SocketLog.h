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

long long tf_change_timestamp = 0;
long long factored_timestamp = 0;
float time_factor = 1.0f;
void IO_UpdateTimeFactor(float new_time_factor) {
	double time = (double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 100.0;
	long long time_elapsed_factored = trunc((time - tf_change_timestamp) * time_factor);
	factored_timestamp += time_elapsed_factored;
	tf_change_timestamp = time;
	time_factor = new_time_factor;
	LogParamsEntry("Time factor has now changed", { param_entry{"new value", (uint64_t)time_factor}, }, t_generic_log);
}
long long IO_history_timestamp() {
	// currently once per second
	//auto now = std::chrono::system_clock::now();
	//return (long long)std::chrono::system_clock::to_time_t(now);
	// 10 times a second
	double time = (double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 100.0;
	long long time_elapsed_factored = trunc((time - tf_change_timestamp) * time_factor);
	return factored_timestamp + time_elapsed_factored;
	// ???????? times a second
	//auto now = std::chrono::high_resolution_clock::now();
	//return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}
std::string secondsToTimestamp(std::chrono::seconds seconds) {
	std::chrono::system_clock::time_point timePoint(seconds);
	std::time_t timeT = std::chrono::system_clock::to_time_t(timePoint);
	std::stringstream timestamp;

	tm time;
	localtime_s(&time, &timeT);
	timestamp << std::put_time(&time, "%H:%M:%S");
	return timestamp.str();
}
std::string nanosecondsToTimestamp(long long nanoseconds) {
	auto seconds = std::chrono::seconds(nanoseconds / 1'000'000'000); 
	return secondsToTimestamp(seconds);
}
std::string HistoryToTimestamp(long long timestamp) {
	auto seconds = std::chrono::seconds(timestamp / 10);
	return secondsToTimestamp(seconds);
}

//#include <sstream>
#include <stacktrace>
char* callstack() {

	auto trace = std::stacktrace::current();

	std::ostringstream oss;
	oss << "Stack Trace:\n";
	for (const auto& entry : trace) {
		oss << entry << "\n";
	}

	auto var = oss.str();

	char* bytes = (char*)malloc(var.size());
	memcpy(bytes, var.c_str(), var.size());
	log_malloc(var.size());
	return bytes;

	//return 0;
}


enum socket_event_category {
	c_create,
	c_send, 
	c_recieve, 
	c_info,
	c_http,
	c_url,
};

enum socket_state {
	s_closed,
	s_open,
	s_unknown,
};
struct __addr {
	short   sin_family;
	u_short sin_port;
	string  IP;
	u_long  sin6_flowinfo;
	u_long  sin6_scope_id;
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
	__addr address;
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
	__addr address;
};
class socentry_wsasendmsg {
public:
	int flags;
	int msg_flags;
	int bytes_sent;
	void* completion_routine;
	vector<vector<char>> buffer;
	vector<char> control_buffer;
	__addr address;
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
	__addr address;
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
	__addr address;
};



class httpentry_open {
public:
	string agent;
	int access_type;
	string proxy;
	string proxy_bypass;
	int flags;
};
class httpentry_close_handle {};
class httpentry_connect {
public:
	string server_name;
	INTERNET_PORT server_port;
	int reserved;
};
class httpentry_open_request {
public:
	string verb;
	string object_name;
	string version;
	string referrer;
	vector<string> accept_types;
	int flags;
};
class httpentry_add_request_headers {
public:
	string headers;
	int modifiers;
};
class httpentry_send_request {
public:
	string headers;
	string optional_data;
	int total_length;
	long long context;
};
class httpentry_write_data {
public:
	string data;
	int bytes_written;
};
class httpentry_read_data {
public:
	string data;
	int bytes_to_read; // this is the intended size of the buffer, not the actual read amount
};
class httpentry_query_headers {
public:
	int info_level;
	string name;
	string buffer;
	int index_in;
	int index_out;
};
class socentry_get_host_by_name {
public:
	string in_name;
	string name;
	vector<string> aliases;
	short addrtype;
	short length;
	vector<string> addr_list;
};
struct _addrinfoW {
public:
	int ai_flags;
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	string name;
	__addr address;
};
class socentry_get_addr_info {
public:
	string node_name;
	string service_name;
	vector<_addrinfoW> results;    
};
class _addrinfoEXW {
public:
	int ai_flags;
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	string name;
	vector<char> ai_blob;
	GUID ai_provider;
	__addr address;
};
class socentry_get_addr_info_ex {
public:
	string node_name;
	string service_name;
	vector<_addrinfoEXW> results;
	DWORD dwNameSpace;
	GUID lpNspId;
	HANDLE lpHandle;
};

typedef struct __ADDR_INFO {
	__addr LocalAddr;
	__addr RemoteAddr;
	INT            iSocketType;
	INT            iProtocol;
};
typedef struct __WSAQuerySetW {
	DWORD dwSize;
	string lpszServiceInstanceName;
	GUID lpServiceClassId;
	WSAVERSION lpVersion;
	string lpszComment;
	DWORD dwNameSpace;
	GUID lpNSProviderId;
	string lpszContext;
	vector<AFPROTOCOLS> afpProtocols;
	string lpszQueryString;
	vector<__ADDR_INFO> csaBuffer;
	DWORD dwOutputFlags;
	vector<char> lpBlob;
};
class socentry_wsa_lookup_service_begin {
public:
	__WSAQuerySetW lpqsRestrictions;
	DWORD dwControlFlags;
	HANDLE lphLookup;
};
class socentry_wsa_lookup_service_next {
public:
	HANDLE hLookup;
	DWORD dwControlFlags;
	DWORD lpdwBufferLength;
	__WSAQuerySetW lpqsResults;
};
class socentry_connect {
public:
	__addr address;
	int namelen;
};

// MCC EXTENSIONS
class __turn_server_info {
public:
	string url;
	string username;
	string password;
};
class socentry_mcc_ns_send {
public:
	vector<char> buffer;
	vector<__turn_server_info> servers;
	string username;
	string password;
	int reliable;
	int param5;
};
class socentry_mcc_wrtc_send {
public:
	vector<char> buffer;
};
class socentry_mcc_signal_send {
public:
	char code;
	GUID guid;
	vector<char> buffer;
};
class socentry_mcc_signal_recv {
public:
	string label_str;
	string response_str;
};



//class httpentry_create_url {
//public:
//	int struct_size;
//	string scheme_name;
//	INTERNET_SCHEME scheme;
//	string host_name;
//	INTERNET_PORT port;
//	string username;
//	string password;
//	string url_path;
//	string extra_info;
//	int flags;
//	string url;
//};
// matches up http connects to their http sessions
map<HINTERNET, HINTERNET> connections_to_sessions = {};
map<HINTERNET, HINTERNET> requests_to_connections = {};
HINTERNET get_connection_parent(HINTERNET connection) {
	auto var = connections_to_sessions[connection];
	// if nothing exists for this guy, just set ourselves to our own parent
	// and we only do this so we can just give everything a single id to group everything with
	if (var) return var;

	return 0;

	//connections_to_sessions[connection] = connection;
	//return connection;
}
HINTERNET get_request_parent(HINTERNET request) {
	auto var = requests_to_connections[request];
	if (!var) { 
		//connections_to_sessions[request] = request; 
		return 0;
	}
	return get_connection_parent(var);
}
HINTERNET get_unknown_parent(HINTERNET hinternet) {
	// first check if this is a request
	auto var = requests_to_connections[hinternet];
	if (var) hinternet = var;
	// then check if this is a connection
	var = connections_to_sessions[hinternet];
	if (var) return var;
	// otherwise we have to assume that its already a session handle
	// however to clean up this crazy mess, we're actually going to make sure that this thing is open in our logs
	for (auto const& x : connections_to_sessions)
		if (x.second == hinternet)
			return hinternet;
	return 0;
}
void http_session_connection_paired(HINTERNET session, HINTERNET connection) {
	connections_to_sessions[connection] = session;
}
void http_request_connection_paired(HINTERNET request, HINTERNET connection) {
	requests_to_connections[request] = connection;
}


// IP address logged stuff
map<string, string> mapped_hosts = { {"8.8.8.8", "google DNS"},  {"2001:4860:4860::8888", "google DNSv6"},  {"127.0.0.1", "localhost"} };
void LogHost(string IP, string host) {
	mapped_hosts[IP] = host;
}
string CheckHost(string IP) {
	return mapped_hosts[IP];
}

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

		httpentry_open http_open;
		httpentry_close_handle http_close;
		httpentry_connect http_connect;
		httpentry_open_request http_open_request;
		httpentry_add_request_headers http_add_request_headers;
		httpentry_send_request http_send_request;
		httpentry_write_data http_write_data;
		httpentry_read_data http_read_data;
		httpentry_query_headers http_query_headers;
		//httpentry_create_url http_create_url;

		socentry_get_host_by_name get_hostbyname;
		socentry_get_addr_info get_addr_info;
		socentry_get_addr_info_ex get_addr_info_ex;
		socentry_wsa_lookup_service_begin wsa_lookup_service_begin;
		socentry_wsa_lookup_service_next wsa_lookup_service_next;

		socentry_connect connect;

		socentry_mcc_ns_send ns_send;
		socentry_mcc_wrtc_send wrtc_send;
		socentry_mcc_signal_send signal_send;
		socentry_mcc_signal_recv signal_recv;
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
		for (long long i = steps_since_last_update; i < io_history_count; i++)
			data[i - steps_since_last_update] = data[i];

		// then reset values of all progressed values?
		for (long long i = io_history_count - steps_since_last_update; i < io_history_count; i++)
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


enum SourceType { st_Socket, st_WinHttp, st_URL, st_ext};
const int socket_custom_label_len = 256;
class SocketLogs {
public:
	SourceType source_type = st_Socket;
	// union because why not
	union {
		SOCKET s;
		HINTERNET i;
	};
	long long timestamp = 0;
	socket_state state = s_unknown;
	vector<socket_log_entry*> events = {};
	IOLog total_send_log = {};
	IOLog total_recv_log = {};

	IOLog send_log = {};		// also HTTP write			| also ns_sent
	IOLog sendto_log = {};		// also HTTP send request	| also wrtc_sent
	IOLog wsasend_log = {};		//							| also sigal_sent
	IOLog wsasendto_log = {};
	IOLog wsasendmsg_log = {};

	IOLog recv_log = {};		// also HTTP read			| also signal_recv
	IOLog recvfrom_log = {};	// also HTTP query headers	|
	IOLog wsarecv_log = {};
	IOLog wsarecvfrom_log = {};

	// UI values
	char custom_label[socket_custom_label_len] = { 0 };
	bool hidden = false;
	int log_event_index = -1;
	int log_event_index_focused = -1;
};




unordered_map<SOCKET, SocketLogs*> logged_sockets = {};
IOLog global_io_send_log = {};
IOLog global_io_recv_log = {};

IOLog global_http_send_log = {};
IOLog global_http_recv_log = {};


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
		// if we created a socket log with the id of -1, then give it a custom label because this is log for special non-socket related events (global events!!!)
		if ((int)s == -1) {
			memcpy(log_container->custom_label, "[GLOBAL EVENTS]", 16);
			log_container->source_type = st_URL;
			log_container->timestamp = 0;
		}
		if ((int)s == -2) {
			memcpy(log_container->custom_label, "[SIMPLENETWORK EVENTS]", 23);
			log_container->source_type = st_ext;
			log_container->timestamp = 0;
		}
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
	case t_http_open:
	case t_http_close:
	case t_http_connect:
	case t_http_open_request:
	case t_http_add_request_headers:
	case t_http_send_request:
	case t_http_write_data:
	case t_http_read_data:
	case t_http_query_headers:
	//case t_http_create_url:
		new_socket_event->category = c_http;
		break;
	case t_get_hostname:
	case t_get_addr_info:
	case t_get_addr_info_ex:
	case t_wsa_lookup_service_begin:
	case t_wsa_lookup_service_next:
		new_socket_event->category = c_url;
		break;
	case t_connect:
		new_socket_event->category = c_create;
		break;
	case t_ns_send:
	case t_wrtc_send:
	case t_signal_send:
		new_socket_event->category = c_send;
		break;
	case t_signal_recv:
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