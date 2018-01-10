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

ObjectPtr garnishObject(const ReadOnlyState& reader, bool value);

ObjectPtr garnishObject(const ReadOnlyState& reader, boost::blank value);

ObjectPtr garnishObject(const ReadOnlyState& reader, std::string value);

ObjectPtr garnishObject(const ReadOnlyState& reader, Symbolic value);

ObjectPtr garnishObject(const ReadOnlyState& reader, int value);

ObjectPtr garnishObject(const ReadOnlyState& reader, long value);

ObjectPtr garnishObject(const ReadOnlyState& reader, Number value);

#endif // GARNISH_HPP
