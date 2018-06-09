#ifndef BASE_HPP
#define BASE_HPP

#include <exception>
#include <string>

template <typename T>
struct Proxy {};

/// This is the general error class for exceptions in the VM system,
/// usually with the integrity of a VM instruction.
class AssemblerError : public std::exception {
private:
    std::string message;
public:
    /// \brief Constructs an assembler error with a generic message.
    AssemblerError();
    /// \brief Constructs an assembler error with a specific message.
    AssemblerError(std::string message);
    /// \brief Returns the error's message.
    ///
    /// \return the message
    std::string getMessage();
    virtual const char* what();
};

class ParseError : public std::exception {
private:
    std::string message;
public:
    ParseError();
    ParseError(std::string message);
    std::string getMessage();
    virtual const char* what();
};

#endif // BASE_HPP
