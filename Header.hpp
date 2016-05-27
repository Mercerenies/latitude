#ifndef _HEADER_HPP_
#define _HEADER_HPP_

#include <string>

enum class HeaderField { MODULE = 1, PACKAGE = 2 };

struct Header {
    unsigned int fields; // Bits from HeaderFields
    std::string module;
    std::string package;
};

Header getFileHeader(std::string filename);

#endif // _HEADER_HPP_
