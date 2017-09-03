#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>

/// \file
///
/// \brief Functions and accessors for file header information

/// This enum class is used for setting which fields of the Header
/// structure are valid. Some file headers may fail to include one or
/// more of the keys, in which case the corresponding field in the
/// Header structure will contain an undefined value and the matching
/// HeaderField bit will be unset.
enum class HeaderField { MODULE = 1, PACKAGE = 2 };

/// This structure represents a file header. The `fields` variable
/// details which of the fields in this structure are valid and which
/// are unused in the particular file header.
struct Header {
    /// \brief A bit field of HeaderField values describing which
    /// values in the structure are valid.
    unsigned int fields;
    /// \brief The module name, valid if the HeaderField::MODULE bit
    /// is set.
    std::string module;
    /// \brief The package name, valid if the HeaderField::PACKAGE bit
    /// is set.
    std::string package;
};

/// Given a filename, this function reads just enough of the file to
/// access its header and returns a Header instance detailing the
/// information that was acquired.
///
/// \param filename the file name
/// \return the file header
Header getFileHeader(std::string filename);

#endif // HEADER_HPP
