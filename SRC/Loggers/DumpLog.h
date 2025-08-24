#pragma once
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip> // for std::hex and setw
#include <WinUser.h>
#include <shellapi.h>
#include <fstream>
#include <filesystem>   // C++17 for folder and path handling
namespace fs = std::filesystem;


unsigned long long randomULL() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<unsigned long long> dis;

    return dis(gen);
}

// Convert unsigned long long to a hex string
std::string toHexString(unsigned long long value) {
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << value;
    return ss.str();
}

// Generate the file name from a random unsigned long long
std::string generateRandomFileName() {
    return toHexString(randomULL()) + ".txt";
}

// Save a file to a specified directory
const char* dump_location = "./226_output";
void DumpString(const std::string& fileName, const std::string& content) {
    try {
        // Create folder if it doesn't exist
        if (!fs::exists(dump_location))
            fs::create_directories(dump_location);
        
        fs::path fullPath = fs::path(dump_location) / fs::path(fileName);

        std::ofstream ofs(fullPath, std::ios::out);
        if (!ofs) {
            LogEntry("DumpLog Error: Could not open file for writing");
            return; }
        ofs << content;
        ofs.close();


        ShellExecuteA(NULL, "open", "notepad.exe", fullPath.string().c_str(), NULL, SW_SHOWNORMAL);
        LogEntry("DumpLog Success.");
    }
    catch (const std::exception& ex) {
        LogEntry("DumpLog Error: failed to create/save file");
        return;
    }
}


void DumpToNotepad(string text) {
    std::string filename = generateRandomFileName();;

    DumpString(filename, text);

    // Use ShellExecute to open file with notepad.exe
}