#ifndef BASE_HPP
#define BASE_HPP

#include <exception>
#include <string>

/// \file
///
/// \brief A catch-all for general Latitude types with no dependencies.

/// The Proxy<T> structure is an empty struct with a single phantom
/// type argument. It is useful in template type deduction, when a
/// type `T` is needed in a return value but not an argument,
/// especially when a variant is being visited.
///
/// \tparam T a phantom type argument
template <typename T>
struct Proxy {};

/// The base class of all Latitude exceptions.
class LatitudeError : public std::exception {
private:
    std::string message;
public:
    /// \brief Constructs a Latitude error with a specific message.
    ///
    /// \param message the message
    LatitudeError(std::string message);
    /// \brief Returns the error's message
    ///
    /// \return the message
    std::string getMessage();
    virtual const char* what();
};

/// \brief Errors in the VM system or the integrity of an instruction.
///
/// This is the general error class for exceptions in the VM system,
/// usually with the integrity of a VM instruction.
class AssemblerError : public LatitudeError {
public:
    AssemblerError();
    AssemblerError(std::string message);
};

/// \brief Errors in parsing a source file into an AST.
///
/// A ParseError occurs when there is a problem in parsing a source
/// file. If an error occurs while parsing a compiled file, use
/// AssemblerError. If an error occurs while parsing the header of a
/// file, use HeaderError.
class ParseError : public LatitudeError {
public:
    ParseError();
    ParseError(std::string message);
};

/// \brief Errors in parsing a header from a source or compiled file.
///
/// A HeaderError occurs when trying to read the Header from a file
/// (either source or compiled).
class HeaderError : public LatitudeError {
public:
    HeaderError();
    HeaderError(std::string message);
};

#endif // BASE_HPP
