
#include "Environment.hpp"
#include "Platform.hpp"
#include <string>
#include <cstdlib>
#include <boost/optional.hpp>

using namespace std;

boost::optional<string> getEnv(string name) {
    char* env = getenv(name.c_str());
    if (env) {
        return string(env);
    } else {
        return boost::none;
    }
}

#ifdef USE_POSIX
bool setEnv(string name, string value) {
    setenv(name.c_str(), value.c_str(), 1);
    return true;
}
bool unsetEnv(string name) {
    unsetenv(name.c_str());
    return true;
}
#endif

#ifdef USE_WINDOWS
#include <windows.h>
bool setEnv(string name, string value) {
    SetEnvironmentVariable(name.c_str(), value.c_str());
    return true;
}
bool unsetEnv(string name) {
    SetEnvironmentVariable(name.c_str(), NULL);
    return true;
}
#endif

#ifdef USE_NULL
bool setEnv(string name, string value) {
    return false;
}
bool unsetEnv(string name) {
    return false;
}
#endif
