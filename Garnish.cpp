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
        seq = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                      makeAssemblerLine(Instr::SYMN, Symbols::get()["meta"].index),
                      makeAssemblerLine(Instr::RTRV),
                      makeAssemblerLine(Instr::SYMN, Symbols::get()["True"].index),
                      makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                      makeAssemblerLine(Instr::RTRV));
    } else {
        seq = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                      makeAssemblerLine(Instr::SYMN, Symbols::get()["meta"].index),
                      makeAssemblerLine(Instr::RTRV),
                      makeAssemblerLine(Instr::SYMN, Symbols::get()["False"].index),
                      makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                      makeAssemblerLine(Instr::RTRV));
    }
    return seq;
}

InstrSeq garnishSeq(boost::blank value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["meta"].index),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["Nil"].index),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV));
    return seq;
}

InstrSeq garnishSeq(std::string value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["meta"].index),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["String"].index),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::CLONE),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                           makeAssemblerLine(Instr::STR, value),
                           makeAssemblerLine(Instr::LOAD, Reg::STR0),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET));
    return seq;
}

InstrSeq garnishSeq(int value) {
    return garnishSeq((long)value);
}

InstrSeq garnishSeq(long value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["meta"].index),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["Number"].index),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::CLONE),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                           makeAssemblerLine(Instr::INT, value),
                           makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET));
    return seq;
}

InstrSeq garnishSeq(Symbolic value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["meta"].index),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["Symbol"].index),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::CLONE),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                           makeAssemblerLine(Instr::SYMN, value.index),
                           makeAssemblerLine(Instr::LOAD, Reg::SYM),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET));
    return seq;
}
/*
InstrSeq garnishSeq(const InstrSeq& value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["meta"].index),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["Method"].index),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::CLONE),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                           makeAssemblerLine(Instr::MTHD, value),
                           makeAssemblerLine(Instr::LOAD, Reg::MTHD),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET));
    return seq;
}
*/
