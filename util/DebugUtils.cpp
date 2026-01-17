#include "DebugUtils.h"
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>

void DebugToFile(const std::string &message) {
  try {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);

    // Format timestamp
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    // Format the full message
    std::string fullMessage = "[" + ss.str() + "] DEBUG: " + message + "\r\n";

    // Use Windows API for direct file I/O
    HANDLE hFile =
        CreateFileA("C:\\Users\\Public\\custom_quickchat_debug.log",
                    FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
      DWORD bytesWritten;
      WriteFile(hFile, fullMessage.c_str(),
                static_cast<DWORD>(fullMessage.length()), &bytesWritten, NULL);

      // Force flush to disk
      FlushFileBuffers(hFile);

      // Close the file
      CloseHandle(hFile);
    }
  } catch (...) {
    // Can't do much if this fails
  }
}
