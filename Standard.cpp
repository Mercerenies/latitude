#include "Standard.hpp"
#include "Assembler.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "Header.hpp"
#include "GC.hpp"
#include "Environment.hpp"
#include "Pathname.hpp"
#include <list>
#include <sstream>
#include <fstream>
#include <boost/scope_exit.hpp>
#include <boost/optional.hpp>

using namespace std;

// TODO Some syntax sugar for pattern matching key-value pairs
//    (So we can do a `capture3` which returns three values)
//    (Or maybe a "variable bomb" method which introduces variables into the local scope)

// TODO Something about the fact that system calls cause Latitude to "double report" stack frames
//  std/core.lat: 180
//  std/core.lat: 180
// Where one of these is technically a system call that isn't in core.lat

ObjectPtr defineMethod(TranslationUnitPtr unit, ObjectPtr global, ObjectPtr method, InstrSeq&& code) {
    ObjectPtr obj = clone(method);
    (makeAssemblerLine(Instr::RET)).appendOnto(code);
    FunctionIndex index = unit->pushMethod(code);
    obj->prim(Method(unit, index));
    obj->put(Symbols::get()["closure"], global);
    return obj;
}

ObjectPtr defineMethodNoRet(TranslationUnitPtr unit, ObjectPtr global, ObjectPtr method, InstrSeq&& code) {
    ObjectPtr obj = clone(method);
    FunctionIndex index = unit->pushMethod(code);
    obj->prim(Method(unit, index));
    obj->put(Symbols::get()["closure"], global);
    return obj;
}

