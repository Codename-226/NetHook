#pragma once

string LPCWSTRToString(LPCWSTR lpcwszStr, int length = -1){
    if (!lpcwszStr) return string("nil");

    // calc required string length
    int str_length = WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, length, nullptr, 0, nullptr, nullptr);
    
    // Perform the conversion from LPCWSTR to std::string
    string str(str_length, 0);
    WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, -1, &str[0], str_length, nullptr, nullptr);
    return str;
}
std::string BytesToHex(char* bytes, int size) {
    std::stringstream ss;
    for (int i = 0; i < size; i++)
        ss << std::setfill('0') << std::setw(2) << std::hex << (0xff & (unsigned int)bytes[i]) << " ";
    return ss.str();
}
string BytesToStringOrHex(char* bytes, int size) {
    

    bool is_probably_unicode = true;
    for (int i = 1; i < size; i += 2) {
        if (bytes[i] != 0){
            is_probably_unicode = false;
            break;
    }}

    // scan through bytes to look for any bad bytes
    bool is_valid_string = true;
    bool is_probably_corrupt16 = false;
    for (int i = 0; i < size; i += 1 + (int)is_probably_unicode) {
        if ((bytes[i] < 0x20 || bytes[i] >= 0x7F) && bytes[i] != 0x0d && bytes[i] != 0x0a && bytes[i] != 0x09) {
            
            if (i < 16) // NOTE: 'probably unicode' skips odd sequence bytes so we dont check them for corrupt
                is_probably_corrupt16 = true;
            else {
                is_valid_string = false;
                break;
            }
    }}


    // so we now have this system to ski[
    if (is_valid_string) {
        if (is_probably_unicode) {
            if (is_probably_corrupt16) {
                if (size > 16) return string("16_byte_corrupt_") + LPCWSTRToString((LPCWSTR)(bytes + 16), size - 16);
                else return string("");
            } else return LPCWSTRToString((LPCWSTR)bytes, size);
        } else {
            if (is_probably_corrupt16) {
                if (size > 16) return string("16_byte_corrupt_") + string(bytes+16, size-16);
                else return string("");
            }
            else return string(bytes, size);
        }
    }
    
    
    // otherwise we have to literally convert all of it to hex string
    return BytesToHex(bytes, size);
}


unsigned short flip_short(unsigned short input) {
    return (input << 8) | (input >> 8);
}
unsigned int flip_int(unsigned int input) {
    return (input << 24)
         |((input << 8 ) & 0x00ff0000)
         |((input >> 8 ) & 0x0000ff00)
         | (input >> 24);
}
unsigned long long flip_longlong(unsigned long long input) {
    return (input << 56)
         |((input << 40) & 0x00ff000000000000)
         |((input << 24) & 0x0000ff0000000000)
         |((input << 8 ) & 0x000000ff00000000)
         |((input >> 8 ) & 0x00000000ff000000)
         |((input >> 24) & 0x0000000000ff0000)
         |((input >> 40) & 0x000000000000ff00)
         | (input >> 56);
}

uint64_t put_data_into_num(const char* data, int data_length) {
    if (data_length <= 0) return 0;
    if (!data) return 0;

    
    //if (data_length == 1) {
    //    return ((uint64_t)*(unsigned char*)data) << 54;
    //}
    //if (data_length == 2) {
    //    return ((uint64_t)flip_short(*(unsigned short*)data)) << 48;
    //}
    //if (data_length == 3) {
    //    char copy_buffer[4] = { 0 };
    //    memcpy(copy_buffer, data, 3);
    //    return ((uint64_t)flip_int(*(unsigned int*)copy_buffer)) << 32;
    //}
    //if (data_length == 4) {
    //    return ((uint64_t)flip_int(*(unsigned int*)data)) << 32;
    //}
    //if (data_length == 5) {
    //    char copy_buffer[8] = { 0 };
    //    memcpy(copy_buffer, data, 5);
    //    return flip_longlong(*(unsigned long long*)copy_buffer);
    //}
    //if (data_length == 6) {
    //    char copy_buffer[8] = { 0 };
    //    memcpy(copy_buffer, data, 6);
    //    return flip_longlong(*(unsigned long long*)copy_buffer);
    //}
    //if (data_length == 7) {
    //    char copy_buffer[8] = { 0 };
    //    memcpy(copy_buffer, data, 7);
    //    return flip_longlong(*(unsigned long long*)copy_buffer);
    //}
    //if (data_length >= 8) {
    char copy_buffer[8] = { 0 };
    memcpy(copy_buffer, data, data_length > 8 ? 8 : data_length);
    return flip_longlong(*(unsigned long long*)copy_buffer);
    //}
}