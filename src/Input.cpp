
#include "Input.hpp"
#include "Platform.hpp"
#include <iostream>

namespace ReadLine {

    std::string readBasic() {
        std::string result;
        getline(std::cin, result);
        return result;
    }

#ifdef USE_POSIX

    bool isTermSmart() {
        return false;
    }

    std::string readRich() {
        return readBasic();
    }

#endif

#ifdef USE_WINDOWS

    bool isTermSmart() {
        return false;
    }

    std::string readRich() {
        return readBasic();
    }

#endif

#ifdef USE_NULL

    bool isTermSmart() {
        return false;
    }

    std::string readRich() {
        return readBasic();
    }

#endif

}
