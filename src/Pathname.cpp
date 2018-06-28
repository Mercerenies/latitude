//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "Pathname.hpp"
#include "Platform.hpp"
#include <string>
#include <cstring>

using namespace std;

#ifdef USE_POSIX
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

std::string getExecutablePathname() {
    constexpr int SIZE = 512;
    char buffer[SIZE];
    memset(buffer, 0, SIZE * sizeof(char));
    if (readlink("/proc/self/exe", buffer, SIZE) == -1)
        return "";
    std::string result(buffer);
    return result;
}

std::string getWorkingDirectory() {
    constexpr int SIZE = 512;
    char buffer[SIZE];
    memset(buffer, 0, SIZE * sizeof(char));
    if (getcwd(buffer, SIZE) == NULL)
        return "";
    std::string result(buffer);
    return result;
}

std::string stripFilename(std::string path) {
    constexpr int SIZE = 512;
    char buffer[SIZE];
    if (path.size() >= SIZE - 1)
        return "";
    strcpy(buffer, path.c_str());
    string result = dirname(buffer);
    return result + "/";
}

std::string stripDirname(std::string path) {
    constexpr int SIZE = 512;
    char buffer[SIZE];
    if (path.size() >= SIZE - 1)
        return "";
    strcpy(buffer, path.c_str());
    string result = basename(buffer);
    return result;
}

std::time_t modificationTime(std::string path) {
    struct stat result;
    if (stat(path.c_str(), &result) == 0) {
        return result.st_mtime;
    } else {
        return time_t {};
    }
}
#endif

#ifdef USE_WINDOWS
#define WINVER 0x0500
#include <windows.h>
#include <stdlib.h>
#include <algorithm>
#include <sys/stat.h>

std::string getExecutablePathname() {
    constexpr int SIZE = 512;
    char buffer[SIZE];
    GetModuleFileName(NULL, buffer, SIZE);
    std::string result(buffer);
    std::replace(result.begin(), result.end(), '\\', '/'); // Use forward slashes in pathnames, for consistency
    return result;
}

std::string getWorkingDirectory() {
    constexpr int SIZE = 512;
    char buffer[SIZE];
    GetCurrentDirectory(SIZE, buffer);
    std::string result(buffer);
    std::replace(result.begin(), result.end(), '\\', '/'); // Use forward slashes in pathnames, for consistency
    return result;
}

std::string stripFilename(std::string path) {
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    _splitpath(path.c_str(), drive, dir, NULL, NULL);
    string result;
    result += string(drive) + string(dir);
    return result;
}

std::string stripDirname(std::string path) {
    char file[_MAX_FNAME];
    char ext[_MAX_EXT];
    _splitpath(path.c_str(), NULL, NULL, file, ext);
    string result;
    result += string(file) + string(ext);
    return result;
}

std::time_t modificationTime(std::string path) {
    struct _stat result;
    if (_stat(path.c_str(), &result) == 0) {
        return result.st_mtime;
    } else {
        return time_t {};
    }
}
#endif

#ifdef USE_NULL
#error Unsupported operating system; Please implement Pathname.cpp for your system
#endif

bool fileExists(std::string fname) {
    if (FILE* file = fopen(fname.c_str(), "rb")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}
