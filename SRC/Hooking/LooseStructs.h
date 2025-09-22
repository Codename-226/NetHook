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
	t_http_query_headers = 131072,
	//t_http_create_url,
	t_error_log =     0x20000000,
	t_generic_log =   0x40000000,
	t_uncategorized = 0x80000000,
};