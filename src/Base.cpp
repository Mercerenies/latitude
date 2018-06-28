//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details


#include "Base.hpp"

LatitudeError::LatitudeError(std::string message)
    : message(message) {}

std::string LatitudeError::getMessage() {
    return message;
}

const char* LatitudeError::what() {
    return message.c_str();
}

AssemblerError::AssemblerError()
    : AssemblerError("Assembler error") {}

AssemblerError::AssemblerError(std::string message)
    : LatitudeError(message) {}

ParseError::ParseError()
    : ParseError("Parse error!") {}

ParseError::ParseError(std::string message)
    : LatitudeError(message) {}

HeaderError::HeaderError()
    : HeaderError("Header error!") {}

HeaderError::HeaderError(std::string message)
    : LatitudeError(message) {}
