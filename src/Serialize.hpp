#ifndef SERIALIZE_HPP
#define SERIALIZE_HPP

#include "Instructions.hpp"

template <typename T>
struct serialize_t;

template <>
struct serialize_t<long> {

    using type = long;

    template <typename OutputIterator>
    void serialize(type arg, OutputIterator& iter) const;
    template <typename InputIterator>
    type deserialize(InputIterator& iter) const;

};

template <>
struct serialize_t<char> {

    using type = char;

    template <typename OutputIterator>
    void serialize(type arg, OutputIterator& iter) const;
    template <typename InputIterator>
    type deserialize(InputIterator& iter) const;

};

template <>
struct serialize_t<Reg> {

    using type = Reg;

    template <typename OutputIterator>
    void serialize(type arg, OutputIterator& iter) const;
    template <typename InputIterator>
    type deserialize(InputIterator& iter) const;

};

template <>
struct serialize_t<Instr> {

    using type = Instr;

    template <typename OutputIterator>
    void serialize(type arg, OutputIterator& iter) const;
    template <typename InputIterator>
    type deserialize(InputIterator& iter) const;

};

template <>
struct serialize_t<std::string> {

    using type = std::string;

    template <typename OutputIterator>
    void serialize(const type& arg, OutputIterator& iter) const;
    template <typename InputIterator>
    type deserialize(InputIterator& iter) const;

};

template <>
struct serialize_t<FunctionIndex> {

    using type = FunctionIndex;

    template <typename OutputIterator>
    void serialize(const type& arg, OutputIterator& iter) const;
    template <typename InputIterator>
    type deserialize(InputIterator& iter) const;

};

/// Appends an argument to a serialized instruction sequence, as a
/// sequence of characters.
///
/// \param arg the instruction argument
/// \param seq the serialized instruction sequence
[[deprecated]]
void appendRegisterArg(const RegisterArg& arg, SerialInstrSeq& seq);

/// Appends an opcode to a serialized instruction sequence, as a
/// sequence of characters.
///
/// \param instr the instruction
/// \param seq the serialized instruction sequence
[[deprecated]]
void appendInstruction(const Instr& instr, SerialInstrSeq& seq);

/// Given a serial instruction sequence, pops a single character off
/// the front and returns it.
///
/// \param state a serial instruction sequence
/// \return the first character, or 0 if there is no character
[[deprecated]]
unsigned char popChar(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops a long integer off the
/// front and returns it.
///
/// \param state a serial instruction sequence
/// \return a long integer
[[deprecated]]
long popLong(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops a string off the front
/// and returns it. Note that these strings are 8-bit clean and can
/// contain nulls.
///
/// \param state a serial instruction sequence
/// \return a string
[[deprecated]]
std::string popString(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops a single character off
/// the front and reads it as a register reference.
///
/// \param state a serial instruction sequence
/// \return a register
[[deprecated]]
Reg popReg(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops a single character off
/// the front and reads it as an instruction.
///
/// \param state a serial instruction sequence
/// \return an instruction
[[deprecated]]
Instr popInstr(SerialInstrSeq& state);

/// Given a serial instruction sequence, pops off enough bytes to
/// parse as a function reference.
///
/// \param state a serial instruction sequence
/// \return a function index
[[deprecated]]
FunctionIndex popFunction(SerialInstrSeq& state);

// ----

template <typename OutputIterator>
auto serialize_t<long>::serialize(type arg, OutputIterator& iter) const -> void {
    long val1 = arg;
    if (val1 < 0)
        *iter++ = 0xFF;
    else
        *iter++ = 0x00;
    val1 = abs(val1);
    for (int i = 0; i < 4; i++) {
        *iter++ = (unsigned char)(val1 % 256);
        val1 /= 256;
    }
}

template <typename InputIterator>
auto serialize_t<long>::deserialize(InputIterator& iter) const -> type {
    int sign = 1;
    if (*iter > 0)
        sign *= -1;
    ++iter;
    long value = 0;
    long pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)(*iter);
        ++iter;
        pow <<= 8;
    }
    return sign * value;
}

template <typename OutputIterator>
auto serialize_t<char>::serialize(type arg, OutputIterator& iter) const -> void {
    *iter++ = (unsigned char)arg;
}

template <typename InputIterator>
auto serialize_t<char>::deserialize(InputIterator& iter) const -> type {
    char ch = *iter;
    ++iter;
    return ch;
}

template <typename OutputIterator>
auto serialize_t<Reg>::serialize(type arg, OutputIterator& iter) const -> void {
    *iter++ = (unsigned char)arg;
}

template <typename InputIterator>
auto serialize_t<Reg>::deserialize(InputIterator& iter) const -> type {
    unsigned char ch = *iter;
    ++iter;
    return (Reg)ch;
}

template <typename OutputIterator>
auto serialize_t<Instr>::serialize(type arg, OutputIterator& iter) const -> void {
    *iter++ = (unsigned char)arg;
}

template <typename InputIterator>
auto serialize_t<Instr>::deserialize(InputIterator& iter) const -> type {
    unsigned char ch = *iter;
    ++iter;
    return (Instr)ch;
}

template <typename OutputIterator>
auto serialize_t<std::string>::serialize(const type& arg, OutputIterator& iter) const -> void {
    for (char ch : arg) {
        if (ch == 0) {
            *iter++ = '\0';
            *iter++ = '.';
        } else {
            *iter++ = ch;
        }
    }
    *iter++ = '\0';
    *iter++ = '\0';
}

template <typename InputIterator>
auto serialize_t<std::string>::deserialize(InputIterator& iter) const -> type {
    std::string str;
    unsigned char ch;
    while (true) {
        ch = *iter;
        ++iter;
        if (ch == '\0') {
            ch = *iter;
            ++iter;
            if (ch == '.')
                str += '\0';
            else if (ch == '\0')
                break;
        } else {
            str += ch;
        }
    }
    return str;
}

template <typename OutputIterator>
auto serialize_t<FunctionIndex>::serialize(const type& arg, OutputIterator& iter) const -> void {
    // No need for a sign bit; this is an index so it's always nonnegative
    int val1 = arg.index;
    for (int i = 0; i < 4; i++) {
        *iter++ = (unsigned char)(val1 % 256);
        val1 /= 256;
    }
}

template <typename InputIterator>
auto serialize_t<FunctionIndex>::deserialize(InputIterator& iter) const -> type {
    int value = 0;
    int pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)(*iter);
        ++iter;
        pow <<= 8;
    }
    return { value };
}

#endif // SERIALIZE_HPP
