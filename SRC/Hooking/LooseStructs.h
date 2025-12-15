#pragma once
enum socket_event_type {
	t_none = 0,
	// transfer: send
	t_send = 1,
	t_send_to = 2,
	t_wsa_send = 4,
	t_wsa_send_to = 8,
	t_wsa_send_msg = 16,
	// transfer: recieve
	t_recv = 32,
	t_recv_from = 64,
	t_wsa_recv = 128,
	t_wsa_recv_from = 256,
	// http: stuff
	t_http_open = 512,
	t_http_close = 1024,
	t_http_connect = 2048,
	t_http_open_request = 4096,
	t_http_add_request_headers = 8192,
	t_http_send_request = 16384,
	t_http_write_data = 32768,
	t_http_read_data = 65536,
	t_http_query_headers = 0x00020000,
	// url stuff
	t_get_hostname =             0x00040000,
	t_get_addr_info =            0x00080000,
	t_get_addr_info_ex =         0x00100000,
	t_wsa_lookup_service_begin = 0x00200000,
	t_wsa_lookup_service_next =  0x00400000,
	t_connect =				     0x00800000,

	// MCC simplenetwork stuff
	t_ns_send = 0x01000000,
	//t_wrtc_create_peer_connection = 0x02000000,
	t_wrtc_recv = 0x02000000,
	t_wrtc_send = 0x04000000,
	t_signal_send = 0x08000000,
	t_signal_recv = 0x10000000,
	//t_http_create_url,
	// generic types
	t_error_log =		0x20000000,
	t_generic_log =		0x40000000,
	t_uncategorized =	0x80000000,
};