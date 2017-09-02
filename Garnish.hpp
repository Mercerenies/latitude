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

/// This function adds instructions to the current interpreter state
/// for producing a value equivalent to its argument. It does so by
/// taking the current continuation in `%%cont`, pushing it onto
/// `%%stack`, and then placing a new sequence obtained by an
/// appropriate call to garnishSeq into `%%cont`.
///
/// The parameter type `T` shall be a type for which there is a valid
/// garnishSeq overload.
///
/// \tparam T the parameter type
/// \param state the interpreter state
/// \param value the value to garnish
template <typename T>
void garnishBegin(IntState& state, T value) {
    InstrSeq seq = garnishSeq(value);
    state.stack = pushNode(state.stack, state.cont);
    state.cont = CodeSeek(std::move(seq));
}

#endif // GARNISH_HPP
