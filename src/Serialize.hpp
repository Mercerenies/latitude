#ifndef SERIALIZE_HPP
#define SERIALIZE_HPP

#include "Instructions.hpp"
#include "Assembler.hpp"

/// \file
///
/// \brief Serialization structures and helpers.

/// serialize_t, on its own, is an incomplete type. Any type which is
/// serializable should implement a partial specialization of
/// serialize_t for its particular type. The specialization should
/// define, publicly, a typedef and two methods.
///
/// * `using type = ...` should be defined to be the type parameter
///   `T`.
///
/// * `void serialize(const type&, OutputIterator&)` should, for any
///   output iterator type, serialize a value of the type to the
///   iterator.
///
/// * `type deserialize(InputIterator&)` should, for any input
///   iterator type, deserialize a value of the type to out of the
///   iterator.
///
/// Serializers are allowed to assume the iterators are valid and, in
/// the input case, that the iterator contains a value of the
/// appropriate type. Additionally, if the type is small (such as an
/// arithmetic type), serializers may replace `const type&` with
/// simply `type`.
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

template <>
struct serialize_t<AssemblerLine> {

    using type = AssemblerLine;

    template <typename OutputIterator>
    void serialize(const type& arg, OutputIterator& iter) const;
    template <typename InputIterator>
    type deserialize(InputIterator& iter) const;

};

/// Serializes the argument into the output iterator.
///
/// \pre A partial specialization `serialize_t<T>` must be in scope
/// \param arg the value to serialize
/// \param iter an output iterator
template <typename T, typename OutputIterator>
void serialize(const T& arg, OutputIterator& iter);

/// Deserializes a value of the given type from the iterator.
///
/// \pre A partial specialization `serialize_t<T>` must be in scope
/// \param iter an input iterator
/// \return the value
template <typename T, typename InputIterator>
T deserialize(InputIterator& iter);

/// Serializes the value within the variant structure. The
/// serialization does not mark which variant was saved, so the value
/// must be deserialized with ::deserialize<T> (for the appropriate type
/// `T`).
///
/// \pre A partial specialization `serialize_t<T>` must be in scope,
///      for each `T` in `Ts...`
/// \param arg the variant
/// \param iter an output iterator
template <typename OutputIterator, typename... Ts>
void serializeVariant(const boost::variant<Ts...>& arg, OutputIterator& iter);

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

template <typename OutputIterator>
auto serialize_t<AssemblerLine>::serialize(const type& instr, OutputIterator& iter) const -> void {
    ::serialize<Instr>(instr.getCommand(), iter);
    for (const auto& arg : instr.arguments()) {
        serializeVariant(arg, iter);
    }
}

/// \cond

template <typename InputIterator>
struct _AsmArgDeserializeVisitor {
    InputIterator& iter;

    template <typename T>
    RegisterArg operator()(Proxy<T>) {
        return deserialize<T>(iter);
    }

};

/// \endcond

template <typename InputIterator>
auto serialize_t<AssemblerLine>::deserialize(InputIterator& iter) const -> type {
    Instr instr = ::deserialize<Instr>(iter);
    AssemblerLine instruction { instr };
    _AsmArgDeserializeVisitor<InputIterator> visitor { iter };
    for (const auto& arg : getAsmArguments(instr)) {
        instruction.addRegisterArg(callOnAsmArgType(visitor, arg));
    }
    return instruction;
}

template <typename T, typename OutputIterator>
void serialize(const T& arg, OutputIterator& iter) {
    serialize_t<T>().serialize(arg, iter);
}

template <typename T, typename InputIterator>
T deserialize(InputIterator& iter) {
    return serialize_t<T>().deserialize(iter);
}

/// \cond

template <typename OutputIterator>
struct _VariantSerializeVisitor : boost::static_visitor<void> {
    OutputIterator& iter;

    _VariantSerializeVisitor(OutputIterator& iter) : iter(iter) {}

    template <typename T>
    void operator()(const T& arg) {
        serialize(arg, iter);
    }

};

/// \endcond

template <typename OutputIterator, typename... Ts>
void serializeVariant(const boost::variant<Ts...>& arg, OutputIterator& iter) {
    _VariantSerializeVisitor<OutputIterator> visitor { iter };
    boost::apply_visitor(visitor, arg);
}

#endif // SERIALIZE_HPP
