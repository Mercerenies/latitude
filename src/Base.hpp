#ifndef BASE_HPP
#define BASE_HPP

#include <exception>
#include <string>

template <typename T>
struct Proxy {};

class LatitudeError : public std::exception {
private:
    std::string message;
public:
    /// \brief Constructs a Latitude error with a specific message.
    LatitudeError(std::string message);
    /// \brief Returns the error's message
    ///
    /// \return the message
    std::string getMessage();
    virtual const char* what();
};

/// This is the general error class for exceptions in the VM system,
/// usually with the integrity of a VM instruction.
class AssemblerError : public LatitudeError {
public:
    AssemblerError();
    AssemblerError(std::string message);
};

/// A ParseError occurs when there is a problem in parsing a source
/// file. If an error occurs while parsing a compiled file, use
/// AssemblerError. If an error occurs while parsing the header of a
/// file, use HeaderError.
class ParseError : public LatitudeError {
public:
    ParseError();
    ParseError(std::string message);
};

/// A HeaderError occurs when trying to read the Header from a file
/// (either source or compiled).
class HeaderError : public LatitudeError {
public:
    HeaderError();
    HeaderError(std::string message);
};

#endif // BASE_HPP
