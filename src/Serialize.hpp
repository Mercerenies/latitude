#ifndef SERIALIZE_HPP
#define SERIALIZE_HPP

#include "Instructions.hpp"

/// Appends an argument to a serialized instruction sequence, as a
/// sequence of characters.
///
/// \param arg the instruction argument
/// \param seq the serialized instruction sequence
void appendRegisterArg(const RegisterArg& arg, SerialInstrSeq& seq);

/// Appends an opcode to a serialized instruction sequence, as a
/// sequence of characters.
///
/// \param instr the instruction
/// \param seq the serialized instruction sequence
void appendInstruction(const Instr& instr, SerialInstrSeq& seq);

/// Given a serial instruction sequence, pops a single character off
/// the front and returns it.
///
/// \param state a serial instruction sequence
/// \return the first character, or 0 if there is no character
unsigned char popChar(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops a long integer off the
/// front and returns it.
///
/// \param state a serial instruction sequence
/// \return a long integer
long popLong(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops a string off the front
/// and returns it. Note that these strings are 8-bit clean and can
/// contain nulls.
///
/// \param state a serial instruction sequence
/// \return a string
std::string popString(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops a single character off
/// the front and reads it as a register reference.
///
/// \param state a serial instruction sequence
/// \return a register
Reg popReg(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops a single character off
/// the front and reads it as an instruction.
///
/// \param state a serial instruction sequence
/// \return an instruction
Instr popInstr(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops off enough bytes to
/// parse as a function reference.
///
/// \param state a serial instruction sequence
/// \return a function index
FunctionIndex popFunction(SerialInstrSeq& state);

#endif // SERIALIZE_HPP
