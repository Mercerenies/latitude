//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef UNICODE_HPP
#define UNICODE_HPP

#include "pl_Unidata.h"
#include <string>
#include <boost/optional.hpp>

/// \file
///
/// \brief Some convenience functions to make Unicode-handling easier.

/// An instance of this class represents a single Unicode code point.
class UniChar {
private:
    long codepoint;
public:

    /// Constructs a UniChar instance from an explicit code point.
    ///
    /// \param cp the code point
    explicit UniChar(long cp);

    /// Returns a string which, when interpreted as UTF-8, represents
    /// the code point.
    ///
    /// \return the string
    operator std::string() const;

    /// Returns the code point.
    ///
    /// \return the code point
    long codePoint() const noexcept;

    /// Returns the Unicode general category of the character
    /// associated with this code point.
    ///
    /// \return the category
    uni_class_t genCat() const;

    /// Returns a new UniChar containing the uppercase equivalent of
    /// the current code point. The current code point is returned if
    /// the case does not make sense.
    ///
    /// \return the uppercase code point
    UniChar toUpper() const;

    /// Returns a new UniChar containing the lowercase equivalent of
    /// the current code point. The current code point is returned if
    /// the case does not make sense.
    ///
    /// \return the lowercase code point
    UniChar toLower() const;

    /// Returns a new UniChar containing the title case equivalent of
    /// the current code point. The current code point is returned if
    /// the case does not make sense.
    ///
    /// \return the title case code point
    UniChar toTitle() const;

};

long uniOrd(UniChar ch);
UniChar uniChr(long cp);

/// Returns the Unicode code point beginning at the nth byte in the
/// string. If the index is out of bounds or not at a character
/// boundary, returns an empty optional instance.
///
/// \param str the string
/// \param i the index
/// \return the character, or an empty optional if out of bounds
boost::optional<UniChar> charAt(std::string str, long i);

/// Returns the index of the next Unicode character after the
/// character which starts at the nth byte. If the index is out of
/// bounds, not at a character boundary, or at the position of the
/// final character, this function returns an empty optional instance.
///
/// \param str the string
/// \param i the index
/// \return the next index, or an empty optional
boost::optional<long> nextCharPos(std::string str, long i);

#endif // UNICODE_HPP
