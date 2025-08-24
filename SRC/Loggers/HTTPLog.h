#pragma once



const int buffer_size = 0xffffff;
static char buffer[buffer_size];
// get all events of a HTTP socket
string FormatHTTPEvents(SocketLogs* s) {
	buffer[0] = 0;
	int buffer_index = 0;

	for (int i = 0; i < s->events.size(); i++) {
		socket_log_entry* curr_loggy = s->events[i];

		int bytes_written = 0;
		switch (curr_loggy->type) {
		case t_http_open:
			bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
				">>>>>>>>>>\\> HTTP Session Opened.\nACCESS:0x%x\n", 
				curr_loggy->data.http_open.access_type);
		case t_http_close:
			bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
				">>>>>>>>>>\\> Handle Closed.\n");
			break;
		case t_http_connect:
			bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
				">>>>>>>>>>\\> Opening Connection\n%s\nPORT:0x%x\n", 
				curr_loggy->data.http_connect.server_name.c_str(), 
				curr_loggy->data.http_connect.server_port);
			break;
		case t_http_open_request:
			bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
				">>>>>>>>>>\\> Opening Request\nVERB:%s\nOBJECT:%s\nVERSION:%s\nREFERRER:%s\nflags:0x%x\n",
				curr_loggy->data.http_open_request.verb.c_str(),
				curr_loggy->data.http_open_request.object_name.c_str(),
				curr_loggy->data.http_open_request.version.c_str(),
				curr_loggy->data.http_open_request.referrer.c_str(),
				curr_loggy->data.http_open_request.flags);
			break;
		case t_http_add_request_headers:
			bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
				">>>>>>>>>>\\> Adding Request Headers\nMODIFIERS:0x%x\n%s\n",
				curr_loggy->data.http_add_request_headers.modifiers,
				curr_loggy->data.http_add_request_headers.headers.c_str());
			break;
		case t_http_send_request:
			if (curr_loggy->data.http_send_request.optional_data.size() > 0) {
				bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
					">>>>>>>>>>\\> Sending Request\nLENGTH:0x%x\n%s\nOPTIONAL DATA:\n%s\n",
					curr_loggy->data.http_send_request.total_length,
					curr_loggy->data.http_send_request.headers.c_str(),
					curr_loggy->data.http_send_request.optional_data.c_str());
			} else {
				bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
					">>>>>>>>>>\\> Sending Request\nLENGTH:0x%x\n%s\n",
					curr_loggy->data.http_send_request.total_length,
					curr_loggy->data.http_send_request.headers.c_str());
			} break;
		case t_http_write_data:
			if (curr_loggy->data.http_write_data.data.size() > 0) {
				bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
					">>>>>>>>>>\\> Writing Data\nLENGTH:0x%llx\n%s\n",
					curr_loggy->data.http_write_data.data.size(),
					curr_loggy->data.http_write_data.data.c_str());
			} else {
				bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
					">>>>>>>>>>\\> Writing Data\nLENGTH:0\n");
			} break;
		case t_http_read_data:
			if (curr_loggy->data.http_read_data.data.size() > 0) {
				bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
					">>>>>>>>>>\\> Read Data\nLENGTH:0x%llx\n%s\n",
					curr_loggy->data.http_read_data.data.size(),
					curr_loggy->data.http_read_data.data.c_str());
			} else {
				bytes_written = snprintf(buffer+buffer_index, buffer_size - buffer_index,
					">>>>>>>>>>\\> Read Data\nLENGTH:0\n");
			} break;
		case t_http_query_headers:
			if (curr_loggy->data.http_query_headers.buffer.size() > 0) {
				bytes_written = snprintf(buffer + buffer_index, buffer_size - buffer_index,
					">>>>>>>>>>\\> Querying Headers\nNAME:%s\nINFO:0x%x\nDATA:0x%llx\n%s\n",
					curr_loggy->data.http_query_headers.name,
					curr_loggy->data.http_query_headers.info_level,
					curr_loggy->data.http_query_headers.buffer.size(),
					curr_loggy->data.http_query_headers.buffer.c_str());
			} else {
				bytes_written = snprintf(buffer + buffer_index, buffer_size - buffer_index,
					">>>>>>>>>>\\> Querying Headers\nNAME:%s\nINFO:0x%x\nDATA:0\n",
					curr_loggy->data.http_query_headers.name,
					curr_loggy->data.http_query_headers.info_level);
			} break;
		}
	

		buffer_index += bytes_written;
	}

	return string(buffer);
}

// Print each data type individually

// get all events of all HTTP sockets
void DumpAllHTTPEvents() {
	std::stringstream ss;
	for (const auto& pair : logged_sockets) {
		if (pair.second->source_type != st_WinHttp)
			continue;
		ss << "\n//\n";
		ss << "// HTTP" << to_string((unsigned long long)pair.second->s) << "\n";
		ss << "//\n";
		ss << FormatHTTPEvents(pair.second);
	}

	std::string s = ss.str();
	DumpToNotepad(s);
}


