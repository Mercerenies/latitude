#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>

enum class HeaderField { MODULE = 1, PACKAGE = 2 };

struct Header {
    unsigned int fields; // Bits from HeaderFields
    std::string module;
    std::string package;
};

Header getFileHeader(std::string filename);

#endif // HEADER_HPP
