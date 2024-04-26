
#ifndef Helper_H
#define Helper_H

#include <chrono>

#define FILENAME_LENGTH 80

void getFilenameStr(const std::chrono::system_clock::time_point now, char* filenameStr, const int filenameLength);

#endif
