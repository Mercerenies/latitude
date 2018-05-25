#ifndef GARNISH_HPP
#define GARNISH_HPP

#include <string>
#include <boost/blank.hpp>
#include "Proto.hpp"
#include "Bytecode.hpp"

/// \file
///
/// \brief Functions for garnishing C++ values.
///
/// "Garnishing", here, is defined as taking a C++ value and
/// converting it to an appropriate Latitude object. This can be done
/// for many of the basic C++ types, including strings, integers, and
/// Booleans.

/// Returns an instruction sequence which places a Boolean value
/// equivalent to the argument in the `%%ret` register.
///
/// \param value the value to garnish
/// \return a sequence of instructions
InstrSeq garnishSeq(bool value);

/// Returns an instruction sequence which places the literal value
/// `Nil` equivalent to the argument in the `%%ret` register.
///
/// \param value the singular `boost::blank` value
/// \return a sequence of instructions
InstrSeq garnishSeq(boost::blank value);

/// Returns an instruction sequence which places a string value
/// equivalent to the argument in the `%%ret` register.
///
/// \param value the value to garnish
/// \return a sequence of instructions
InstrSeq garnishSeq(std::string value);

/// Returns an instruction sequence which places an integer value
/// equivalent to the argument in the `%%ret` register.
///
/// \param value the value to garnish
/// \return a sequence of instructions
InstrSeq garnishSeq(int value);

/// Returns an instruction sequence which places a long integer value
/// equivalent to the argument in the `%%ret` register.
///
/// \param value the value to garnish
/// \return a sequence of instructions
InstrSeq garnishSeq(long value);

/// Returns an instruction sequence which places a symbol value
/// equivalent to the argument in the `%%ret` register.
///
/// \param value the value to garnish
/// \return a sequence of instructions
InstrSeq garnishSeq(Symbolic value);

/// Returns the Latitude Boolean value True or False, depending on the
/// value of the argument.
///
/// \param reader the read-only state
/// \param value the Boolean value
/// \return either True or False, the Latitude objects
ObjectPtr garnishObject(const ReadOnlyState& reader, bool value);

/// Returns the Latitude special value Nil.
///
/// \param reader the read-only state
/// \param value a placeholder for the "empty" object
/// \return the Nil object
ObjectPtr garnishObject(const ReadOnlyState& reader, boost::blank value);

/// Allocates and returns a new Latitude string object representing
/// the given string.
///
/// \param reader the read-only state
/// \param value the string value
/// \return a Latitude string object
ObjectPtr garnishObject(const ReadOnlyState& reader, std::string value);

/// Allocates and returns a new Latitude symbol object representing
/// the given symbol.
///
/// \param reader the read-only state
/// \param value the symbol value
/// \return a Latitude symbol object
ObjectPtr garnishObject(const ReadOnlyState& reader, Symbolic value);

/// Allocates and returns a new Latitude numerical object representing
/// the given integer.
///
/// \param reader the read-only state
/// \param value the integer value
/// \return a Latitude number object
ObjectPtr garnishObject(const ReadOnlyState& reader, int value);

/// Allocates and returns a new Latitude numerical object representing
/// the given integer.
///
/// \param reader the read-only state
/// \param value the integer value
/// \return a Latitude number object
ObjectPtr garnishObject(const ReadOnlyState& reader, long value);

/// Allocates and returns a new Latitude numerical object representing
/// the given number.
///
/// \param reader the read-only state
/// \param value the number object
/// \return a Latitude number object
ObjectPtr garnishObject(const ReadOnlyState& reader, Number value);

#endif // GARNISH_HPP
