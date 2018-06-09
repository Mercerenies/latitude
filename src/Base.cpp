
#include "Base.hpp"

AssemblerError::AssemblerError()
    : AssemblerError("Assembler error") {}

AssemblerError::AssemblerError(std::string message)
    : message(message) {}

std::string AssemblerError::getMessage() {
    return message;
}

const char* AssemblerError::what() {
    return message.c_str();
}

ParseError::ParseError()
    : ParseError("Parse error!") {}

ParseError::ParseError(std::string message)
    : message(message) {}

std::string ParseError::getMessage() {
    return message;
}

const char* ParseError::what() {
    return message.c_str();
}