void spawnSystemCallsNew(ObjectPtr global,
                         ObjectPtr method,
                         ObjectPtr sys,
                         ReadOnlyState& reader) {
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
        FILE_DOOPEN = 17,
        FILE_DOCLOSE = 18,
        FILE_EOF = 19,
        STRING_LENGTH = 20,
        STRING_SUB = 21,
        STRING_FIND = 22,
        GC_RUN = 23,
        FILE_HEADER = 24,
        STR_ORD = 25,
        STR_CHR = 26,
        GC_TOTAL = 27,
        TIME_SPAWN = 28,
        ENV_GET = 29,
        ENV_SET = 30,
        EXE_PATH = 31,
        PATH_OP = 32,
        FILE_EXISTS = 33,
        TRIG_OP = 34,
        MATH_FLOOR = 35,
        NUM_CONST = 36,
        PROT_VAR = 37,
        PROT_IS = 38,
        STR_NEXT = 39,
        COMPLEX = 40;

    TranslationUnitPtr unit = make_shared<TranslationUnit>();

    // TERMINATE
    reader.cpp[TERMINATE] = [](IntState& state0) {
        // A last-resort termination of a fiber that malfunctioned; this should ONLY
        // be used as a last resort, as it does not correctly unwind the frames
        // before aborting
        hardKill(state0);
    };

    // KERNEL_LOAD ($1 = filename, $2 = global)
    //  * Checks %num0 (if 0, then standard load; if 1, then raw load)
    // kernelLoad#: filename, global.
    // kernelLoadRaw#: filename, global.
    reader.cpp[KERNEL_LOAD] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr str = (*dyn)[ Symbols::get()["$1"] ].getPtr();
        ObjectPtr global = (*dyn)[ Symbols::get()["$2"] ].getPtr();
        if ((str != nullptr) && (global != nullptr)) {
            auto str0 = boost::get<string>(&str->prim());
            if (str0) {
                string str1 = *str0;
                if (state0.num0.asSmallInt() == 1)
                    str1 = stripFilename(getExecutablePathname()) + str1;
                readFile(str1, { global, global }, state0);
            } else {
                throwError(state0, "TypeError", "String expected");
            }
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys->put(Symbols::get()["kernelLoad#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 0),
                                  makeAssemblerLine(Instr::CPP, KERNEL_LOAD))));
    sys->put(Symbols::get()["kernelLoad0#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 1),
                                  makeAssemblerLine(Instr::CPP, KERNEL_LOAD))));

    // accessSlot#: obj, sym.
    sys->put(Symbols::get()["accessSlot#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                  makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                  makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::RTRV))));

    // doClone#: obj, sym.
    sys->put(Symbols::get()["doClone#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::CLONE))));

    // invoke#: obj, mthd.
    sys->put(Symbols::get()["invoke#"],
                    defineMethodNoRet(unit, global, method,
                                      asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                              makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                              makeAssemblerLine(Instr::GETD, Reg::SLF),
                                              makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                              makeAssemblerLine(Instr::RTRV),
                                              makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                              // We want to forward the parent's arguments
                                              makeAssemblerLine(Instr::RET),
                                              makeAssemblerLine(Instr::CALL, 0L))));

    // STREAM_DIR ($1 = argument) (where %num0 specifies the direction; 0 = in, 1 = out)
    // streamIn#: stream.
    // streamOut#: stream.
    reader.cpp[STREAM_DIR] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn)[ Symbols::get()["$1"] ].getPtr();
        if (stream != nullptr) {
            auto stream0 = boost::get<StreamPtr>(&stream->prim());
            if (stream0) {
                switch (state0.num0.asSmallInt()) {
                case 0:
                    garnishBegin(state0, (*stream0)->hasIn());
                    break;
                case 1:
                    garnishBegin(state0, (*stream0)->hasOut());
                    break;
                }
            } else {
                throwError(state0, "TypeError", "Stream expected");
            }
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys->put(Symbols::get()["streamIn#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, STREAM_DIR))));
    sys->put(Symbols::get()["streamOut#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, STREAM_DIR))));

    // STREAM_PUT ($1 = stream, $2 = string) (where %num0 specifies whether a newline is added; 0 = no, 1 = yes)
    // streamPuts#: stream, str.
    // streamPutln#: stream, str.
    reader.cpp[STREAM_PUT] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn)[ Symbols::get()["$1"] ].getPtr();
        ObjectPtr str = (*dyn)[ Symbols::get()["$2"] ].getPtr();
        if ((stream != nullptr) && (str != nullptr)) {
            auto stream0 = boost::get<StreamPtr>(&stream->prim());
            auto str0 = boost::get<string>(&str->prim());
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
                    garnishBegin(state0, boost::blank());
                } else {
                    throwError(state0, "IOError", "Stream not designated for output");
                }
            } else {
                throwError(state0, "SystemArgError", "Invalid argument to output function");
            }
        } else {
            throwError(state0, "SystemArgError", "Wrong number of arguments");
        }
    };
    sys->put(Symbols::get()["streamPuts#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 0L),
                                  makeAssemblerLine(Instr::CPP, STREAM_PUT))));
    sys->put(Symbols::get()["streamPutln#"],
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
    reader.cpp[TO_STRING] = [](IntState& state0) {
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
        garnishBegin(state0, oss.str());
    };
    sys->put(Symbols::get()["numToString#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::INT, 0L),
                                  makeAssemblerLine(Instr::CPP, TO_STRING))));
    sys->put(Symbols::get()["strToString#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                  makeAssemblerLine(Instr::THROA, "String expected"),
                                  makeAssemblerLine(Instr::INT, 1L),
                                  makeAssemblerLine(Instr::CPP, TO_STRING))));
    sys->put(Symbols::get()["symToString#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
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
    reader.cpp[GENSYM] = [](IntState& state0) {
        switch (state0.num0.asSmallInt()) {
        case 0:
        state0.sym = Symbols::gensym();
        break;
        case 1:
        state0.sym = Symbols::gensym(state0.str0);
        break;
        }
    };
    sys->put(Symbols::get()["gensym#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::INT, 0L),
                                  makeAssemblerLine(Instr::CPP, GENSYM),
                                  makeAssemblerLine(Instr::LOAD, Reg::SYM),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["gensymOf#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                  makeAssemblerLine(Instr::THROA, "String expected"),
                                  makeAssemblerLine(Instr::INT, 1L),
                                  makeAssemblerLine(Instr::CPP, GENSYM),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::SYM),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // ptrEquals#: obj1, obj2.
    sys->put(Symbols::get()["ptrEquals#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::TEST),
                                  makeAssemblerLine(Instr::BOL))));

    // ifThenElse#: trueValue, cond, mthd0, mthd1.
    // _onTrue# and _onFalse# have underscores in front of their names for a reason.
    // DON'T call them directly; they will corrupt your call stack if called from
    // anywhere other than ifThenElse#.
    sys->put(Symbols::get()["_onTrue#"],
             defineMethodNoRet(unit, global, method,
                               asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                       makeAssemblerLine(Instr::SYMN, Symbols::get()["$3"].index),
                                       makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                       makeAssemblerLine(Instr::RTRV),
                                       makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                       makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                       makeAssemblerLine(Instr::CALL, 0L))));
    sys->put(Symbols::get()["_onFalse#"],
             defineMethodNoRet(unit, global, method,
                               asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                       makeAssemblerLine(Instr::SYMN, Symbols::get()["$4"].index),
                                       makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                       makeAssemblerLine(Instr::RTRV),
                                       makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                       makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                       makeAssemblerLine(Instr::CALL, 0L))));
    sys->put(Symbols::get()["ifThenElse#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETL, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["self"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["_onTrue#"].index),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETL, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["self"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["_onFalse#"].index),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::MTHDZ),
                                  makeAssemblerLine(Instr::THROA, "Method expected"),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::MTHD),
                                  makeAssemblerLine(Instr::THROA, "Method expected"),
                                  makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::TEST),
                                  makeAssemblerLine(Instr::BRANCH))));

    // putSlot#: obj, sym, val.
    sys->put(Symbols::get()["putSlot#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$3"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                  makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                  makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::SETF),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // callCC#: newCont, mthd.
    // exitCC#: cont, ret.
    sys->put(Symbols::get()["callCC#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::CCALL))));
    sys->put(Symbols::get()["exitCC#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::CRET))));

    // Note that these two "methods" in particular do not have well-defined return values.
    // thunk#: before, after.
    // unthunk#.
    sys->put(Symbols::get()["thunk#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::WND),
                                  makeAssemblerLine(Instr::THROA, "Method expected"))));
    sys->put(Symbols::get()["unthunk#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::UNWND))));

    // INSTANCE_OF (check if %slf is an instance of %ptr, put result in %flag)
    // instanceOf#: obj, anc
    reader.cpp[INSTANCE_OF] = [](IntState& state0) {
        auto hier = hierarchy(state0.slf);
        state0.flag = (find_if(hier.begin(), hier.end(),
                               [&state0](auto& o){ return o == state0.ptr; }) != hier.end());
    };
    sys->put(Symbols::get()["instanceOf#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::CPP, INSTANCE_OF),
                                  makeAssemblerLine(Instr::BOL))));

    // throw#: obj.
    sys->put(Symbols::get()["throw#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::THROW))));

    // handler#: obj.
    // unhandler#.
    sys->put(Symbols::get()["handler#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::HAND))));
    sys->put(Symbols::get()["unhandler#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::POP, Reg::RET, Reg::HAND))));

    // kill#.
    sys->put(Symbols::get()["kill#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::CPP, 0L))));

    // STREAM_READ ($1 = stream) (constructs and stores the resulting string in %ret, uses %num0 for mode)
    // - 0 - Read a line
    // - 1 - Read a single character
    // streamRead#: stream.
    reader.cpp[STREAM_READ] = [](IntState& state0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn)[ Symbols::get()["$1"] ].getPtr();
        if (stream != NULL) {
            auto stream0 = boost::get<StreamPtr>(&stream->prim());
            if (stream0) {
                if ((*stream0)->hasIn()) {
                    if (state0.num0.asSmallInt() == 0)
                        garnishBegin(state0, (*stream0)->readLine());
                    else
                        garnishBegin(state0, (*stream0)->readText(1));
                } else {
                    throwError(state0, "IOError", "Stream not designated for output");
                }
            } else {
                throwError(state0, "TypeError", "Stream expected");
            }
        }
    };
    sys->put(Symbols::get()["streamRead#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 0L),
                                  makeAssemblerLine(Instr::CPP, STREAM_READ))));
    sys->put(Symbols::get()["streamReadChar#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 1L),
                                  makeAssemblerLine(Instr::CPP, STREAM_READ))));

    // EVAL (where %str0 is a string to evaluate; throws if something goes wrong)
    // eval#: lex, dyn, str.
    reader.cpp[EVAL] = [](IntState& state0) {
        eval(state0, state0.str0);
    };
    sys->put(Symbols::get()["eval#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$3"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                  makeAssemblerLine(Instr::THROA, "String expected"),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::DYN),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::LEX),
                                  makeAssemblerLine(Instr::CPP, EVAL),
                                  makeAssemblerLine(Instr::NRET))));

    // stringConcat#: str1, str2.
    sys->put(Symbols::get()["stringConcat#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::YLDC, Lit::STRING, Reg::RET),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                  makeAssemblerLine(Instr::THROA, "String expected"),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::STR1),
                                  makeAssemblerLine(Instr::THROA, "String expected"),
                                  makeAssemblerLine(Instr::ADDS),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::STR0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // numAdd#: n1, n2.
    // numSub#: n1, n2.
    // numMul#: n1, n2.
    // numDiv#: n1, n2.
    // numMod#: n1, n2.
    // numPow#: n1, n2.
    sys->put(Symbols::get()["numAdd#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::RET),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::ARITH, 1L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["numSub#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::RET),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::ARITH, 2L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["numMul#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::RET),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::ARITH, 3L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["numDiv#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::RET),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::ARITH, 4L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["numMod#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::RET),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::ARITH, 5L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["numPow#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::RET),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::ARITH, 6L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // SYM_NAME (takes %sym, looks up its name, and outputs a string as %ret)
    // symName#: sym.
    reader.cpp[SYM_NAME] = [](IntState& state0) {
        std::string name = Symbols::get()[ state0.sym ];
        garnishBegin(state0, name);
    };
    sys->put(Symbols::get()["symName#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                  makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                  makeAssemblerLine(Instr::CPP, SYM_NAME))));

    // SYM_NUM (takes %num0 and outputs an appropriate symbol to %ret)
    // natSym#: num.
    reader.cpp[SYM_NUM] = [](IntState& state0) {
        if (state0.num0.asSmallInt() <= 0) {
            throwError(state0, "TypeError", "Cannot produce symbols from non-positive numbers");
        } else {
            Symbolic sym = Symbols::natural((int)state0.num0.asSmallInt());
            garnishBegin(state0, sym);
        }
    };
    sys->put(Symbols::get()["natSym#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::CPP, SYM_NUM))));

    // doWithCallback#: self, mthd, modifier
    // (This one manipulates the call stack a bit, so there is no RET at the end; there's one
    //  in the middle though that has basically the same effect)
    sys->put(Symbols::get()["doWithCallback#"],
             defineMethodNoRet(unit, global, method,
                               asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                       makeAssemblerLine(Instr::SYMN, Symbols::get()["$3"].index),
                                       makeAssemblerLine(Instr::RTRV),
                                       makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                       makeAssemblerLine(Instr::GETD, Reg::SLF),
                                       makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                       makeAssemblerLine(Instr::RTRV),
                                       makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                       makeAssemblerLine(Instr::GETD, Reg::SLF),
                                       makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                       makeAssemblerLine(Instr::RTRV),
                                       makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                       makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                       makeAssemblerLine(Instr::RET),
                                       makeAssemblerLine(Instr::XCALL0, 0L),
                                       makeAssemblerLine(Instr::POP, Reg::RET, Reg::STO),
                                       makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO),
                                       makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                       makeAssemblerLine(Instr::POP, Reg::PTR, Reg::LEX),
                                       makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG),
                                       makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO),
                                       makeAssemblerLine(Instr::POP, Reg::PTR, Reg::DYN),
                                       makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG),
                                       makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO),
                                       makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                       makeAssemblerLine(Instr::CALL, 2L),
                                       makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                       makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::DYN),
                                       makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                       makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::LEX),
                                       makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                       makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                       makeAssemblerLine(Instr::XCALL))));

    // SYM_INTERN (takes %str0, looks it up, and puts the result as a symbol in %ret)
    // intern#: str.
    reader.cpp[SYM_INTERN] = [](IntState& state0) {
        Symbolic name = Symbols::get()[ state0.str0 ];
        garnishBegin(state0, name);
    };
    sys->put(Symbols::get()["intern#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
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
    reader.cpp[SIMPLE_CMP] = [](IntState& state0) {
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
        auto prim0 = state0.slf->prim();
        auto prim1 = state0.ptr->prim();
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
    sys->put(Symbols::get()["primEquals#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::INT, 0L),
                                  makeAssemblerLine(Instr::CPP, SIMPLE_CMP),
                                  makeAssemblerLine(Instr::BOL))));
    sys->put(Symbols::get()["primLT#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::INT, 1L),
                                  makeAssemblerLine(Instr::CPP, SIMPLE_CMP),
                                  makeAssemblerLine(Instr::BOL))));

    // NUM_LEVEL (determine the "level" of %num0 and put the result in %ret)
    // numLevel#: num.
    reader.cpp[NUM_LEVEL] = [](IntState& state0) {
        garnishBegin(state0, state0.num0.hierarchyLevel());
    };
    sys->put(Symbols::get()["numLevel#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
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
    sys->put(Symbols::get()["stackTrace#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::LOCRT))));

    // ORIGIN (find the origin of %sym in %slf, store resulting object in %ret, throw SlotError otherwise
    // origin#: self, sym.
    reader.cpp[ORIGIN] = [](IntState& state0) {
        list<ObjectPtr> parents;
        ObjectPtr curr = state0.slf;
        Symbolic name = state0.sym;
        ObjectPtr value = nullptr;
        while (find(parents.begin(), parents.end(), curr) == parents.end()) {
            parents.push_back(curr);
            Slot slot = (*curr)[name];
            if (slot.getType() == SlotType::PTR) {
                value = curr;
                break;
            }
            curr = (*curr)[ Symbols::get()["parent"] ].getPtr();
        }
        if (value == nullptr) {
            throwError(state0, "SlotError", "Cannot find origin of nonexistent slot");
        } else {
            state0.ret = value;
        }
    };
    sys->put(Symbols::get()["origin#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                  makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                  makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
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
    reader.cpp[PROCESS_TASK] = [](IntState& state0) {
        switch (state0.num0.asSmallInt()) {
        case 0: {
            ProcessPtr proc = makeProcess(state0.str0);
            if (!proc)
                throwError(state0, "NotSupportedError",
                           "Asynchronous processes not supported on this system");
            state0.ret = clone(state0.slf);
            state0.ret->prim(proc);
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
            state0.ptr->prim(state0.prcs->stdOut());
            break;
        }
        case 3: {
            state0.ptr->prim(state0.prcs->stdIn());
            break;
        }
        case 4: {
            state0.ptr->prim(state0.prcs->stdErr());
            break;
        }
        case 5: {
            garnishBegin(state0, state0.prcs->isRunning());
            break;
        }
        case 6: {
            garnishBegin(state0, state0.prcs->getExitCode());
            break;
        }
        case 7: {
            garnishBegin(state0, state0.prcs->isDone());
            break;
        }
        }
    };
    sys->put(Symbols::get()["processInStream#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                  makeAssemblerLine(Instr::THROA, "Process expected"),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::INT, 3L),
                                  makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["processOutStream#"],
                      defineMethod(unit, global, method,
                                   asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                           makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                           makeAssemblerLine(Instr::RTRV),
                                           makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                           makeAssemblerLine(Instr::GETD, Reg::SLF),
                                           makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                           makeAssemblerLine(Instr::RTRV),
                                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                           makeAssemblerLine(Instr::ECLR),
                                           makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                           makeAssemblerLine(Instr::THROA, "Process expected"),
                                           makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                           makeAssemblerLine(Instr::INT, 2L),
                                           makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
     sys->put(Symbols::get()["processErrStream#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                   makeAssemblerLine(Instr::THROA, "Process expected"),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::INT, 4L),
                                   makeAssemblerLine(Instr::CPP, PROCESS_TASK),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
     sys->put(Symbols::get()["processCreate#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                   makeAssemblerLine(Instr::INT, 0L),
                                   makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
     sys->put(Symbols::get()["processFinished#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                   makeAssemblerLine(Instr::THROA, "Process expected"),
                                   makeAssemblerLine(Instr::INT, 7L),
                                   makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
     sys->put(Symbols::get()["processRunning#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                   makeAssemblerLine(Instr::THROA, "Process expected"),
                                   makeAssemblerLine(Instr::INT, 5L),
                                   makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
     sys->put(Symbols::get()["processExitCode#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::PRCS),
                                   makeAssemblerLine(Instr::THROA, "Process expected"),
                                   makeAssemblerLine(Instr::INT, 6L),
                                   makeAssemblerLine(Instr::CPP, PROCESS_TASK))));
     sys->put(Symbols::get()["processExec#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
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
     reader.cpp[OBJECT_KEYS] = [](IntState& state0) {
         set<Symbolic> allKeys = keys(state0.slf);
         InstrSeq total = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["meta"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["brackets"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::GETL, Reg::SLF),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::CALL, 0L),
                                  makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO));
         for (Symbolic sym : allKeys) {
             InstrSeq seq = garnishSeq(sym);
             (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::ARG)).appendOnto(seq);
             (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
             (makeAssemblerLine(Instr::SYMN, Symbols::get()["next"].index)).appendOnto(seq);
             (makeAssemblerLine(Instr::RTRV)).appendOnto(seq);
             (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(seq);
             (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq);
             (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq);
             total.insert(total.end(), seq.begin(), seq.end());
         }
         (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(total);
         (makeAssemblerLine(Instr::SYMN, Symbols::get()["finish"].index)).appendOnto(total);
         (makeAssemblerLine(Instr::RTRV)).appendOnto(total);
         (makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO)).appendOnto(total);
         (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(total);
         (makeAssemblerLine(Instr::CALL, 0L)).appendOnto(total);
         (makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)).appendOnto(total);
         state0.stack = pushNode(state0.stack, state0.cont);
         state0.cont = CodeSeek(total);
     };
     sys->put(Symbols::get()["objectKeys#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                   makeAssemblerLine(Instr::CPP, OBJECT_KEYS))));

     // FILE_DOOPEN (%str0 is the filename, %ptr is a stream object to be filled, $3 and $4 are access and mode)
     // streamFileOpen#: strm, fname, access, mode.
     reader.cpp[FILE_DOOPEN] = [](IntState& state0) {
         ObjectPtr dyn = state0.dyn.top();
         ObjectPtr access = (*dyn)[ Symbols::get()["$3"] ].getPtr();
         ObjectPtr mode = (*dyn)[ Symbols::get()["$4"] ].getPtr();
         auto access0 = boost::get<Symbolic>(&access->prim());
         auto mode0 = boost::get<Symbolic>(&mode->prim());
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
             state0.ptr->prim( StreamPtr(new FileStream(state0.str0, access2, mode2)) );
         } else {
             throwError(state0, "TypeError",
                        "Symbol expected");
         }
     };
     sys->put(Symbols::get()["streamFileOpen#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::CPP, FILE_DOOPEN),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // FILE_DOCLOSE (takes %strm and closes it)
     // streamClose#: strm.
     reader.cpp[FILE_DOCLOSE] = [](IntState& state0) {
         state0.strm->close();
     };
     sys->put(Symbols::get()["streamClose#"],
              defineMethod(unit, global, method,
                                   asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                           makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                           makeAssemblerLine(Instr::RTRV),
                                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                           makeAssemblerLine(Instr::ECLR),
                                           makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                           makeAssemblerLine(Instr::THROA, "Stream expected"),
                                           makeAssemblerLine(Instr::CPP, FILE_DOCLOSE),
                                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // FILE_EOF (takes %strm and outputs whether it's at eof into %flag)
     // streamEof#: strm.
     reader.cpp[FILE_EOF] = [](IntState& state0) {
         state0.flag = state0.strm->isEof();
     };
     sys->put(Symbols::get()["streamEof#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                   makeAssemblerLine(Instr::THROA, "Stream expected"),
                                   makeAssemblerLine(Instr::CPP, FILE_EOF),
                                   makeAssemblerLine(Instr::BOL))));

     // STRING_LENGTH (outputs length of %str0 into %ret)
     // stringLength#: str.
     reader.cpp[STRING_LENGTH] = [](IntState& state0) {
         // TODO Possible loss of precision from size_t to signed long?
         garnishBegin(state0, (long)state0.str0.length());
     };
     sys->put(Symbols::get()["stringLength#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, STRING_LENGTH))));

     // STRING_SUB (outputs substring of %str0 from %num0 to %num1 into %ret)
     // stringSubstring#: str, beg, end.
     reader.cpp[STRING_SUB] = [](IntState& state0) {
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
         garnishBegin(state0, state0.str0.substr(start1, len));
     };
     sys->put(Symbols::get()["stringSubstring#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$3"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, STRING_SUB))));

     // STRING_FIND (find first occurence of %str1 in %str0 starting at %num0 index, storing
     //              new index or Nil in %ret)
     // stringFindFirst#: str, substr, pos.
     reader.cpp[STRING_FIND] = [](IntState& state0) {
         auto pos = state0.str0.find(state0.str1, state0.num0.asSmallInt());
         if (pos == string::npos)
             garnishBegin(state0, boost::blank());
         else
             garnishBegin(state0, (long)pos);
     };
     sys->put(Symbols::get()["stringFindFirst#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$3"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR1),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, STRING_FIND))));

     // GC_RUN (run the garbage collector and store the number of objects deleted at %ret)
     // runGC#.
     reader.cpp[GC_RUN] = [&reader](IntState& state0) {
         // TODO This assumes the reader that's in use is the same as the reader that was created
         //      with this reader. Remove this unusual dependency somehow
         long result = GC::get().garbageCollect(state0, reader);
         garnishBegin(state0, result);
     };
     sys->put(Symbols::get()["runGC#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::CPP, GC_RUN))));

     // FILE_HEADER (check the %str0 file and put a FileHeader object in %ret)
     // fileHeader#: filename.
     reader.cpp[FILE_HEADER] = [](IntState& state0) {
         InstrSeq intro = asmCode(makeAssemblerLine(Instr::YLDC, Lit::FHEAD, Reg::SLF));
         InstrSeq mid;
         Header header = getFileHeader(state0.str0);
         if (header.fields & (unsigned int)HeaderField::MODULE) {
             InstrSeq curr = garnishSeq(header.module);
             InstrSeq pre = asmCode(makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO));
             InstrSeq post = asmCode(makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                     makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                     makeAssemblerLine(Instr::SYMN, Symbols::get()["moduleName"].index),
                                     makeAssemblerLine(Instr::SETF));
             mid.insert(mid.end(), pre.begin(), pre.end());
             mid.insert(mid.end(), curr.begin(), curr.end());
             mid.insert(mid.end(), post.begin(), post.end());
         }
         if (header.fields & (unsigned int)HeaderField::PACKAGE) {
             InstrSeq curr = garnishSeq(header.package);
             InstrSeq pre = asmCode(makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO));
             InstrSeq post = asmCode(makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                     makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                     makeAssemblerLine(Instr::SYMN, Symbols::get()["packageName"].index),
                                     makeAssemblerLine(Instr::SETF));
             mid.insert(mid.end(), pre.begin(), pre.end());
             mid.insert(mid.end(), curr.begin(), curr.end());
             mid.insert(mid.end(), post.begin(), post.end());
         }
         InstrSeq concl = asmCode(makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET));
         InstrSeq total;
         total.insert(total.end(), intro.begin(), intro.end());
         total.insert(total.end(), mid.begin(), mid.end());
         total.insert(total.end(), concl.begin(), concl.end());
         state0.stack = pushNode(state0.stack, state0.cont);
         state0.cont = CodeSeek(total);
     };
     sys->put(Symbols::get()["fileHeader#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
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
     reader.cpp[STR_ORD] = [](IntState& state0) {
         if (state0.str0 == "")
             garnishBegin(state0, 0);
         else
             garnishBegin(state0, (int)state0.str0[0]);
     };
     reader.cpp[STR_CHR] = [](IntState& state0) {
         garnishBegin(state0, string(1, (char)state0.num0.asSmallInt()));
     };
     sys->put(Symbols::get()["strOrd#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, STR_ORD))));
     sys->put(Symbols::get()["strChr#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::CPP, STR_CHR))));

     // GC_TOTAL (get the total number of allocated objects from the garbage collector and store it in %ret)
     // totalGC#.
     reader.cpp[GC_TOTAL] = [](IntState& state0) {
         // TODO Possible loss of precision
         garnishBegin(state0, (long)GC::get().getTotal());
     };
     sys->put(Symbols::get()["totalGC#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::CPP, GC_TOTAL))));

     // TIME_SPAWN (put all the information about the current system time in the %ptr object, using %num0 to
     //             determine whether local time (1) or global time (2))
     // timeSpawnLocal#: obj.
     // timeSpawnGlobal#: obj.
     reader.cpp[TIME_SPAWN] = [](IntState& state0) {
         time_t raw;
         tm info;
         time(&raw);
         if (state0.num0.asSmallInt() == 1)
             info = *localtime(&raw);
         else if (state0.num0.asSmallInt() == 2)
             info = *gmtime(&raw);
         InstrSeq prologue = asmCode(makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO));
         InstrSeq second = garnishSeq(info.tm_sec);
         InstrSeq second1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                    makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                    makeAssemblerLine(Instr::SYMN, Symbols::get()["second"].index),
                                    makeAssemblerLine(Instr::SETF));
         InstrSeq minute = garnishSeq(info.tm_min);
         InstrSeq minute1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                    makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                    makeAssemblerLine(Instr::SYMN, Symbols::get()["minute"].index),
                                    makeAssemblerLine(Instr::SETF));
         InstrSeq hour = garnishSeq(info.tm_hour);
         InstrSeq hour1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["hour"].index),
                                  makeAssemblerLine(Instr::SETF));
         InstrSeq day = garnishSeq(info.tm_mday);
         InstrSeq day1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                 makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                 makeAssemblerLine(Instr::SYMN, Symbols::get()["day"].index),
                                 makeAssemblerLine(Instr::SETF));
         InstrSeq month = garnishSeq(info.tm_mon);
         InstrSeq month1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["monthNumber"].index),
                                   makeAssemblerLine(Instr::SETF));
         InstrSeq year = garnishSeq(info.tm_year + 1900);
         InstrSeq year1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["year"].index),
                                  makeAssemblerLine(Instr::SETF));
         InstrSeq weekday = garnishSeq(info.tm_wday);
         InstrSeq weekday1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                     makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                     makeAssemblerLine(Instr::SYMN, Symbols::get()["weekdayNumber"].index),
                                     makeAssemblerLine(Instr::SETF));
         InstrSeq yearday = garnishSeq(info.tm_yday + 1);
         InstrSeq yearday1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                     makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                     makeAssemblerLine(Instr::SYMN, Symbols::get()["yearDay"].index),
                                     makeAssemblerLine(Instr::SETF));
         InstrSeq dst = garnishSeq(info.tm_isdst);
         InstrSeq dst1 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                 makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                 makeAssemblerLine(Instr::SYMN, Symbols::get()["dstNumber"].index),
                                 makeAssemblerLine(Instr::SETF));
         InstrSeq epilogue = asmCode(makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO));
         InstrSeq total;
         total.insert(total.end(), prologue.begin(), prologue.end());
         total.insert(total.end(), second.begin(), second.end());
         total.insert(total.end(), second1.begin(), second1.end());
         total.insert(total.end(), minute.begin(), minute.end());
         total.insert(total.end(), minute1.begin(), minute1.end());
         total.insert(total.end(), hour.begin(), hour.end());
         total.insert(total.end(), hour1.begin(), hour1.end());
         total.insert(total.end(), day.begin(), day.end());
         total.insert(total.end(), day1.begin(), day1.end());
         total.insert(total.end(), month.begin(), month.end());
         total.insert(total.end(), month1.begin(), month1.end());
         total.insert(total.end(), year.begin(), year.end());
         total.insert(total.end(), year1.begin(), year1.end());
         total.insert(total.end(), weekday.begin(), weekday.end());
         total.insert(total.end(), weekday1.begin(), weekday1.end());
         total.insert(total.end(), yearday.begin(), yearday.end());
         total.insert(total.end(), yearday1.begin(), yearday1.end());
         total.insert(total.end(), dst.begin(), dst.end());
         total.insert(total.end(), dst1.begin(), dst1.end());
         total.insert(total.end(), epilogue.begin(), epilogue.end());
         state0.stack = pushNode(state0.stack, state0.cont);
         state0.cont = CodeSeek(std::move(total));
     };
     sys->put(Symbols::get()["timeSpawnLocal#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, TIME_SPAWN),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
     sys->put(Symbols::get()["timeSpawnGlobal#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::INT, 2),
                                   makeAssemblerLine(Instr::CPP, TIME_SPAWN),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // ENV_GET (retrieve the environment variable matching the name in %str0 and store the result in %ret)
     // envGet#: str.
     reader.cpp[ENV_GET] = [](IntState& state0) {
         boost::optional<std::string> value = getEnv(state0.str0);
         if (value)
             garnishBegin(state0, *value);
         else
             garnishBegin(state0, boost::blank());
     };
     sys->put(Symbols::get()["envGet#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, ENV_GET))));

     // ENV_SET (assign the environment variable with name %str0 to value %str1 (or unset it, if %num0 is nonzero)
     // envSet#: name, value.
     // envUnset#: name.
     reader.cpp[ENV_SET] = [](IntState& state0) {
         bool success = false;
         if (state0.num0.asSmallInt() != 0) {
             success = unsetEnv(state0.str0);
         } else {
             success = setEnv(state0.str0, state0.str1);
         }
         if (success)
             garnishBegin(state0, boost::blank());
         else
             throwError(state0, "NotSupportedError",
                        "Mutable environment variables not supported on this system");
     };
     sys->put(Symbols::get()["envSet#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR1),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::INT, 0),
                                   makeAssemblerLine(Instr::CPP, ENV_SET))));
     sys->put(Symbols::get()["envUnset#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::STR, ""),
                                   makeAssemblerLine(Instr::SSWAP),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, ENV_SET))));

     // EXE_PATH (put an appropriate pathname into %ret%, given by the %num0 argument)
     //  * %num0 == 1: Executable pathname
     // exePath#.
     reader.cpp[EXE_PATH] = [](IntState& state0) {
         switch (state0.num0.asSmallInt()) {
         case 1:
             garnishBegin(state0, getExecutablePathname());
             break;
         default:
             throwError(state0, "SystemArgError",
                        "Invalid numerical argument to EXE_PATH");
             break;
         }
     };
     sys->put(Symbols::get()["exePath#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, EXE_PATH))));

     // PATH_OP (put an appropriate pathname into %ret%, given by the %num0 argument and %str0 input)
     //  * %num0 == 1: Get directory of pathname
     //  * %num0 == 2: Get filename of pathname
     // dirName#: str.
     reader.cpp[PATH_OP] = [](IntState& state0) {
         switch (state0.num0.asSmallInt()) {
         case 1:
             garnishBegin(state0, stripFilename(state0.str0));
         break;
         case 2:
             garnishBegin(state0, stripDirname(state0.str0));
             break;
         default:
             throwError(state0, "SystemArgError",
                        "Invalid numerical argument to PATH_OP");
             break;
         }
     };
     sys->put(Symbols::get()["dirName#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, PATH_OP))));
     sys->put(Symbols::get()["fileName#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::INT, 2),
                                   makeAssemblerLine(Instr::CPP, PATH_OP))));

     // FILE_EXISTS (check the pathname in %str0 for existence, storing result in %flag)
     // fileExists#: fname.
     reader.cpp[FILE_EXISTS] = [](IntState& state0) {
         std::ifstream f(state0.str0.c_str());
         state0.flag = f.good();
         f.close();
     };
     sys->put(Symbols::get()["fileExists#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, FILE_EXISTS),
                                   makeAssemblerLine(Instr::BOL))));

     // TRIG_OP (perform the operation indicated in %num0 on the value in %num1, storing result in %num1)
     // numTrig#: num, op.
     reader.cpp[TRIG_OP] = [](IntState& state0) {
         switch (state0.num0.asSmallInt()) {
         case 0: // Sin
             state0.num1 = state0.num1.sin();
             break;
         case 1: // Cos
             state0.num1 = state0.num1.cos();
             break;
         case 2: // Tan
             state0.num1 = state0.num1.tan();
             break;
         case 3: // Sinh
             state0.num1 = state0.num1.sinh();
             break;
         case 4: // Cosh
             state0.num1 = state0.num1.cosh();
             break;
         case 5: // Tanh
            state0.num1 = state0.num1.tanh();
             break;
         case 6: // Exp
             // So yes, exp() isn't technically a trig function, but it fits in nicely with this part of the code
             state0.num1 = state0.num1.exp();
             break;
         case 7: // Asin
             state0.num1 = state0.num1.asin();
             break;
         case 8: // Acos
             state0.num1 = state0.num1.acos();
             break;
         case 9: // Atan
             state0.num1 = state0.num1.atan();
             break;
         case 10: // Asinh
             state0.num1 = state0.num1.asinh();
             break;
         case 11: // Acosh
             state0.num1 = state0.num1.acosh();
             break;
         case 12: // Atanh
             state0.num1 = state0.num1.atanh();
             break;
         case 13: // Ln
             // See the comment on Exp()
             state0.num1 = state0.num1.log();
             break;
         default:
             throwError(state0, "SystemArgError",
                        "Invalid numerical argument to TRIG_OP");
             break;
         }
     };
     sys->put(Symbols::get()["numTrig#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::RET),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::CPP, TRIG_OP),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM1),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // MATH_FLOOR (floor the value in %num0, storing result in %num0)
     // numFloor#: num.
     reader.cpp[MATH_FLOOR] = [](IntState& state0) {
         state0.num0 = state0.num0.floor();
     };
     sys->put(Symbols::get()["numFloor#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::YLDC, Lit::NUMBER, Reg::RET),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::CPP, MATH_FLOOR),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // NUM_CONST (store something in %num1, based on the value of %num0, sets %err0 if non-applicable)
     // numNan#: number.
     // numInfinity#: number.
     // numNegInfinity#: number.
     // numEpsilon#: number.
     reader.cpp[NUM_CONST] = [](IntState& state0) {
         switch (state0.num0.asSmallInt()) {
         case 0: { // NaN
             auto num = constantNan();
             if (num) {
                 state0.num1 = *num;
             } else {
                 state0.err0 = true;
             }
             break;
         }
         case 1: { // infinity
             auto num = constantInf();
             if (num) {
                 state0.num1 = *num;
             } else {
                 state0.err0 = true;
             }
             break;
         }
         case 2: { // negative infinity
             auto num = constantNegInf();
             if (num) {
                 state0.num1 = *num;
             } else {
                 state0.err0 = true;
             }
             break;
         }
         case 3:
             state0.num1 = constantEps();
             break;
         }
     };
     sys->put(Symbols::get()["numNan#"],
              defineMethod(unit, global, method,
                   asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::INT, 0),
                           makeAssemblerLine(Instr::ECLR),
                           makeAssemblerLine(Instr::CPP, NUM_CONST),
                           makeAssemblerLine(Instr::THROA, "Invalid constant"),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                           makeAssemblerLine(Instr::LOAD, Reg::NUM1),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
     sys->put(Symbols::get()["numInfinity#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::CPP, NUM_CONST),
                                   makeAssemblerLine(Instr::THROA, "Invalid constant"),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM1),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
     sys->put(Symbols::get()["numNegInfinity#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::INT, 2),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::CPP, NUM_CONST),
                                   makeAssemblerLine(Instr::THROA, "Invalid constant"),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM1),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
     sys->put(Symbols::get()["numEpsilon#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::INT, 3),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::CPP, NUM_CONST),
                                   makeAssemblerLine(Instr::THROA, "Invalid constant"),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM1),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // PROT_VAR (protect the variable named %sym in the object %slf)
     // protectVar#: obj, var.
     reader.cpp[PROT_VAR] = [](IntState& state0) {
         state0.slf->protect(state0.sym);
     };
     sys->put(Symbols::get()["protectVar#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                   makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                   makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                   makeAssemblerLine(Instr::CPP, PROT_VAR),
                                   makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET))));

     // PROT_IS (check protection of the variable named %sym in the object %slf, returning %ret)
     // protectIs#: obj, var.
     reader.cpp[PROT_IS] = [](IntState& state0) {
         garnishBegin(state0, state0.slf->isProtected(state0.sym));
     };
     sys->put(Symbols::get()["protectIs#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                   makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                   makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                   makeAssemblerLine(Instr::CPP, PROT_IS))));

     // STR_NEXT (given %str0 and %num0, put next byte index in %ret, or Nil on failure)
     // stringNext#: str, num.
     reader.cpp[STR_NEXT] = [](IntState& state0) {
         // Based on the algorithm at http://stackoverflow.com/a/4063258/2288659
         bool valid = true;
         auto i = state0.num0.asSmallInt();
         auto full_len = state0.str0.length();
         unsigned char c = static_cast<unsigned char>(state0.str0[i]);
         unsigned long n = 0;
         if ((c & 0x80) == 0x00)
             n = 1;
         else if ((c & 0xE0) == 0xC0)
             n = 2;
         else if ((c & 0xF0) == 0xE0)
             n = 3;
         else if ((c & 0xF8) == 0xF0)
             n = 4;
         else
             valid = false;
         if (n + i > full_len)
             valid = false;
         if (valid) {
             for (unsigned long j = i + 1; j < i + n; j++) {
                 unsigned char c1 = static_cast<unsigned char>(state0.str0[j]);
                 if ((c1 & 0xC0) != 0x80)
                     valid = false;
             }
         }
         if (valid)
             garnishBegin(state0, static_cast<signed long>(i + n));
         else
             garnishBegin(state0, boost::blank());
     };
     sys->put(Symbols::get()["stringNext#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, STR_NEXT))));

     // COMPLEX (take %num0 as real part, %num1 as imaginary part, and produce complex number in %num0)
     // complexNumber#: number, re, im.
     reader.cpp[COMPLEX] = [](IntState& state0) {
         state0.num0 = complex_number(state0.num0, state0.num1);
     };
     sys->put(Symbols::get()["complexNumber#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$3"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::CPP, COMPLEX),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

}

ObjectPtr spawnObjects(IntState& state, ReadOnlyState& reader) {

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

    // Global calls for basic types
    global->put(Symbols::get()["Object"], object);
    global->put(Symbols::get()["Proc"], proc);
    global->put(Symbols::get()["Method"], method);
    global->put(Symbols::get()["Number"], number);
    global->put(Symbols::get()["String"], string);
    global->put(Symbols::get()["Symbol"], symbol);
    global->put(Symbols::get()["Stream"], stream);
    global->put(Symbols::get()["Process"], process);
    global->put(Symbols::get()["True"], true_);
    global->put(Symbols::get()["False"], false_);
    global->put(Symbols::get()["Nil"], nil);
    global->put(Symbols::get()["Boolean"], boolean);
    global->put(Symbols::get()["Cont"], cont);
    global->put(Symbols::get()["sys"], sys);
    global->put(Symbols::get()["Exception"], exception);
    global->put(Symbols::get()["SystemError"], systemError);
    global->put(Symbols::get()["Array"], array_);
    global->put(Symbols::get()["Kernel"], kernel);
    global->put(Symbols::get()["StackFrame"], stackFrame);
    global->put(Symbols::get()["FileHeader"], fileHeader);

    // Object is its own parent
    object->put(Symbols::get()["parent"], object);

    // Meta linkage
    meta->put(Symbols::get()["meta"], meta);
    object->put(Symbols::get()["meta"], meta);
    meta->put(Symbols::get()["sys"], sys);

    // Global variables not accessible in meta
    global->put(Symbols::get()["stdin"], stdin_);
    global->put(Symbols::get()["stderr"], stderr_);
    global->put(Symbols::get()["stdout"], stdout_);
    global->put(Symbols::get()["global"], global);

    state.lex.push(global);
    state.dyn.push(global);

    // Method and system call properties
    spawnSystemCallsNew(global, method, sys, reader);

    // Prim Fields
    // For pragmatic reasons, Method does NOT have a prim() field
    //method->prim(asmCode(makeAssemblerLine(Instr::RET)));
    number->prim(0.0);
    string->prim("");
    symbol->prim(Symbols::get()[""]);
    stdout_->prim(outStream());
    stdin_->prim(inStream());
    stderr_->prim(errStream());

    // Spawn the literal objects table
    reader.lit[Lit::NIL   ] = nil;
    reader.lit[Lit::FALSE ] = false_;
    reader.lit[Lit::TRUE  ] = true_;
    reader.lit[Lit::BOOL  ] = boolean;
    reader.lit[Lit::STRING] = string;
    reader.lit[Lit::NUMBER] = number;
    reader.lit[Lit::SYMBOL] = symbol;
    reader.lit[Lit::METHOD] = method;
    reader.lit[Lit::SFRAME] = stackFrame;
    reader.lit[Lit::FHEAD ] = fileHeader;

    // The core libraries (this is done in runREPL now)
    //readFile("std/latitude.lat", { global, global }, state);

    // TODO Use the new protection system to protect the global names that are defined here

    return global;
}

void throwError(IntState& state, std::string name, std::string msg) {
    state.stack = pushNode(state.stack, state.cont);
    state.cont = CodeSeek(asmCode(makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                  makeAssemblerLine(Instr::GETL, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["err"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::SYM, name),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::CLONE),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["message"].index),
                                  makeAssemblerLine(Instr::SETF),
                                  makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::LOCRT),
                                  makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["stack"].index),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::SETF),
                                  makeAssemblerLine(Instr::THROW)));
    garnishBegin(state, msg);
}

void throwError(IntState& state, std::string name) {
    state.stack = pushNode(state.stack, state.cont);
    state.cont = CodeSeek(asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["err"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::SYM, name),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::CLONE),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                  makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::LOCRT),
                                  makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["stack"].index),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::SETF),
                                  makeAssemblerLine(Instr::THROW)));
}
