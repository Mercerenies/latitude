#include "Standard.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "Header.hpp"
#include "GC.hpp"
#include <list>
#include <sstream>
#include <fstream>
#include <boost/scope_exit.hpp>

using namespace std;

// TODO Some syntax sugar for pattern matching key-value pairs
//    (So we can do a `capture3` which returns three values)
//    (Or maybe a "variable bomb" method which introduces variables into the local scope)

// TODO Something about the fact that system calls cause Latitude to "double report" stack frames
//  std/core.lat: 180
//  std/core.lat: 180
// Where one of these is technically a system call that isn't in core.lat

// TODO Unicode support

ObjectPtr defineMethod(TranslationUnitPtr unit, ObjectPtr global, ObjectPtr method, InstrSeq&& code) {
    ObjectPtr obj = clone(method);
    (makeAssemblerLine(Instr::RET)).appendOnto(code);
    FunctionIndex index = unit->pushMethod(code);
    obj.lock()->prim(Method(unit, index));
    obj.lock()->put(Symbols::get()["closure"], global);
    return obj;
}

ObjectPtr defineMethodNoRet(TranslationUnitPtr unit, ObjectPtr global, ObjectPtr method, InstrSeq&& code) {
    ObjectPtr obj = clone(method);
    FunctionIndex index = unit->pushMethod(code);
    obj.lock()->prim(Method(unit, index));
    obj.lock()->put(Symbols::get()["closure"], global);
    return obj;
}

