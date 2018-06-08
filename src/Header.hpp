#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>
#include "Serialize.hpp"

/// \file
///
/// \brief Functions and accessors for file header information

constexpr int FILE_VERSION = 1000;

/// This enum class is used for setting which fields of the Header
/// structure are valid. Some file headers may fail to include one or
/// more of the keys, in which case the corresponding field in the
/// Header structure will contain an undefined value and the matching
/// HeaderField bit will be unset.
enum class HeaderField { MODULE = 1, PACKAGE = 2, VERSION = 4 };

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
    /// \brief The compiled "version" of the file, valid if the
    /// HeaderField::VERSION bit is set
    int version;
};

/// Given a filename, this function reads just enough of the file to
/// access its header and returns a Header instance detailing the
/// information that was acquired.
///
/// \param filename the file name
/// \return the file header
Header getFileHeaderSource(std::string filename);

Header getFileHeaderComp(std::ifstream& file);

void saveFileHeader(std::ofstream& file, const Header& header);

// ----

template <>
struct serialize_t<Header> {

    using type = Header;

    template <typename OutputIterator>
    void serialize(const type& arg, OutputIterator& iter) const;
    template <typename InputIterator>
    type deserialize(InputIterator& iter) const;

};

/*
 * Fields:
 * .. end of header
 * #  signed long
 * $  std::string
 * (Note: The final . of the end of header is left in the stream)
 */

template <typename OutputIterator>
auto serialize_t<Header>::serialize(const type& arg, OutputIterator& iter) const -> void {
    using ::serialize;

    serialize<long>(FILE_VERSION, iter);

    if (arg.fields & (unsigned int)HeaderField::MODULE) {
        serialize('M', iter);
        serialize('$', iter);
        serialize(arg.module, iter);
    }

    if (arg.fields & (unsigned int)HeaderField::PACKAGE) {
        serialize('P', iter);
        serialize('$', iter);
        serialize(arg.package, iter);
    }

    serialize('.', iter);

}

template <typename InputIterator>
auto serialize_t<Header>::deserialize(InputIterator& iter) const -> type{
    using ::deserialize;

    Header header;
    header.fields = 0;

    header.version = deserialize<long>(iter);
    // TODO Err if the version is not valid
    header.fields &= (unsigned int)HeaderField::VERSION;

    char field = deserialize<char>(iter);
    while (field != '.') {
        char valtype = deserialize<char>(iter);
        switch (field) {
        case 'M':
            // Assume it's a string
            header.module = deserialize<std::string>(iter);
            header.fields &= (unsigned int)HeaderField::MODULE;
            break;
        case 'P':
            // Assume it's a string
            header.package = deserialize<std::string>(iter);
            header.fields &= (unsigned int)HeaderField::PACKAGE;
            break;
        default:
            // Unknown so ignore it
            switch (valtype) {
            case '#':
                deserialize<long>(iter);
                break;
            case '$':
                deserialize<std::string>(iter);
                break;
            }
            break;
        }
        field = deserialize<char>(iter);
    }

    return header;

}

#endif // HEADER_HPP
