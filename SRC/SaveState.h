#pragma once
#include <fstream>
void WriteInt(ofstream& file, int value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}
void WriteFloat(ofstream& file, float value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}
void WriteBool(ofstream& file, bool value) {
    WriteInt(file, value);
}
int ReadInt(ifstream& file) {
    int i = 0;
    file.read(reinterpret_cast<char*>(&i), sizeof(i));
    return file ? i : 0;
}
float ReadFloat(ifstream& file) {
    float i = 0;
    file.read(reinterpret_cast<char*>(&i), sizeof(i));
    return file ? i : 0.0f;
}
bool ReadBool(ifstream& file) {
    return ReadInt(file);
}


void SaveConfigs() {
    std::ofstream file("c226configs.bin", std::ios::binary | std::ios::out);
    if (!file) { LogEntry("Failed to open/create configs file!!", t_error_log); return; }

    WriteBool(file, focus_http);
    WriteBool(file, preview_socket_packets_sent);
    WriteBool(file, preview_socket_bytes_sent);
    WriteBool(file, preview_socket_packets_recieved);
    WriteBool(file, preview_socket_bytes_recieved);
    WriteBool(file, preview_socket_timestamp);
    WriteBool(file, preview_socket_status);
    WriteBool(file, preview_socket_event_count);
    WriteBool(file, preview_socket_label);
    WriteBool(file, focused_show_global_send);
    WriteBool(file, focused_show_global_recv);
    WriteBool(file, focused_show_send);
    WriteBool(file, focused_show_sendto);
    WriteBool(file, focused_show_wsasend);
    WriteBool(file, focused_show_wsasendto);
    WriteBool(file, focused_show_wsasendmsg);
    WriteBool(file, focused_show_recv);
    WriteBool(file, focused_show_recvfrom);
    WriteBool(file, focused_show_wsarecv);
    WriteBool(file, focused_show_wsarecvfrom);
    WriteBool(file, filter_by_has_recieve);
    WriteBool(file, filter_by_has_send);
    WriteBool(file, filter_by_status_open);
    WriteBool(file, filter_by_status_unknown);
    WriteBool(file, filter_by_status_closed);
    WriteBool(file, filter_by_hidden);
    WriteBool(file, filter_by_is_socket);
    WriteBool(file, filter_by_is_http);
    WriteBool(file, filter_by_has_custom_name);
    WriteBool(file, invert);
    WriteFloat(file, time_factor);
    WriteInt(file, (int)filter_out_log_types);

    WriteBool(file, 0);// end of file indicator!! i think due to the read int func, we need to have extra padding or else it'll determine that this next last value is actually null/EOF and return 0
    file.close();
}

void LoadConfigs() {
    ifstream file;
    file.open("c226configs.bin");
    if (!file.is_open()) return;

    focus_http = ReadBool(file);  
    preview_socket_packets_sent = ReadBool(file);
    preview_socket_bytes_sent = ReadBool(file);
    preview_socket_packets_recieved = ReadBool(file);
    preview_socket_bytes_recieved = ReadBool(file);
    preview_socket_timestamp = ReadBool(file);
    preview_socket_status = ReadBool(file);
    preview_socket_event_count = ReadBool(file);
    preview_socket_label = ReadBool(file); 
    focused_show_global_send = ReadBool(file);
    focused_show_global_recv = ReadBool(file);
    focused_show_send = ReadBool(file);
    focused_show_sendto = ReadBool(file);
    focused_show_wsasend = ReadBool(file);
    focused_show_wsasendto = ReadBool(file);
    focused_show_wsasendmsg = ReadBool(file);
    focused_show_recv = ReadBool(file);
    focused_show_recvfrom = ReadBool(file);
    focused_show_wsarecv = ReadBool(file);
    focused_show_wsarecvfrom = ReadBool(file);
    filter_by_has_recieve = ReadBool(file);
    filter_by_has_send = ReadBool(file);
    filter_by_status_open = ReadBool(file);
    filter_by_status_unknown = ReadBool(file);
    filter_by_status_closed = ReadBool(file);
    filter_by_hidden = ReadBool(file);
    filter_by_is_socket = ReadBool(file);
    filter_by_is_http = ReadBool(file);
    filter_by_has_custom_name = ReadBool(file);
    invert = ReadBool(file);
    IO_UpdateTimeFactor(ReadFloat(file));
    filter_out_log_types = (socket_event_type)ReadInt(file);

    file.close();
}