void spawnSystemCallsNew(ObjectPtr global, ObjectPtr method, ObjectPtr sys, IntState& state) {
    static constexpr long
        TERMINATE = 0,
        KERNEL_LOAD = 1,
        STREAM_DIR = 2,
        STREAM_PUT = 3,
        TO_STRING = 4,
        GENSYM = 5,
        INSTANCE_OF = 6,
        STREAM_READ = 7,
        EVAL = 8,
        SYM_NAME = 9,
        SYM_NUM = 10,
        SYM_INTERN = 11,
        SIMPLE_CMP = 12,
        NUM_LEVEL = 13,
        ORIGIN = 14,
        PROCESS_TASK = 15,
        OBJECT_KEYS = 16,
        FILE_OPEN = 17,
        FILE_CLOSE = 18,
        FILE_EOF = 19,
        STRING_LENGTH = 20,
        STRING_SUB = 21,
        STRING_FIND = 22,
        GC_RUN = 23,
        FILE_HEADER = 24,
        STR_ORD = 25,
        STR_CHR = 26,
        GC_TOTAL = 27;

    TranslationUnitPtr unit = make_shared<TranslationUnit>();

    // TERMINATE
    state.cpp[TERMINATE] = [](IntState& state0) {
        // A last-resort termination of a fiber that malfunctioned; this should ONLY
        // be used as a last resort, as it does not correctly unwind the frames
        // before aborting
        state0 = intState();
    };

    // KERNEL_LOAD ($1 = filename, $2 = global)
    // kernelLoad#: filename, global.
    state.cpp[KERNEL_LOAD] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr str = (*dyn.lock())[ Symbols::get()["$1"] ].getPtr();
        ObjectPtr global = (*dyn.lock())[ Symbols::get()["$2"] ].getPtr();
        if ((!str.expired()) && (!global.expired())) {
            auto str0 = boost::get<string>(&str.lock()->prim());
            if (str0)
                readFile(*str0, { global, global }, state0);
            else
                throwError(state0, "TypeError", "String expected");
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys.lock()->put(Symbols::get()["kernelLoad#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::CPP, KERNEL_LOAD))));

    // accessSlot#: obj, sym.
    sys.lock()->put(Symbols::get()["accessSlot#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV))));

    // doClone#: obj, sym.
    sys.lock()->put(Symbols::get()["doClone#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE))));

    // invoke#: obj, mthd.
    sys.lock()->put(Symbols::get()["invoke#"],
                    defineMethodNoRet(unit, global, method,
                                      asmCode(makeAssemblerLine(Instr::GETD),
                                              makeAssemblerLine(Instr::SYM, "$1"),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                              makeAssemblerLine(Instr::GETD),
                                              makeAssemblerLine(Instr::SYM, "$2"),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                              // We want to forward the parent's arguments
                                              makeAssemblerLine(Instr::RET),
                                              makeAssemblerLine(Instr::CALL, 0L))));

    // STREAM_DIR ($1 = argument) (where %num0 specifies the direction; 0 = in, 1 = out)
    // streamIn#: stream.
    // streamOut#: stream.
    state.cpp[STREAM_DIR] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn.lock())[ Symbols::get()["$1"] ].getPtr();
        if (!stream.expired()) {
            auto stream0 = boost::get<StreamPtr>(&stream.lock()->prim());
            if (stream0) {
                switch (state0.num0.asSmallInt()) {
                case 0:
                    garnishEnd(state0, (*stream0)->hasIn());
                    break;
                case 1:
                    garnishEnd(state0, (*stream0)->hasOut());
                    break;
                }
            } else {
                throwError(state0, "TypeError", "Stream expected");
            }
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys.lock()->put(Symbols::get()["streamIn#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, STREAM_DIR))));
    sys.lock()->put(Symbols::get()["streamOut#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, STREAM_DIR))));

    // STREAM_PUT ($1 = stream, $2 = string) (where %num0 specifies whether a newline is added; 0 = no, 1 = yes)
    // streamPuts#: stream, str.
    // streamPutln#: stream, str.
    state.cpp[STREAM_PUT] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn.lock())[ Symbols::get()["$1"] ].getPtr();
        ObjectPtr str = (*dyn.lock())[ Symbols::get()["$2"] ].getPtr();
        if ((!stream.expired()) && (!str.expired())) {
            auto stream0 = boost::get<StreamPtr>(&stream.lock()->prim());
            auto str0 = boost::get<string>(&str.lock()->prim());
            if (stream0 && str0) {
                if ((*stream0)->hasOut()) {
                    switch (state0.num0.asSmallInt()) {
                    case 0:
                        (*stream0)->writeText(*str0);
                        break;
                    case 1:
                        (*stream0)->writeLine(*str0);
                        break;
                    }
                    garnishEnd(state0, boost::blank());
                } else {
                    throwError(state0, "IOError", "Stream not designated for output");
                }
            }
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys.lock()->put(Symbols::get()["streamPuts#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, STREAM_PUT))));
    sys.lock()->put(Symbols::get()["streamPutln#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, STREAM_PUT))));

    // TO_STRING (where %num0 specifies which register to use)
    //   0 = %num1
    //   1 = %str0
    //   2 = %sym
    // numToString#: num
    // strToString#: str
    // symToString#: sym
    state.cpp[TO_STRING] = [](IntState& state0) {
        ostringstream oss;
        switch (state0.num0.asSmallInt()) {
        case 0:
            oss << state0.num1.asString();
            break;
        case 1:
            oss << '"';
            for (char ch : state0.str0) {
                if (ch == '"')
                    oss << "\\\"";
                else if (ch == '\\')
                    oss << "\\\\";
                else if (ch == '\n')
                    oss << "\\\n";
                else if (ch == '\r')
                    oss << "\\\r";
                else if (ch == '\a')
                    oss << "\\\a";
                else if (ch == '\b')
                    oss << "\\\b";
                else if (ch == '\v')
                    oss << "\\\v";
                else if (ch == '\t')
                    oss << "\\\t";
                else if (ch == '\f')
                    oss << "\\\f";
                else
                    oss << ch;
            }
            oss << '"';
            break;
        case 2: {
            string str = Symbols::get()[state0.sym];
            if (Symbols::requiresEscape(str)) {
                oss << "'(";
                for (char ch : str) {
                    if (ch == '(')
                        oss << "\\(";
                    else if (ch == ')')
                        oss << "\\)";
                    else if (ch == '\\')
                        oss << "\\\\";
                    else
                        oss << ch;
                }
                oss << ")";
            } else {
                if (!Symbols::isUninterned(str))
                    oss << '\'';
                oss << str;
            }
        }
            break;
        }
        garnishEnd(state0, oss.str());
    };
    sys.lock()->put(Symbols::get()["numToString#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, TO_STRING))));
    sys.lock()->put(Symbols::get()["strToString#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, TO_STRING))));
    sys.lock()->put(Symbols::get()["symToString#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                         makeAssemblerLine(Instr::INT, 2L),
                                         makeAssemblerLine(Instr::CPP, TO_STRING))));

    // GENSYM (if %num0 is 1 then use %str0 as prefix, else if %num0 is 0 use default prefix; store in %sym)
    // gensym#: self.
    // gensymOf#: self, prefix.
    state.cpp[GENSYM] = [](IntState& state0) {
        switch (state0.num0.asSmallInt()) {
        case 0:
            state0.sym = Symbols::gensym();
            break;
        case 1:
            state0.sym = Symbols::gensym(state0.str0);
            break;
        }
    };
    sys.lock()->put(Symbols::get()["gensym#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, GENSYM),
                                         makeAssemblerLine(Instr::LOAD, Reg::SYM),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["gensymOf#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, GENSYM),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::LOAD, Reg::SYM),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // ptrEquals#: obj1, obj2.
    sys.lock()->put(Symbols::get()["ptrEquals#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::TEST),
                                         makeAssemblerLine(Instr::BOL))));

    // ifThenElse#: trueValue, cond, mthd0, mthd1.
    // _onTrue# and _onFalse# have underscores in front of their names for a reason.
    // DON'T call them directly; they will corrupt your call stack if called from
    // anywhere other than ifThenElse#.
    sys.lock()->put(Symbols::get()["_onTrue#"],
                    defineMethodNoRet(unit, global, method,
                                      asmCode(makeAssemblerLine(Instr::GETD),
                                              makeAssemblerLine(Instr::SYM, "$3"),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                              makeAssemblerLine(Instr::CALL, 0L))));
    sys.lock()->put(Symbols::get()["_onFalse#"],
                    defineMethodNoRet(unit, global, method,
                                      asmCode(makeAssemblerLine(Instr::GETD),
                                              makeAssemblerLine(Instr::SYM, "$4"),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                              makeAssemblerLine(Instr::CALL, 0L))));
    sys.lock()->put(Symbols::get()["ifThenElse#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "self"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "_onTrue#"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "self"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "_onFalse#"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::MTHDZ),
                                         makeAssemblerLine(Instr::THROA, "Method expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::MTHD),
                                         makeAssemblerLine(Instr::THROA, "Method expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::TEST),
                                         makeAssemblerLine(Instr::BRANCH))));

    // putSlot#: obj, sym, val.
    sys.lock()->put(Symbols::get()["putSlot#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$3"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::SETF),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // callCC#: newCont, mthd.
    // exitCC#: cont, ret.
    sys.lock()->put(Symbols::get()["callCC#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::CCALL))));
    sys.lock()->put(Symbols::get()["exitCC#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::CRET))));

    // Note that these two "methods" in particular do not have well-defined return values.
    // thunk#: before, after.
    // unthunk#.
    sys.lock()->put(Symbols::get()["thunk#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::WND),
                                         makeAssemblerLine(Instr::THROA, "Method expected"))));
    sys.lock()->put(Symbols::get()["unthunk#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::UNWND))));

    // INSTANCE_OF (check if %slf is an instance of %ptr, put result in %flag)
    // instanceOf#: obj, anc
    state.cpp[INSTANCE_OF] = [](IntState& state0) {
        auto hier = hierarchy(state0.slf);
        state0.flag = (find_if(hier.begin(), hier.end(),
                               [&state0](auto& o){ return o.lock() == state0.ptr.lock(); }) != hier.end());
    };
    sys.lock()->put(Symbols::get()["instanceOf#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::CPP, INSTANCE_OF),
                                         makeAssemblerLine(Instr::BOL))));

    // throw#: obj.
    sys.lock()->put(Symbols::get()["throw#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::THROW))));

    // handler#: obj.
    // unhandler#.
    sys.lock()->put(Symbols::get()["handler#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::HAND))));
    sys.lock()->put(Symbols::get()["unhandler#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::POP, Reg::HAND),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // kill#.
    sys.lock()->put(Symbols::get()["kill#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::CPP, 0L))));

    // STREAM_READ ($1 = stream) (constructs and stores the resulting string in %ret, uses %num0 for mode)
    // - 0 - Read a line
    // - 1 - Read a single character
    // streamRead#: stream.
    state.cpp[STREAM_READ] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn.lock())[ Symbols::get()["$1"] ].getPtr();
        if (!stream.expired()) {
            auto stream0 = boost::get<StreamPtr>(&stream.lock()->prim());
            if (stream0) {
                if ((*stream0)->hasIn()) {
                    if (state0.num0.asSmallInt() == 0)
                        garnishEnd(state0, (*stream0)->readLine());
                    else
                        garnishEnd(state0, (*stream0)->readText(1));
                } else {
                    throwError(state0, "IOError", "Stream not designated for output");
                }
            } else {
                throwError(state0, "TypeError", "Stream expected");
            }
        }
    };
    sys.lock()->put(Symbols::get()["streamRead#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, STREAM_READ))));
    sys.lock()->put(Symbols::get()["streamReadChar#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, STREAM_READ))));

    // EVAL (where %str0 is a string to evaluate; throws if something goes wrong)
    // eval#: lex, dyn, str.
    state.cpp[EVAL] = [](IntState& state0) {
        eval(state0, state0.str0);
    };
    sys.lock()->put(Symbols::get()["eval#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$3"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::DYN),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::LEX),
                                         makeAssemblerLine(Instr::CPP, EVAL),
                                         makeAssemblerLine(Instr::NRET))));

    // stringConcat#: str1, str2.
    sys.lock()->put(Symbols::get()["stringConcat#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "meta"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "String"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR1),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::ADDS),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::LOAD, Reg::STR0),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // numAdd#: n1, n2.
    // numSub#: n1, n2.
    // numMul#: n1, n2.
    // numDiv#: n1, n2.
    // numMod#: n1, n2.
    // numPow#: n1, n2.
    sys.lock()->put(Symbols::get()["numAdd#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "meta"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "Number"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::ARITH, 1L),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numSub#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "meta"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "Number"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::ARITH, 2L),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numMul#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "meta"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "Number"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::ARITH, 3L),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numDiv#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "meta"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "Number"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::ARITH, 4L),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numMod#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "meta"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "Number"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::ARITH, 5L),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["numPow#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETL),
                                         makeAssemblerLine(Instr::SYM, "meta"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::SYM, "Number"),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::ARITH, 6L),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // SYM_NAME (takes %sym, looks up its name, and outputs a string as %ret)
    // symName#: sym.
    state.cpp[SYM_NAME] = [](IntState& state0) {
        std::string name = Symbols::get()[ state0.sym ];
        garnishEnd(state0, name);
    };
    sys.lock()->put(Symbols::get()["symName#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                         makeAssemblerLine(Instr::CPP, SYM_NAME))));

    // SYM_NUM (takes %num0 and outputs an appropriate symbol to %ret)
    // natSym#: num.
    state.cpp[SYM_NUM] = [](IntState& state0) {
        if (state0.num0.asSmallInt() <= 0) {
            throwError(state0, "TypeError", "Cannot produce symbols from non-positive numbers");
        } else {
            Symbolic sym = Symbols::natural((int)state0.num0.asSmallInt());
            garnishEnd(state0, sym);
        }
    };
    sys.lock()->put(Symbols::get()["natSym#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::CPP, SYM_NUM))));

    // doWithCallback#: self, mthd, modifier
    // (This one manipulates the call stack a bit, so there is no RET at the end; there's one
    //  in the middle though that has basically the same effect)
    sys.lock()->put(Symbols::get()["doWithCallback#"],
                    defineMethodNoRet(unit, global, method,
                                      asmCode(makeAssemblerLine(Instr::GETD),
                                              makeAssemblerLine(Instr::SYM, "$3"),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                              makeAssemblerLine(Instr::GETD),
                                              makeAssemblerLine(Instr::SYM, "$2"),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                              makeAssemblerLine(Instr::GETD),
                                              makeAssemblerLine(Instr::SYM, "$1"),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::RET),
                                              makeAssemblerLine(Instr::XCALL0, 0L),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET),
                                              makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                              makeAssemblerLine(Instr::POP, Reg::LEX),
                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG),
                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO),
                                              makeAssemblerLine(Instr::POP, Reg::DYN),
                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG),
                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO),
                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                              makeAssemblerLine(Instr::CALL, 2L),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::DYN),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::LEX),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                              makeAssemblerLine(Instr::POP, Reg::STO),
                                              makeAssemblerLine(Instr::XCALL))));

    // SYM_INTERN (takes %str0, looks it up, and puts the result as a symbol in %ret)
    // intern#: str.
    state.cpp[SYM_INTERN] = [](IntState& state0) {
        Symbolic name = Symbols::get()[ state0.str0 ];
        garnishEnd(state0, name);
    };
    sys.lock()->put(Symbols::get()["intern#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::CPP, SYM_INTERN))));

    // SIMPLE_CMP (compares %slf's and %ptr's respective prim fields based on the value of %num0)
    // - 0 - Compare for equality and put the result in %flag
    // - 1 - Compare for LT and put the result in %flag
    // In any case, if either argument lacks a prim or the prim fields have different types, false
    // is returned by default.
    // SIMPLE_CMP will compare strings, numbers, and symbols. Anything else returns false.
    // primEquals#: lhs, rhs.
    // primLT#: lhs, rhs.
    state.cpp[SIMPLE_CMP] = [](IntState& state0) {
        bool doLT = false;
        if (state0.num0.asSmallInt() == 1)
            doLT = true;
        auto magicCmp = [doLT, &state0](auto x, auto y) {
            if (doLT)
                state0.flag = (*x < *y);
            else
                state0.flag = (*x == *y);
        };
        state0.flag = false;
        auto prim0 = state0.slf.lock()->prim();
        auto prim1 = state0.ptr.lock()->prim();
        auto n0 = boost::get<Number>(&prim0);
        auto n1 = boost::get<Number>(&prim1);
        auto st0 = boost::get<string>(&prim0);
        auto st1 = boost::get<string>(&prim1);
        auto sy0 = boost::get<Symbolic>(&prim0);
        auto sy1 = boost::get<Symbolic>(&prim1);
        if (n0 && n1)
            magicCmp(n0, n1);
        else if (st0 && st1)
            magicCmp(st0, st1);
        else if (sy0 && sy1)
            magicCmp(sy0, sy1);
    };
    sys.lock()->put(Symbols::get()["primEquals#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, SIMPLE_CMP),
                                         makeAssemblerLine(Instr::BOL))));
    sys.lock()->put(Symbols::get()["primLT#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, SIMPLE_CMP),
                                         makeAssemblerLine(Instr::BOL))));

    // NUM_LEVEL (determine the "level" of %num0 and put the result in %ret)
    // numLevel#: num.
    state.cpp[NUM_LEVEL] = [](IntState& state0) {
        garnishEnd(state0, state0.num0.hierarchyLevel());
    };
    sys.lock()->put(Symbols::get()["numLevel#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::CPP, NUM_LEVEL))));

    // stackTrace#.
    // stackTrace# only includes %trace, not %line nor %file. %line and %file are often
    // undesired in this case as they represent only the line and file of the stackTrace#
    // call, which is bogus, as stackTrace# is not defined in a file.
    sys.lock()->put(Symbols::get()["stackTrace#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::LOCRT))));

    // ORIGIN (find the origin of %sym in %slf, store resulting object in %ret, throw SlotError otherwise
    // origin#: self, sym.
    state.cpp[ORIGIN] = [](IntState& state0) {
        list<ObjectSPtr> parents;
        ObjectSPtr curr = state0.slf.lock();
        Symbolic name = state0.sym;
        ObjectPtr value;
        while (find(parents.begin(), parents.end(), curr) == parents.end()) {
            parents.push_back(curr);
            Slot slot = (*curr)[name];
            if (slot.getType() == SlotType::PTR) {
                value = curr;
                break;
            }
            curr = (*curr)[ Symbols::get()["parent"] ].getPtr().lock();
        }
        if (value.expired()) {
            throwError(state0, "SlotError", "Cannot find origin of nonexistent slot");
        } else {
            state0.ret = value;
        }
    };
    sys.lock()->put(Symbols::get()["origin#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                         makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::CPP, ORIGIN))));

    // PROCESS_TASK - Do something with %slf and possibly other registers, based on %num0
    // - 0 - Create (%slf should be `Process`, %str0 should be the command, and %ret will be a new clone)
    // - 1 - Exec (%prcs should be the process, no return value)
    // - 2 - OutStream (%prcs should be the process, %ptr a stream object)
    // - 3 - InStream (%prcs should be the process, %ptr a stream object)
    // - 4 - ErrStream (%prcs should be the process, %ptr a stream object)
    // - 5 - IsRunning (%prcs should be the process, %ret will be a boolean)
    // - 6 - ExitCode (%prcs should be the process, %ret will be the number)
    // - 7 - IsFinished (%prcs should be the process, %ret will be a boolean)
    // processInStream#: strm, prc.
    // processOutStream#: strm, prc.
    // processErrStream#: strm, prc.
    // processCreate#: self, str.
    // processFinished#: prc.
    // processRunning#: prc.
    // processExitCode#: prc.
    // processExec#: prc.
    state.cpp[PROCESS_TASK] = [](IntState& state0) {
        switch (state0.num0.asSmallInt()) {
        case 0: {
            ProcessPtr proc = makeProcess(state0.str0);
            if (!proc)
                throwError(state0, "NotSupportedError",
                           "Asynchronous processes not supported on this system");
            state0.ret = clone(state0.slf);
            state0.ret.lock()->prim(proc);
            break;
        }
        case 1: {
            bool status = state0.prcs->run();
            if (!status)
                throwError(state0, "IOError",
                           "Could not start process");
            break;
        }
        case 2: {
            state0.ptr.lock()->prim(state0.prcs->stdOut());
            break;
        }
        case 3: {
            state0.ptr.lock()->prim(state0.prcs->stdIn());
            break;
        }
        case 4: {
            state0.ptr.lock()->prim(state0.prcs->stdErr());
            break;
        }
        case 5: {
            garnishEnd(state0, state0.prcs->isRunning());
            break;
        }
        case 6: {
            garnishEnd(state0, state0.prcs->getExitCode());
            break;
        }
        case 7: {
            garnishEnd(state0, state0.prcs->isDone());
            break;
        }
        }
    };
    sys.lock()->put(Symbols::get()["processInStream#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::INT, 3L),
                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["processOutStream#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::INT, 2L),
                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["processErrStream#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::INT, 4L),
                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys.lock()->put(Symbols::get()["processCreate#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
    sys.lock()->put(Symbols::get()["processFinished#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                         makeAssemblerLine(Instr::INT, 7L),
                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
    sys.lock()->put(Symbols::get()["processRunning#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                         makeAssemblerLine(Instr::INT, 5L),
                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
    sys.lock()->put(Symbols::get()["processExitCode#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                         makeAssemblerLine(Instr::INT, 6L),
                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
    sys.lock()->put(Symbols::get()["processExec#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                         makeAssemblerLine(Instr::THROA, "Process expected"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET),
                                         makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, PROCESS_TASK))));

    // OBJECT_KEYS (takes an object in %slf and outputs an array-like construct using `meta brackets`
    //              which contains all of the slot names of %slf)
    // objectKeys#: obj.
    state.cpp[OBJECT_KEYS] = [](IntState& state0) {
        set<Symbolic> allKeys = keys(state0.slf);
        state0.stack = pushNode(state0.stack, state0.cont);
        state0.cont = asmCode(makeAssemblerLine(Instr::GETL),
                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                              makeAssemblerLine(Instr::SYM, "meta"),
                              makeAssemblerLine(Instr::RTRV),
                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                              makeAssemblerLine(Instr::SYM, "brackets"),
                              makeAssemblerLine(Instr::RTRV),
                              makeAssemblerLine(Instr::GETL),
                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                              makeAssemblerLine(Instr::CALL, 0L),
                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO));
        for (Symbolic sym : allKeys) {
            InstrSeq seq = garnishSeq(sym);
            (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);
            (makeAssemblerLine(Instr::PEEK, Reg::STO)).appendOnto(seq);
            (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(seq);
            (makeAssemblerLine(Instr::SYM, "next")).appendOnto(seq);
            (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
            (makeAssemblerLine(Instr::PEEK, Reg::STO)).appendOnto(seq);
            (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(seq);
            (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
            (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq);
            state0.cont.insert(state0.cont.end(), seq.begin(), seq.end());
        }
        (makeAssemblerLine(Instr::PEEK, Reg::STO)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::SYM, "finish")).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::RTRV)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::PEEK, Reg::STO)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::CALL, 0L)).appendOnto(state0.cont);
        (makeAssemblerLine(Instr::POP, Reg::STO)).appendOnto(state0.cont);
    };
    sys.lock()->put(Symbols::get()["objectKeys#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CPP, OBJECT_KEYS))));

    // FILE_OPEN (%str0 is the filename, %ptr is a stream object to be filled, $3 and $4 are access and mode)
    // streamFileOpen#: strm, fname, access, mode.
    state.cpp[FILE_OPEN] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr access = (*dyn.lock())[ Symbols::get()["$3"] ].getPtr();
        ObjectPtr mode = (*dyn.lock())[ Symbols::get()["$4"] ].getPtr();
        auto access0 = boost::get<Symbolic>(&access.lock()->prim());
        auto mode0 = boost::get<Symbolic>(&mode.lock()->prim());
        if (access0 && mode0) {
            auto access1 = Symbols::get()[*access0];
            auto mode1 = Symbols::get()[*mode0];
            FileAccess access2;
            FileMode mode2;
            if (access1 == "read")
                access2 = FileAccess::READ;
            else if (access1 == "write")
                access2 = FileAccess::WRITE;
            else
                throwError(state0, "SystemArgError",
                           "Invalid access specifier when opening file");
            if (mode1 == "text")
                mode2 = FileMode::TEXT;
            else if (mode1 == "binary")
                mode2 = FileMode::BINARY;
            else
                throwError(state0, "SystemArgError",
                           "Invalid mode specifier when opening file");
            state0.ptr.lock()->prim( StreamPtr(new FileStream(state0.str0, access2, mode2)) );
        } else {
            throwError(state0, "TypeError",
                       "Symbol expected");
        }
    };
    sys.lock()->put(Symbols::get()["streamFileOpen#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::CPP, FILE_OPEN),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // FILE_CLOSE (takes %strm and closes it)
    // streamClose#: strm.
    state.cpp[FILE_CLOSE] = [](IntState& state0) {
        state0.strm->close();
    };
    sys.lock()->put(Symbols::get()["streamClose#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                         makeAssemblerLine(Instr::THROA, "Stream expected"),
                                         makeAssemblerLine(Instr::CPP, FILE_CLOSE),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // FILE_EOF (takes %strm and outputs whether it's at eof into %flag)
    // streamEof#: strm.
    state.cpp[FILE_EOF] = [](IntState& state0) {
        state0.flag = state0.strm->isEof();
    };
    sys.lock()->put(Symbols::get()["streamEof#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                         makeAssemblerLine(Instr::THROA, "Stream expected"),
                                         makeAssemblerLine(Instr::CPP, FILE_EOF),
                                         makeAssemblerLine(Instr::BOL))));

    // STRING_LENGTH (outputs length of %str0 into %ret)
    // stringLength#: str.
    state.cpp[STRING_LENGTH] = [](IntState& state0) {
        // TODO Possible loss of precision from size_t to signed long?
        garnishEnd(state0, (long)state0.str0.length());
    };
    sys.lock()->put(Symbols::get()["stringLength#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::CPP, STRING_LENGTH))));

    // STRING_SUB (outputs substring of %str0 from %num0 to %num1 into %ret)
    // stringSubstring#: str, beg, end.
    state.cpp[STRING_SUB] = [](IntState& state0) {
        long start1 = state0.num0.asSmallInt();
        long end1 = state0.num1.asSmallInt();
        long size = state0.str0.length();
        if (start1 < 0)
            start1 += size;
        if (end1 < 0)
            end1 += size;
        if (start1 >= size)
            start1 = size;
        if (end1 >= size)
            end1 = size;
        if (start1 < 0)
            start1 = 0;
        if (end1 < 0)
            end1 = 0;
        long len = end1 - start1;
        if (len < 0)
            len = 0;
        garnishEnd(state0, state0.str0.substr(start1, len));
    };
    sys.lock()->put(Symbols::get()["stringSubstring#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$3"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::CPP, STRING_SUB))));

    // STRING_FIND (find first occurence of %str1 in %str0 starting at %num0 index, storing
    //              new index or Nil in %ret)
    // stringFindFirst#: str, substr, pos.
    state.cpp[STRING_FIND] = [](IntState& state0) {
        auto pos = state0.str0.find(state0.str1, state0.num0.asSmallInt());
        if (pos == string::npos)
            garnishEnd(state0, boost::blank());
        else
            garnishEnd(state0, (long)pos);
    };
    sys.lock()->put(Symbols::get()["stringFindFirst#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$2"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                         makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$3"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR1),
                                         makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::CPP, STRING_FIND))));

    // GC_RUN (run the garbage collector and store the number of objects deleted at %ret)
    // runGC#.
    state.cpp[GC_RUN] = [](IntState& state0) {
        long result = GC::get().garbageCollect(state0);
        garnishEnd(state0, result);
    };
    sys.lock()->put(Symbols::get()["runGC#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::CPP, GC_RUN))));

    // FILE_HEADER (check the %str0 file and put a FileHeader object in %ret)
    // fileHeader#: filename.
    state.cpp[FILE_HEADER] = [](IntState& state0) {
        InstrSeq intro = asmCode(makeAssemblerLine(Instr::GETL),
                                 makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                 makeAssemblerLine(Instr::SYM, "meta"),
                                 makeAssemblerLine(Instr::RTRV),
                                 makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                 makeAssemblerLine(Instr::SYM, "FileHeader"),
                                 makeAssemblerLine(Instr::RTRV),
                                 makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                 makeAssemblerLine(Instr::CLONE),
                                 makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF));
        InstrSeq mid;
        Header header = getFileHeader(state0.str0);
        if (header.fields & (unsigned int)HeaderField::MODULE) {
            InstrSeq curr = garnishSeq(header.module);
            InstrSeq pre = asmCode(makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO));
            InstrSeq post = asmCode(makeAssemblerLine(Instr::POP, Reg::STO),
                                    makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                    makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                    makeAssemblerLine(Instr::SYM, "moduleName"),
                                    makeAssemblerLine(Instr::SETF));
            mid.insert(mid.end(), pre.begin(), pre.end());
            mid.insert(mid.end(), curr.begin(), curr.end());
            mid.insert(mid.end(), post.begin(), post.end());
        }
        if (header.fields & (unsigned int)HeaderField::PACKAGE) {
            InstrSeq curr = garnishSeq(header.package);
            InstrSeq pre = asmCode(makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO));
            InstrSeq post = asmCode(makeAssemblerLine(Instr::POP, Reg::STO),
                                    makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                    makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                    makeAssemblerLine(Instr::SYM, "packageName"),
                                    makeAssemblerLine(Instr::SETF));
            mid.insert(mid.end(), pre.begin(), pre.end());
            mid.insert(mid.end(), curr.begin(), curr.end());
            mid.insert(mid.end(), post.begin(), post.end());
        }
        InstrSeq concl = asmCode(makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET));
        state0.stack = pushNode(state0.stack, state0.cont);
        state0.cont.clear();
        state0.cont.insert(state0.cont.end(), intro.begin(), intro.end());
        state0.cont.insert(state0.cont.end(), mid.begin(), mid.end());
        state0.cont.insert(state0.cont.end(), concl.begin(), concl.end());
    };
    sys.lock()->put(Symbols::get()["fileHeader#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::CPP, FILE_HEADER))));

    // STR_ORD (check the %str0 register and put the ASCII value in %ret, empty string returns 0)
    // STR_CHR (check the %num0 register and put the character in %ret)
    // strOrd#: str.
    // strChr#: num.
    state.cpp[STR_ORD] = [](IntState& state0) {
        if (state0.str0 == "")
            garnishEnd(state0, 0);
        else
            garnishEnd(state0, (int)state0.str0[0]);
    };
    state.cpp[STR_CHR] = [](IntState& state0) {
        garnishEnd(state0, string(1, (char)state0.num0.asSmallInt()));
    };
    sys.lock()->put(Symbols::get()["strOrd#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                         makeAssemblerLine(Instr::THROA, "String expected"),
                                         makeAssemblerLine(Instr::CPP, STR_ORD))));
    sys.lock()->put(Symbols::get()["strChr#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::GETD),
                                         makeAssemblerLine(Instr::SYM, "$1"),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::RTRV),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::ECLR),
                                         makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                         makeAssemblerLine(Instr::THROA, "Number expected"),
                                         makeAssemblerLine(Instr::CPP, STR_CHR))));

    // GC_TOTAL (get the total number of allocated objects from the garbage collector and store it in %ret)
    // totalGC#.
    state.cpp[GC_TOTAL] = [](IntState& state0) {
        // TODO Possible loss of precision
        garnishEnd(state0, (long)GC::get().getTotal());
    };
    sys.lock()->put(Symbols::get()["totalGC#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::CPP, GC_TOTAL))));

}

ObjectPtr spawnObjects(IntState& state) {

    ObjectPtr object(GC::get().allocate());
    ObjectPtr meta(clone(object));
    ObjectPtr global(clone(object));

    ObjectPtr proc(clone(object));
    ObjectPtr method(clone(proc));
    ObjectPtr number(clone(object));
    ObjectPtr string(clone(object));
    ObjectPtr symbol(clone(object));

    ObjectPtr cont(clone(proc));

    ObjectPtr exception(clone(object));
    ObjectPtr systemError(clone(exception));

    ObjectPtr process(clone(object));
    ObjectPtr stream(clone(object));
    ObjectPtr stdout_(clone(stream));
    ObjectPtr stdin_(clone(stream));
    ObjectPtr stderr_(clone(stream));

    ObjectPtr array_(clone(object));

    ObjectPtr sys(clone(object));
    ObjectPtr stackFrame(clone(object));
    ObjectPtr fileHeader(clone(object));
    ObjectPtr kernel(clone(object));

    ObjectPtr nil(clone(object));
    ObjectPtr boolean(clone(object));
    ObjectPtr true_(clone(boolean));
    ObjectPtr false_(clone(boolean));

    // Meta calls for basic types
    meta.lock()->put(Symbols::get()["Object"], object);
    meta.lock()->put(Symbols::get()["Proc"], proc);
    meta.lock()->put(Symbols::get()["Method"], method);
    meta.lock()->put(Symbols::get()["Number"], number);
    meta.lock()->put(Symbols::get()["String"], string);
    meta.lock()->put(Symbols::get()["Symbol"], symbol);
    meta.lock()->put(Symbols::get()["Stream"], stream);
    meta.lock()->put(Symbols::get()["Process"], process);
    meta.lock()->put(Symbols::get()["True"], true_);
    meta.lock()->put(Symbols::get()["False"], false_);
    meta.lock()->put(Symbols::get()["Nil"], nil);
    meta.lock()->put(Symbols::get()["Boolean"], boolean);
    meta.lock()->put(Symbols::get()["Cont"], cont);
    meta.lock()->put(Symbols::get()["sys"], sys);
    meta.lock()->put(Symbols::get()["Exception"], exception);
    meta.lock()->put(Symbols::get()["SystemError"], systemError);
    meta.lock()->put(Symbols::get()["Array"], array_);
    meta.lock()->put(Symbols::get()["Kernel"], kernel);

    // Object is its own parent
    object.lock()->put(Symbols::get()["parent"], object);

    // Meta linkage
    meta.lock()->put(Symbols::get()["meta"], meta);
    object.lock()->put(Symbols::get()["meta"], meta);
    meta.lock()->put(Symbols::get()["sys"], sys);
    meta.lock()->put(Symbols::get()["StackFrame"], stackFrame);
    meta.lock()->put(Symbols::get()["FileHeader"], fileHeader);

    // Global variables not accessible in meta
    global.lock()->put(Symbols::get()["stdin"], stdin_);
    global.lock()->put(Symbols::get()["stderr"], stderr_);
    global.lock()->put(Symbols::get()["stdout"], stdout_);
    global.lock()->put(Symbols::get()["global"], global);

    state.lex.push(global);
    state.dyn.push(global);

    // Method and system call properties
    spawnSystemCallsNew(global, method, sys, state);

    // Prim Fields
    // TODO Should `method` have a prim field?
    //method.lock()->prim(asmCode(makeAssemblerLine(Instr::RET)));
    number.lock()->prim(0.0);
    string.lock()->prim("");
    symbol.lock()->prim(Symbols::get()[""]);
    stdout_.lock()->prim(outStream());
    stdin_.lock()->prim(inStream());
    stderr_.lock()->prim(errStream());

    // The core libraries
    //readFile("std/latitude.lat", { global, global }, state);

    return global;
}

void throwError(IntState& state, std::string name, std::string msg) {
    state.stack = pushNode(state.stack, state.cont);
    state.cont = asmCode(makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                         makeAssemblerLine(Instr::GETL),
                         makeAssemblerLine(Instr::SYM, "meta"),
                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                         makeAssemblerLine(Instr::RTRV),
                         makeAssemblerLine(Instr::SYM, name),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::RTRV),
                         makeAssemblerLine(Instr::POP, Reg::STO),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::CLONE),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::SYM, "message"),
                         makeAssemblerLine(Instr::SETF),
                         makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                         makeAssemblerLine(Instr::LOCRT),
                         makeAssemblerLine(Instr::POP, Reg::STO),
                         makeAssemblerLine(Instr::SYM, "stack"),
                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                         makeAssemblerLine(Instr::SETF),
                         makeAssemblerLine(Instr::THROW));
    state.stack = pushNode(state.stack, state.cont);
    state.cont.clear();
    garnishEnd(state, msg);
}

void throwError(IntState& state, std::string name) {
    state.stack = pushNode(state.stack, state.cont);
    state.cont = asmCode(makeAssemblerLine(Instr::GETL),
                         makeAssemblerLine(Instr::SYM, "meta"),
                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                         makeAssemblerLine(Instr::RTRV),
                         makeAssemblerLine(Instr::SYM, name),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::RTRV),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::CLONE),
                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                         makeAssemblerLine(Instr::GETD),
                         makeAssemblerLine(Instr::SYM, "stack"),
                         makeAssemblerLine(Instr::SETF),
                         makeAssemblerLine(Instr::THROW));
}
