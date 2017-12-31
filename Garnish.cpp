#include "Garnish.hpp"
#include "Reader.hpp"
#include "Macro.hpp"
#include "Assembler.hpp"
#include <sstream>
#include <type_traits>

using namespace std;

InstrSeq garnishSeq(bool value) {
    InstrSeq seq;
    if (value) {
        seq = asmCode(makeAssemblerLine(Instr::YLD, Lit::TRUE, Reg::RET));
    } else {
        seq = asmCode(makeAssemblerLine(Instr::YLD, Lit::FALSE, Reg::RET));
    }
    return seq;
}

InstrSeq garnishSeq(boost::blank value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::YLD, Lit::NIL, Reg::RET));
    return seq;
}

InstrSeq garnishSeq(std::string value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::YLDC, Lit::STRING, Reg::PTR),
                           makeAssemblerLine(Instr::STR, value),
                           makeAssemblerLine(Instr::LOAD, Reg::STR0),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET));
    return seq;
}

InstrSeq garnishSeq(int value) {
    return garnishSeq((long)value);
}

InstrSeq garnishSeq(long value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::PTR),
                           makeAssemblerLine(Instr::INT, value),
                           makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET));
    return seq;
}

InstrSeq garnishSeq(Symbolic value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::YLDC, Lit::SYMBOL, Reg::PTR),
                           makeAssemblerLine(Instr::SYMN, value.index),
                           makeAssemblerLine(Instr::LOAD, Reg::SYM),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET));
    return seq;
}

ObjectPtr garnishObject(const ReadOnlyState& reader, bool value) {
    return value ? reader.lit.at(Lit::TRUE) : reader.lit.at(Lit::FALSE);
}

ObjectPtr garnishObject(const ReadOnlyState& reader, boost::blank value) {
    return reader.lit.at(Lit::NIL);
}
