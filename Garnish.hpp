#ifndef _GARNISH_HPP_
#define _GARNISH_HPP_

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
void garnishEnd(IntState& state, T value) {
    InstrSeq seq = garnishSeq(value);
    state.cont.insert(state.cont.end(), seq.begin(), seq.end());
}

template <typename T>
void garnishBegin(IntState& state, T value) {
    InstrSeq seq = garnishSeq(value);
    state.cont.insert(state.cont.begin(), seq.begin(), seq.end());
}

#endif // _GARNISH_HPP_
