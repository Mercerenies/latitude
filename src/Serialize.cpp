
#include "Serialize.hpp"
#include <string>
#include <iterator>
#include <type_traits>

template <typename Container>
class PopIterator : public std::iterator<std::input_iterator_tag, typename std::remove_reference<decltype(std::declval<Container>().front())>::type> {
private:
    Container& container;
public:
    PopIterator(Container& container) : container(container) {}
    bool operator==(const PopIterator<Container>& other) { return container == other.container; }
    bool operator!=(const PopIterator<Container>& other) { return !(*this == other); }
    typename PopIterator::reference operator*() { return container.front(); }
    typename PopIterator::pointer operator->() { return &container.front(); }
    PopIterator<Container> operator++() { container.pop_front(); return *this; }
    // Note: This "iterator" does not support postfix ++ and is
    // therefore technically not an iterator. It's a temporary patch
    // for the process of transition and will be removed soon.
};

template <typename Container>
PopIterator<Container> popIterator(Container& container) {
    return { container };
}

struct AppendVisitor {
    SerialInstrSeq* instructions;

    template <typename T>
    void operator()(const T& val) {
        auto temp = std::back_inserter(*instructions);
        serialize_t<T>().serialize(val, temp);
    }

};

void appendRegisterArg(const RegisterArg& arg, SerialInstrSeq& seq) {
    AppendVisitor visitor { &seq };
    boost::apply_visitor(visitor, arg);
}

void appendInstruction(const Instr& instr, SerialInstrSeq& seq) {
    AppendVisitor visitor { &seq };
    visitor(instr);
}

unsigned char popChar(SerialInstrSeq& state) {
    auto temp = popIterator(state);
    return (unsigned char)serialize_t<char>().deserialize(temp);
}

long popLong(SerialInstrSeq& state) {
    auto temp = popIterator(state);
    return serialize_t<long>().deserialize(temp);
}

std::string popString(SerialInstrSeq& state) {
    auto temp = popIterator(state);
    return serialize_t<std::string>().deserialize(temp);
}

Reg popReg(SerialInstrSeq& state) {
    auto temp = popIterator(state);
    return serialize_t<Reg>().deserialize(temp);
}

Instr popInstr(SerialInstrSeq& state) {
    auto temp = popIterator(state);
    return serialize_t<Instr>().deserialize(temp);
}

FunctionIndex popFunction(SerialInstrSeq& state) {
    auto temp = popIterator(state);
    return serialize_t<FunctionIndex>().deserialize(temp);
}
