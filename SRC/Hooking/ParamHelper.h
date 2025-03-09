#pragma once


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