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
// TODO Is this a good idea? It doesn't put a RET instruction or have a closure.
//InstrSeq garnishSeq(const InstrSeq& value);

/*
template <typename T>
void garnishEnd(IntState& state, T value) {
    InstrSeq seq = garnishSeq(value);
    state.stack = pushNode(state.stack, state.cont);
    state.cont = CodeSeek(seq);
}
*/

template <typename T>
void garnishBegin(IntState& state, T value) {
    InstrSeq seq = garnishSeq(value);
    state.stack = pushNode(state.stack, state.cont);
    state.cont = CodeSeek(std::move(seq));
}

#endif // _GARNISH_HPP_
