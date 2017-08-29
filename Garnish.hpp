#ifndef GARNISH_HPP
#define GARNISH_HPP

#include <string>
#include <boost/blank.hpp>
#include "Proto.hpp"
#include "Bytecode.hpp"

InstrSeq garnishSeq(bool value);
InstrSeq garnishSeq(boost::blank value);
InstrSeq garnishSeq(std::string value);
InstrSeq garnishSeq(int value);
InstrSeq garnishSeq(long value);
InstrSeq garnishSeq(Symbolic value);

template <typename T>
void garnishBegin(IntState& state, T value) {
    InstrSeq seq = garnishSeq(value);
    state.stack = pushNode(state.stack, state.cont);
    state.cont = CodeSeek(std::move(seq));
}

#endif // GARNISH_HPP
