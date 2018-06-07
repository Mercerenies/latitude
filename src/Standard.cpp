#include "Standard.hpp"
#include "Assembler.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "Header.hpp"
#include "GC.hpp"
#include "Environment.hpp"
#include "Pathname.hpp"
#include "Unicode.hpp"
#include "Platform.hpp"
#include "Dump.hpp"
#include "Parents.hpp"
#include "Precedence.hpp"
#include "Input.hpp"
#include <list>
#include <sstream>
#include <fstream>
#include <boost/scope_exit.hpp>
#include <boost/optional.hpp>

using namespace std;

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

Protection numToProtection(long num) {
    Protection prot = Protection::NO_PROTECTION;
    if (num & 1)
        prot |= Protection::PROTECT_ASSIGN;
    if (num & 2)
        prot |= Protection::PROTECT_DELETE;
    return prot;
}

void spawnSystemCallsNew(ObjectPtr global,
                         ObjectPtr method,
                         ObjectPtr sys,
                         ReadOnlyState& reader) {

    using namespace Table;

    TranslationUnitPtr unit = make_shared<TranslationUnit>();

    // CPP_TERMINATE
    assert(reader.cpp.size() == CPP_TERMINATE);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        // A last-resort termination of a fiber that malfunctioned; this should ONLY
        // be used as a last resort, as it does not correctly unwind the frames
        // before aborting
        hardKill(state0, reader0);
    });

    // CPP_KERNEL_LOAD ($1 = filename, $2 = global)
    //  * Checks %num0 (if 0, then standard load; if 1, then raw load; if 2, then compile only)
    // kernelLoad#: filename, global.
    // kernelLoad0#: filename, global.
    assert(reader.cpp.size() == CPP_KERNEL_LOAD);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr str = (*dyn)[ Symbols::get()["$1"] ];
        ObjectPtr global = (*dyn)[ Symbols::get()["$2"] ];
        OperatorTable table = getTable(state0.lex.top());
        if ((str != nullptr) && (global != nullptr)) {
            auto str0 = boost::get<string>(&str->prim());
            if (str0) {
                string str1 = *str0;
                if (state0.num0.asSmallInt() == 2) {
                    compileFile(str1, str1 + "c", state0, reader0, table);
                } else {
                    if (state0.num0.asSmallInt() == 1)
                        str1 = stripFilename(getExecutablePathname()) + str1;
                    readFile(str1, { global, global }, state0, reader0, table);
                }
            } else {
                throwError(state0, reader0, "TypeError", "String expected");
            }
        } else {
            throwError(state0, reader0, "SystemArgError", "Wrong number of arguments");
        }
    });
    sys->put(Symbols::get()["kernelLoad#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 0),
                                  makeAssemblerLine(Instr::CPP, CPP_KERNEL_LOAD))));
    sys->put(Symbols::get()["kernelLoad0#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 1),
                                  makeAssemblerLine(Instr::CPP, CPP_KERNEL_LOAD))));
    sys->put(Symbols::get()["kernelComp#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 2),
                                  makeAssemblerLine(Instr::CPP, CPP_KERNEL_LOAD))));

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

    // CPP_STREAM_DIR ($1 = argument) (where %num0 specifies the direction; 0 = in, 1 = out)
    // streamIn#: stream.
    // streamOut#: stream.
    assert(reader.cpp.size() == CPP_STREAM_DIR);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn)[ Symbols::get()["$1"] ];
        if (stream != nullptr) {
            auto stream0 = boost::get<StreamPtr>(&stream->prim());
            if (stream0) {
                switch (state0.num0.asSmallInt()) {
                case 0:
                    state0.ret = garnishObject(reader0, (*stream0)->hasIn());
                    break;
                case 1:
                    state0.ret = garnishObject(reader0, (*stream0)->hasOut());
                    break;
                }
            } else {
                throwError(state0, reader0, "TypeError", "Stream expected");
            }
        } else {
            throwError(state0, reader0, "SystemArgError", "Wrong number of arguments");
        }
    });
    sys->put(Symbols::get()["streamIn#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 0L),
                                         makeAssemblerLine(Instr::CPP, CPP_STREAM_DIR))));
    sys->put(Symbols::get()["streamOut#"],
                    defineMethod(unit, global, method,
                                 asmCode(makeAssemblerLine(Instr::INT, 1L),
                                         makeAssemblerLine(Instr::CPP, CPP_STREAM_DIR))));

    // CPP_STREAM_PUT ($1 = stream, $2 = string) (where %num0 specifies whether a newline is added; 0 = no, 1 = yes)
    // streamPuts#: stream, str.
    // streamPutln#: stream, str.
    assert(reader.cpp.size() == CPP_STREAM_PUT);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn)[ Symbols::get()["$1"] ];
        ObjectPtr str = (*dyn)[ Symbols::get()["$2"] ];
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
                    state0.ret = garnishObject(reader0, boost::blank());
                } else {
                    throwError(state0, reader0, "IOError", "Stream not designated for output");
                }
            } else {
                throwError(state0, reader0, "SystemArgError", "Invalid argument to output function");
            }
        } else {
            throwError(state0, reader0, "SystemArgError", "Wrong number of arguments");
        }
    });
    sys->put(Symbols::get()["streamPuts#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 0L),
                                  makeAssemblerLine(Instr::CPP, CPP_STREAM_PUT))));
    sys->put(Symbols::get()["streamPutln#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 1L),
                                  makeAssemblerLine(Instr::CPP, CPP_STREAM_PUT))));

    // CPP_TO_STRING (where %num0 specifies which register to use)
    //   0 = %num1
    //   1 = %str0
    //   2 = %sym
    // numToString#: num
    // strToString#: str
    // symToString#: sym
    assert(reader.cpp.size() == CPP_TO_STRING);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
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
                    oss << "\\n";
                else if (ch == '\r')
                    oss << "\\r";
                else if (ch == '\a')
                    oss << "\\a";
                else if (ch == '\b')
                    oss << "\\b";
                else if (ch == '\v')
                    oss << "\\v";
                else if (ch == '\t')
                    oss << "\\t";
                else if (ch == '\f')
                    oss << "\\f";
                else if (ch == '\0')
                    oss << "\\0";
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
                else if (ch == '\n')
                    oss << "\\n";
                else if (ch == '\r')
                    oss << "\\r";
                else if (ch == '\a')
                    oss << "\\a";
                else if (ch == '\b')
                    oss << "\\b";
                else if (ch == '\v')
                    oss << "\\v";
                else if (ch == '\t')
                    oss << "\\t";
                else if (ch == '\f')
                    oss << "\\f";
                else if (ch == '\0')
                    oss << "\\0";
                    else
                        oss << ch;
                }
                oss << ")";
            } else {
                if (Symbols::symbolType(state0.sym) != SymbolType::GENERATED)
                    oss << '\'';
                oss << str;
            }
        }
            break;
        }
        state0.ret = garnishObject(reader0, oss.str());
    });
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
                                  makeAssemblerLine(Instr::CPP, CPP_TO_STRING))));
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
                                  makeAssemblerLine(Instr::CPP, CPP_TO_STRING))));
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
                                  makeAssemblerLine(Instr::CPP, CPP_TO_STRING))));

    // CPP_GENSYM (if %num0 is 1 then use %str0 as prefix, else if %num0 is 0 use default prefix; store in %sym)
    // gensym#: self.
    // gensymOf#: self, prefix.
    assert(reader.cpp.size() == CPP_GENSYM);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        switch (state0.num0.asSmallInt()) {
        case 0:
        state0.sym = Symbols::gensym();
        break;
        case 1:
        state0.sym = Symbols::gensym(state0.str0);
        break;
        }
    });
    sys->put(Symbols::get()["gensym#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::INT, 0L),
                                  makeAssemblerLine(Instr::CPP, CPP_GENSYM),
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
                                  makeAssemblerLine(Instr::CPP, CPP_GENSYM),
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
                                       makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                       makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                       makeAssemblerLine(Instr::RTRV),
                                       makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                       makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                       makeAssemblerLine(Instr::CALL, 0L))));
    sys->put(Symbols::get()["_onFalse#"],
             defineMethodNoRet(unit, global, method,
                               asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                       makeAssemblerLine(Instr::SYMN, Symbols::get()["$3"].index),
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
                                  makeAssemblerLine(Instr::YLD, Lit::TRUE, Reg::PTR),
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

    // CPP_INSTANCE_OF (check if %slf is an instance of %ptr, put result in %flag)
    // instanceOf#: obj, anc
    assert(reader.cpp.size() == CPP_INSTANCE_OF);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        auto hier = hierarchy(state0.slf);
        state0.flag = (find_if(hier.begin(), hier.end(),
                               [&state0](auto& o){ return o == state0.ptr; }) != hier.end());
    });
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
                                  makeAssemblerLine(Instr::CPP, CPP_INSTANCE_OF),
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
                          asmCode(makeAssemblerLine(Instr::CPP, Table::CPP_TERMINATE))));

    // CPP_STREAM_READ ($1 = stream) (constructs and stores the resulting string in %ret, uses %num0 for mode)
    // - 0 - Read a line
    // - 1 - Read a single character
    // - 2 - (Ignores the stream) Special read; using REPL history, etc
    // streamRead#: stream.
    // streamReadChar#: stream.
    // streamReadSpec#: stream.
    assert(reader.cpp.size() == CPP_STREAM_READ);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        ObjectPtr dyn = state0.dyn.top();
        ObjectPtr stream = (*dyn)[ Symbols::get()["$1"] ];
        if (state0.num0.asSmallInt() == 2) {
            // Special read
            state0.ret = garnishObject(reader0, ReadLine::readRich());
        } else {
            // Normal read
            if (stream != NULL) {
                auto stream0 = boost::get<StreamPtr>(&stream->prim());
                if (stream0) {
                    if ((*stream0)->hasIn()) {
                        if (state0.num0.asSmallInt() == 0)
                            state0.ret = garnishObject(reader0, (*stream0)->readLine());
                        else
                            state0.ret = garnishObject(reader0, (*stream0)->readText(1));
                    } else {
                        throwError(state0, reader0, "IOError", "Stream not designated for input");
                    }
                } else {
                    throwError(state0, reader0, "TypeError", "Stream expected");
                }
            }
        }
    });
    sys->put(Symbols::get()["streamRead#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 0L),
                                  makeAssemblerLine(Instr::CPP, CPP_STREAM_READ))));
    sys->put(Symbols::get()["streamReadChar#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 1L),
                                  makeAssemblerLine(Instr::CPP, CPP_STREAM_READ))));
    sys->put(Symbols::get()["streamReadSpec#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::INT, 2L),
                                  makeAssemblerLine(Instr::CPP, CPP_STREAM_READ))));

    // CPP_EVAL (where %str0 is a string to evaluate; throws if something goes wrong)
    // eval#: lex, dyn, str.
    assert(reader.cpp.size() == CPP_EVAL);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        OperatorTable table = getTable(state0.lex.top());
        eval(state0, reader0, table, state0.str0);
    });
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
                                  makeAssemblerLine(Instr::CPP, CPP_EVAL),
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
    // numAnd#: n1, n2.
    // numIor#: n1, n2.
    // numEor#: n1, n2.
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
    sys->put(Symbols::get()["numAnd#"],
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
                                  makeAssemblerLine(Instr::ARITH, 7L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["numIor#"],
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
                                  makeAssemblerLine(Instr::ARITH, 8L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
    sys->put(Symbols::get()["numEor#"],
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
                                  makeAssemblerLine(Instr::ARITH, 9L),
                                  makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                  makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                  makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

    // CPP_SYM_NAME (takes %sym, looks up its name, and outputs a string as %ret)
    // symName#: sym.
    assert(reader.cpp.size() == CPP_SYM_NAME);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        std::string name = Symbols::get()[ state0.sym ];
        state0.ret = garnishObject(reader0, name);
    });
    sys->put(Symbols::get()["symName#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                  makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                  makeAssemblerLine(Instr::CPP, CPP_SYM_NAME))));

    // CPP_SYM_NUM (takes %num0 and outputs an appropriate symbol to %ret)
    // natSym#: num.
    assert(reader.cpp.size() == CPP_SYM_NUM);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        if (state0.num0.hierarchyLevel() > 1) {
            throwError(state0, reader0, "TypeError", "Cannot produce symbols from non-integers");
        } else if (state0.num0 > Number(2147483647L)) {
            throwError(state0, reader0, "TypeError", "Cannot produce symbols from large integers");
        } else if (state0.num0.asSmallInt() < 0) {
            throwError(state0, reader0, "TypeError", "Cannot produce symbols from negative numbers");
        } else {
            Symbolic sym = Symbols::natural((int)state0.num0.asSmallInt());
            state0.ret = garnishObject(reader0, sym);
        }
    });
    sys->put(Symbols::get()["natSym#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::CPP, CPP_SYM_NUM))));

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

    // CPP_SYM_INTERN (takes %str0, looks it up, and puts the result as a symbol in %ret)
    // intern#: str.
    assert(reader.cpp.size() == CPP_SYM_INTERN);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        Symbolic name = Symbols::get()[ state0.str0 ];
        state0.ret = garnishObject(reader0, name);
    });
    sys->put(Symbols::get()["intern#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                  makeAssemblerLine(Instr::THROA, "String expected"),
                                  makeAssemblerLine(Instr::CPP, CPP_SYM_INTERN))));

    // CPP_SIMPLE_CMP (compares %slf's and %ptr's respective prim fields based on the value of %num0)
    // - 0 - Compare for equality and put the result in %flag
    // - 1 - Compare for LT and put the result in %flag
    // In any case, if either argument lacks a prim or the prim fields have different types, false
    // is returned by default.
    // SIMPLE_CMP will compare strings, numbers, and symbols. Anything else returns false.
    // primEquals#: lhs, rhs.
    // primLT#: lhs, rhs.
    assert(reader.cpp.size() == CPP_SIMPLE_CMP);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
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
    });
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
                                  makeAssemblerLine(Instr::CPP, CPP_SIMPLE_CMP),
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
                                  makeAssemblerLine(Instr::CPP, CPP_SIMPLE_CMP),
                                  makeAssemblerLine(Instr::BOL))));

    // CPP_NUM_LEVEL (determine the "level" of %num0 and put the result in %ret)
    // numLevel#: num.
    assert(reader.cpp.size() == CPP_NUM_LEVEL);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        state0.ret = garnishObject(reader0, state0.num0.hierarchyLevel());
    });
    sys->put(Symbols::get()["numLevel#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                  makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                  makeAssemblerLine(Instr::RTRV),
                                  makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                  makeAssemblerLine(Instr::ECLR),
                                  makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                  makeAssemblerLine(Instr::THROA, "Number expected"),
                                  makeAssemblerLine(Instr::CPP, CPP_NUM_LEVEL))));

    // stackTrace#.
    // stackTrace# only includes %trace, not %line nor %file. %line and %file are often
    // undesired in this case as they represent only the line and file of the stackTrace#
    // call, which is bogus, as stackTrace# is not defined in a file.
    sys->put(Symbols::get()["stackTrace#"],
             defineMethod(unit, global, method,
                          asmCode(makeAssemblerLine(Instr::LOCRT))));

    // CPP_ORIGIN (find the origin of %sym in %slf, store resulting object in %ret, throw SlotError otherwise
    // origin#: self, sym.
    assert(reader.cpp.size() == CPP_ORIGIN);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        ObjectPtr value = origin(state0.slf, state0.sym);
        if (value == nullptr) {
            throwError(state0, reader0, "SlotError", "Cannot find origin of nonexistent slot");
        } else {
            state0.ret = value;
        }
    });
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
                                  makeAssemblerLine(Instr::CPP, CPP_ORIGIN))));

    // CPP_PROCESS_TASK - Do something with %slf and possibly other registers, based on %num0
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
    assert(reader.cpp.size() == CPP_PROCESS_TASK);
    reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        switch (state0.num0.asSmallInt()) {
        case 0: {
            ProcessPtr proc = makeProcess(state0.str0);
            if (!proc)
                throwError(state0, reader0, "NotSupportedError",
                           "Asynchronous processes not supported on this system");
            state0.ret = clone(state0.slf);
            state0.ret->prim(proc);
            break;
        }
        case 1: {
            bool status = state0.prcs->run();
            if (!status)
                throwError(state0, reader0, "IOError",
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
            state0.ret = garnishObject(reader0, state0.prcs->isRunning());
            break;
        }
        case 6: {
            state0.ret = garnishObject(reader0, state0.prcs->getExitCode());
            break;
        }
        case 7: {
            state0.ret = garnishObject(reader0, state0.prcs->isDone());
            break;
        }
        }
    });
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
                                  makeAssemblerLine(Instr::CPP, CPP_PROCESS_TASK),
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
                                           makeAssemblerLine(Instr::CPP, CPP_PROCESS_TASK),
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
                                   makeAssemblerLine(Instr::CPP, CPP_PROCESS_TASK),
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
                                   makeAssemblerLine(Instr::CPP, CPP_PROCESS_TASK))));
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
                                   makeAssemblerLine(Instr::CPP, CPP_PROCESS_TASK))));
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
                                   makeAssemblerLine(Instr::CPP, CPP_PROCESS_TASK))));
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
                                   makeAssemblerLine(Instr::CPP, CPP_PROCESS_TASK))));
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
                                   makeAssemblerLine(Instr::CPP, CPP_PROCESS_TASK))));

     // CPP_OBJECT_KEYS (takes an object in %slf and a method on %sto and calls the method
     //                  for each key in the object)
     // objectKeys#: obj.
     assert(reader.cpp.size() == CPP_OBJECT_KEYS);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         set<Symbolic> allKeys = state0.slf->directKeys();
         state0.stack = pushNode(state0.stack, state0.cont);
         state0.cont = MethodSeek(Method(reader0.gtu, { GTU_KEY_TERM }));
         for (auto& key : allKeys) {
             ObjectPtr obj = clone(reader0.lit.at(Lit::SYMBOL));
             obj->prim(key);
             state0.arg.push(obj);
             state0.stack = pushNode(state0.stack, state0.cont);
             state0.cont = MethodSeek(Method(reader0.gtu, { GTU_KEYS }));
         }
     });
     sys->put(Symbols::get()["objectKeys#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                   makeAssemblerLine(Instr::CPP, CPP_OBJECT_KEYS))));

     // CPP_FILE_DOOPEN (%str0 is the filename, %ptr is a stream object to be filled, $3 is access and mode)
     // streamFileOpen#: strm, fname, access, mode.
     assert(reader.cpp.size() == CPP_FILE_DOOPEN);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         ObjectPtr dyn = state0.dyn.top();
         ObjectPtr access = (*dyn)[ Symbols::get()["$3"] ];
         auto access0 = boost::get<std::string>(&access->prim());
         if (access0) {
             bool okay = true;
             if (*access0 == "")
                 okay = false;
             FileAccess access2;
             FileMode mode2;
             if (okay) {
                 if ((*access0)[0] == 'r')
                     access2 = FileAccess::READ;
                 else if ((*access0)[0] == 'w')
                     access2 = FileAccess::WRITE;
                 else
                     okay = false;
                 if ((access0->length() < 2) || ((*access0)[0] != 'b'))
                     mode2 = FileMode::TEXT;
                 else
                     mode2 = FileMode::BINARY;
             }
             if (okay)
                 state0.ptr->prim( StreamPtr(new FileStream(state0.str0, access2, mode2)) );
             else
                 throwError(state0, reader0, "SystemArgError",
                            "Invalid mode/access specifier when opening file");
         } else {
             throwError(state0, reader0, "TypeError",
                        "Symbol expected");
         }
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_FILE_DOOPEN),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // CPP_FILE_DOCLOSE (takes %strm and closes it)
     // streamClose#: strm.
     assert(reader.cpp.size() == CPP_FILE_DOCLOSE);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.strm->close();
     });
     sys->put(Symbols::get()["streamClose#"],
              defineMethod(unit, global, method,
                                   asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                           makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                           makeAssemblerLine(Instr::RTRV),
                                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                           makeAssemblerLine(Instr::ECLR),
                                           makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                           makeAssemblerLine(Instr::THROA, "Stream expected"),
                                           makeAssemblerLine(Instr::CPP, CPP_FILE_DOCLOSE),
                                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // CPP_FILE_EOF (takes %strm and outputs whether it's at eof into %flag)
     // streamEof#: strm.
     assert(reader.cpp.size() == CPP_FILE_EOF);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.flag = state0.strm->isEof();
     });
     sys->put(Symbols::get()["streamEof#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                   makeAssemblerLine(Instr::THROA, "Stream expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_FILE_EOF),
                                   makeAssemblerLine(Instr::BOL))));

     // CPP_STRING_LENGTH (outputs length of %str0 into %ret)
     // stringLength#: str.
     assert(reader.cpp.size() == CPP_STRING_LENGTH);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.ret = garnishObject(reader0, (long)state0.str0.length());
     });
     sys->put(Symbols::get()["stringLength#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_STRING_LENGTH))));

     // CPP_STRING_SUB (outputs substring of %str0 from %num0 to %num1 into %ret)
     // stringSubstring#: str, beg, end.
     assert(reader.cpp.size() == CPP_STRING_SUB);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
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
         state0.ret = garnishObject(reader0, state0.str0.substr(start1, len));
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_STRING_SUB))));

     // CPP_STRING_FIND (find first occurence of %str1 in %str0 starting at %num0 index, storing
     //              new index or Nil in %ret)
     // stringFindFirst#: str, substr, pos.
     assert(reader.cpp.size() == CPP_STRING_FIND);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         auto pos = state0.str0.find(state0.str1, state0.num0.asSmallInt());
         if (pos == string::npos)
             state0.ret = garnishObject(reader0, boost::blank());
         else
             state0.ret = garnishObject(reader0, (long)pos);
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_STRING_FIND))));

     // CPP_GC_RUN (run the garbage collector and store the number of objects deleted at %ret)
     // runGC#.
     assert(reader.cpp.size() == CPP_GC_RUN);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         long result = GC::get().garbageCollect(state0, reader0);
         state0.ret = garnishObject(reader0, result);
     });
     sys->put(Symbols::get()["runGC#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::CPP, CPP_GC_RUN))));

     // CPP_FILE_HEADER (check the %str0 file and put a FileHeader object in %ret)
     // fileHeader#: filename.
     assert(reader.cpp.size() == CPP_FILE_HEADER);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         ObjectPtr obj = clone(reader0.lit.at(Lit::FHEAD));
         Header header = getFileHeader(state0.str0);
         if (header.fields & (unsigned int)HeaderField::MODULE) {
             obj->put(Symbols::get()["moduleName"], garnishObject(reader0, header.module));
         }
         if (header.fields & (unsigned int)HeaderField::PACKAGE) {
             obj->put(Symbols::get()["packageName"], garnishObject(reader0, header.package));
         }
         state0.ret = obj;
     });
     sys->put(Symbols::get()["fileHeader#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_FILE_HEADER))));

     // CPP_STR_ORD (check the %str0 register and put the ASCII value in %ret, empty string returns 0)
     // CPP_STR_CHR (check the %num0 register and put the character in %ret)
     // strOrd#: str.
     // strChr#: num.
     assert(reader.cpp.size() == CPP_STR_ORD);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         if (state0.str0 == "")
             state0.ret = garnishObject(reader0, 0);
         else
             state0.ret = garnishObject(reader0, (int)state0.str0[0]);
     });
     assert(reader.cpp.size() == CPP_STR_CHR);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.ret = garnishObject(reader0, string(1, (char)state0.num0.asSmallInt()));
     });
     sys->put(Symbols::get()["strOrd#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_STR_ORD))));
     sys->put(Symbols::get()["strChr#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_STR_CHR))));

     // CPP_GC_TOTAL (get the total number of allocated objects from the garbage collector and store it in %ret if %num0 is 0, limit of GC if %num0 is 1)
     // totalGC#.
     // limitGC#.
     assert(reader.cpp.size() == CPP_GC_TOTAL);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         if (state0.num0.asSmallInt() == 0)
             state0.ret = garnishObject(reader0, (long)GC::get().getTotal());
         else if (state0.num0.asSmallInt() == 1)
             state0.ret = garnishObject(reader0, (long)GC::get().getLimit());
     });
     sys->put(Symbols::get()["totalGC#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::INT, 0L),
                                   makeAssemblerLine(Instr::CPP, CPP_GC_TOTAL))));
     sys->put(Symbols::get()["limitGC#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::INT, 1L),
                                   makeAssemblerLine(Instr::CPP, CPP_GC_TOTAL))));

     // CPP_TIME_SPAWN (put all the information about the current system time in the %ptr object, using %num0 to
     //             determine whether local time (1) or global time (2))
     // timeSpawnLocal#: obj.
     // timeSpawnGlobal#: obj.
     assert(reader.cpp.size() == CPP_TIME_SPAWN);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         time_t raw;
         tm info;
         time(&raw);
         if (state0.num0.asSmallInt() == 1)
             info = *localtime(&raw);
         else if (state0.num0.asSmallInt() == 2)
             info = *gmtime(&raw);
         else
             assert(false);
         Number base = Number::bigint(raw);
         state0.ptr->put(Symbols::get()["second"], garnishObject(reader0, info.tm_sec));
         state0.ptr->put(Symbols::get()["minute"], garnishObject(reader0, info.tm_min));
         state0.ptr->put(Symbols::get()["hour"], garnishObject(reader0, info.tm_hour));
         state0.ptr->put(Symbols::get()["day"], garnishObject(reader0, info.tm_mday));
         state0.ptr->put(Symbols::get()["monthNumber"], garnishObject(reader0, info.tm_mon));
         state0.ptr->put(Symbols::get()["year"], garnishObject(reader0, info.tm_year + 1900));
         state0.ptr->put(Symbols::get()["weekdayNumber"], garnishObject(reader0, info.tm_wday));
         state0.ptr->put(Symbols::get()["yearDay"], garnishObject(reader0, info.tm_yday + 1));
         state0.ptr->put(Symbols::get()["dstNumber"], garnishObject(reader0, info.tm_isdst));
         state0.ptr->put(Symbols::get()["raw"], garnishObject(reader0, base));
     });
     sys->put(Symbols::get()["timeSpawnLocal#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, CPP_TIME_SPAWN),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));
     sys->put(Symbols::get()["timeSpawnGlobal#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::INT, 2),
                                   makeAssemblerLine(Instr::CPP, CPP_TIME_SPAWN),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // CPP_ENV_GET (retrieve the environment variable matching the name in %str0 and store the result in %ret)
     // envGet#: str.
     assert(reader.cpp.size() == CPP_ENV_GET);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         boost::optional<std::string> value = getEnv(state0.str0);
         if (value)
             state0.ret = garnishObject(reader0, *value);
         else
             state0.ret = garnishObject(reader0, boost::blank());
     });
     sys->put(Symbols::get()["envGet#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_ENV_GET))));

     // CPP_ENV_SET (assign the environment variable with name %str0 to value %str1 (or unset it, if %num0 is nonzero)
     // envSet#: name, value.
     // envUnset#: name.
     assert(reader.cpp.size() == CPP_ENV_SET);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         bool success = false;
         if (state0.num0.asSmallInt() != 0) {
             success = unsetEnv(state0.str0);
         } else {
             success = setEnv(state0.str0, state0.str1);
         }
         if (success)
             state0.ret = garnishObject(reader0, boost::blank());
         else
             throwError(state0, reader0, "NotSupportedError",
                        "Mutable environment variables not supported on this system");
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_ENV_SET))));
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
                                   makeAssemblerLine(Instr::CPP, CPP_ENV_SET))));

     // CPP_EXE_PATH (put an appropriate pathname into %ret%, given by the %num0 argument)
     //  * %num0 == 1: Executable pathname
     //  & %num0 == 2: Current working directory
     // exePath#.
     // cwdPath#.
     assert(reader.cpp.size() == CPP_EXE_PATH);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         switch (state0.num0.asSmallInt()) {
         case 1:
             state0.ret = garnishObject(reader0, getExecutablePathname());
             break;
         case 2:
             state0.ret = garnishObject(reader0, getWorkingDirectory());
             break;
         default:
             throwError(state0, reader0, "SystemArgError",
                        "Invalid numerical argument to EXE_PATH");
             break;
         }
     });
     sys->put(Symbols::get()["exePath#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, CPP_EXE_PATH))));
     sys->put(Symbols::get()["cwdPath#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::INT, 2),
                                   makeAssemblerLine(Instr::CPP, CPP_EXE_PATH))));

     // CPP_PATH_OP (put an appropriate pathname into %ret%, given by the %num0 argument and %str0 input)
     //  * %num0 == 1: Get directory of pathname
     //  * %num0 == 2: Get filename of pathname
     // dirName#: str.
     assert(reader.cpp.size() == CPP_PATH_OP);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         switch (state0.num0.asSmallInt()) {
         case 1:
             state0.ret = garnishObject(reader0, stripFilename(state0.str0));
         break;
         case 2:
             state0.ret = garnishObject(reader0, stripDirname(state0.str0));
             break;
         default:
             throwError(state0, reader0, "SystemArgError",
                        "Invalid numerical argument to PATH_OP");
             break;
         }
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_PATH_OP))));
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
                                   makeAssemblerLine(Instr::CPP, CPP_PATH_OP))));

     // CPP_FILE_EXISTS (check the pathname in %str0 for existence, storing result in %flag)
     // fileExists#: fname.
     assert(reader.cpp.size() == CPP_FILE_EXISTS);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         std::ifstream f(state0.str0.c_str());
         state0.flag = f.good();
         f.close();
     });
     sys->put(Symbols::get()["fileExists#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_FILE_EXISTS),
                                   makeAssemblerLine(Instr::BOL))));

     // CPP_TRIG_OP (perform the operation indicated in %num0 on the value in %num1, storing result in %num1)
     // numTrig#: num, op.
     assert(reader.cpp.size() == CPP_TRIG_OP);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
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
             throwError(state0, reader0, "SystemArgError",
                        "Invalid numerical argument to TRIG_OP");
             break;
         }
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_TRIG_OP),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM1),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // CPP_MATH_FLOOR (floor the value in %num0, storing result in %num0)
     // numFloor#: num.
     assert(reader.cpp.size() == CPP_MATH_FLOOR);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.num0 = state0.num0.floor();
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_MATH_FLOOR),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // CPP_NUM_CONST (store something in %num1, based on the value of %num0, sets %err0 if non-applicable)
     // numNan#: number.
     // numInfinity#: number.
     // numNegInfinity#: number.
     // numEpsilon#: number.
     assert(reader.cpp.size() == CPP_NUM_CONST);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
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
     });
     sys->put(Symbols::get()["numNan#"],
              defineMethod(unit, global, method,
                   asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                           makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::INT, 0),
                           makeAssemblerLine(Instr::ECLR),
                           makeAssemblerLine(Instr::CPP, CPP_NUM_CONST),
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
                                   makeAssemblerLine(Instr::CPP, CPP_NUM_CONST),
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
                                   makeAssemblerLine(Instr::CPP, CPP_NUM_CONST),
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
                                   makeAssemblerLine(Instr::CPP, CPP_NUM_CONST),
                                   makeAssemblerLine(Instr::THROA, "Invalid constant"),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM1),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // CPP_PROT_VAR (protect the variable named %sym in the object %slf using the protection mask %num0)
     // protectVar#: obj, var, level.
     assert(reader.cpp.size() == CPP_PROT_VAR);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         long num = state0.num0.asSmallInt();
         Protection prot = numToProtection(num);
         bool result = state0.slf->addProtection(state0.sym, prot);
         if (!result) {
             ObjectPtr err = reader0.lit[Lit::ERR];
             ObjectPtr slot = (*err)[Symbols::get()["SlotError"]];
             ObjectPtr curr = clone(slot);
             curr->put(Symbols::get()["objectInstance"], state0.slf);
             curr->put(Symbols::get()["slotName"], garnishObject(reader0, state0.sym));
             throwError(state0, reader0, curr);
         }
     });
     sys->put(Symbols::get()["protectVar#"],
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
                                   makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                   makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                   makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                   makeAssemblerLine(Instr::CPP, CPP_PROT_VAR),
                                   makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET))));

     // CPP_PROT_IS (check protection of the variable named %sym in the object %slf, returning %ret)
     // - %num0 (0) - Check for *any* protections
     // - %num0 (1) - Check specifically for %num1 protections
     // protectIs#: obj, var.
     // protectIsThis#: obj, var, prot.
     assert(reader.cpp.size() == CPP_PROT_IS);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         long param = state0.num0.asSmallInt();
         switch (param) {
         case 0:
             state0.ret = garnishObject(reader0, state0.slf->hasAnyProtection(state0.sym));
             break;
         case 1:
             {
                 Protection prot = numToProtection(state0.num1.asSmallInt());
                 state0.ret = garnishObject(reader0, state0.slf->isProtected(state0.sym, prot));
                 break;
             }
         }
     });
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
                                   makeAssemblerLine(Instr::INT, 0),
                                   makeAssemblerLine(Instr::CPP, CPP_PROT_IS))));
     sys->put(Symbols::get()["protectIsThis#"],
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
                                   makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                   makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                   makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                   makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, CPP_PROT_IS))));

     // CPP_STR_NEXT (given %str0 and %num0, put next byte index in %ret, or Nil on failure)
     // stringNext#: str, num.
     assert(reader.cpp.size() == CPP_STR_NEXT);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         auto result = nextCharPos(state0.str0, state0.num0.asSmallInt());
         if (result)
             state0.ret = garnishObject(reader0, *result);
         else
             state0.ret = garnishObject(reader0, boost::blank());
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_STR_NEXT))));

     // CPP_COMPLEX (take %num0 as real part, %num1 as imaginary part, and produce complex number in %num0)
     // complexNumber#: number, re, im.
     assert(reader.cpp.size() == CPP_COMPLEX);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.num0 = complexNumber(state0.num0, state0.num1);
     });
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
                                   makeAssemblerLine(Instr::CPP, CPP_COMPLEX),
                                   makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                   makeAssemblerLine(Instr::LOAD, Reg::NUM0),
                                   makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET))));

     // remSlot#: obj, sym.
     sys->put(Symbols::get()["remSlot#"],
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
                                   makeAssemblerLine(Instr::DEL),
                                   makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET))));

     // CPP_PRIM_METHOD (takes %slf and puts whether or not it has a method prim in %flag)
     // primIsMethod#: obj.
     assert(reader.cpp.size() == CPP_PRIM_METHOD);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.flag = variantIsType<Method>(state0.slf->prim());
     });
     sys->put(Symbols::get()["primIsMethod#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                   makeAssemblerLine(Instr::CPP, CPP_PRIM_METHOD),
                                   makeAssemblerLine(Instr::BOL))));

     // CPP_LOOP_DO (takes %ptr, calls it, then does LOOP_DO again)
     // loopDo#: method.
     assert(reader.cpp.size() == CPP_LOOP_DO);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.stack = pushNode(state0.stack, state0.cont);
         state0.cont = MethodSeek(Method(reader0.gtu, { GTU_LOOP_DO }));
     });
     sys->put(Symbols::get()["loopDo#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::CPP, CPP_LOOP_DO))));

     // CPP_UNI_ORD (check the %str0 register and put the code point in %ret, empty string returns 0)
     // CPP_UNI_CHR (check the %num0 register and put the character in %ret)
     // uniOrd#: str.
     // uniChr#: num.
     assert(reader.cpp.size() == CPP_UNI_ORD);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         auto ch = charAt(state0.str0, 0L);
         if (ch)
             state0.ret = garnishObject(reader0, uniOrd(*ch));
         else
             state0.ret = garnishObject(reader0, 0L);
     });
     assert(reader.cpp.size() == CPP_UNI_CHR);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.ret = garnishObject(reader0,
                                    static_cast<std::string>(UniChar(state0.num0.asSmallInt())));
     });
     sys->put(Symbols::get()["uniOrd#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_UNI_ORD))));
     sys->put(Symbols::get()["uniChr#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM0),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_UNI_CHR))));

     // CPP_OSINFO (store info about the OS in %ptr)
     // osInfo#: obj.
     assert(reader.cpp.size() == CPP_OSINFO);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         Symbolic sym = Symbols::get()[""];
         switch (systemOS) {
         case OS::WINDOWS:
             sym = Symbols::get()["windows"];
             break;
         case OS::POSIX:
             sym = Symbols::get()["posix"];
             break;
         case OS::UNKNOWN:
             sym = Symbols::get()["unknown"];
             break;
         }
         ObjectPtr symbolObj = clone(reader0.lit.at(Lit::SYMBOL));
         symbolObj->prim(sym);
         state0.ptr->put(Symbols::get()["class"], symbolObj);
     });
     sys->put(Symbols::get()["osInfo#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   // No need to move back, since %ptr == %ret now
                                   makeAssemblerLine(Instr::CPP, CPP_OSINFO))));

     // CPP_COMPLPARTS (store into %ret the appropriate part of %num1)
     //  * %num0 == 1; store real part
     //  * %num0 == 2; store imag part
     // realPart#: num.
     // imagPart#: num.
     assert(reader.cpp.size() == CPP_COMPLPARTS);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         switch (state0.num0.asSmallInt()) {
         case 1:
             state0.ret = garnishObject(reader0, state0.num1.realPart());
             break;
         case 2:
             state0.ret = garnishObject(reader0, state0.num1.imagPart());
             break;
         default:
             throwError(state0, reader0, "SystemArgError",
                        "Invalid numerical argument to EXE_PATH");
             break;
         }
     });
     sys->put(Symbols::get()["realPart#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, CPP_COMPLPARTS))));
     sys->put(Symbols::get()["imagPart#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::NUM1),
                                   makeAssemblerLine(Instr::THROA, "Number expected"),
                                   makeAssemblerLine(Instr::INT, 2),
                                   makeAssemblerLine(Instr::CPP, CPP_COMPLPARTS))));

     // CPP_OBJID (store in %ret the identifier of %slf)
     // objId#: obj.
     assert(reader.cpp.size() == CPP_OBJID);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         auto value = (intptr_t)(state0.ret.get());
         auto value1 = Number::bigint(value);
         state0.ret = garnishObject(reader0, Number(value1));
     });
     sys->put(Symbols::get()["objId#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                   makeAssemblerLine(Instr::CPP, CPP_OBJID))));

     // CPP_STREAM_FLUSH (flush the stream in %strm)
     // streamFlush#: stream.
     assert(reader.cpp.size() == CPP_STREAM_FLUSH);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.strm->flush();
     });
     sys->put(Symbols::get()["streamFlush#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STRM),
                                   makeAssemblerLine(Instr::THROA, "Stream expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_STREAM_FLUSH))));

     // CPP_UNI_CAT (put the category of the string in %str0 into %ret as a number object)
     // uniCat#: str.
     assert(reader.cpp.size() == CPP_UNI_CAT);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         auto ch = charAt(state0.str0, 0L);
         if (ch)
             state0.ret = garnishObject(reader0, static_cast<long>(ch->genCat()));
         else
             state0.ret = garnishObject(reader0, 0L);
     });
     sys->put(Symbols::get()["uniCat#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_UNI_CAT))));

     // CPP_GC_TRACE (set the tracing attribute of the garbage collector to %num0)
     // traceGC#.
     // untraceGC#.
     assert(reader.cpp.size() == CPP_GC_TRACE);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
          auto val = state0.num0.asSmallInt();
          GC::get().setTracing(val != 0);
     });
     sys->put(Symbols::get()["traceGC#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, CPP_GC_TRACE))));
     sys->put(Symbols::get()["untraceGC#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::INT, 0),
                                   makeAssemblerLine(Instr::CPP, CPP_GC_TRACE))));

     // CPP_PARSE_DOUBLE (parse %str0 as a double, returning in %ret; throws if no valid conversion could be made)
     // strToDouble#: value.
     assert(reader.cpp.size() == CPP_PARSE_DOUBLE);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         const char* start = state0.str0.c_str();
         char* end = nullptr;
         double x = strtod(start, &end);
         if (end > start) {
             state0.ret = garnishObject(reader0, Number(x));
         } else {
             throwError(state0, reader0, "InputError", "Text is not a number");
         }
     });
     sys->put(Symbols::get()["strToDouble#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::CPP, CPP_PARSE_DOUBLE))));

     // CPP_DUPLICATE (replace the object pointed to at %ptr with one identical to that of %slf)
     // duplicate#: obj.
     assert(reader.cpp.size() == CPP_DUPLICATE);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         *(state0.ptr) = *(state0.slf);
     });
     sys->put(Symbols::get()["duplicate#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                   makeAssemblerLine(Instr::CLONE),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::CPP, CPP_DUPLICATE))));

     // CPP_UNI_CASE (convert the first character of %str0 to the given case, returning ONLY THIS CHARACTER into %ret, based on the value of %num0)
     // - 0 - Lower
     // - 1 - Upper
     // - 2 - Title
     // strLower#: str.
     // strUpper#: str.
     // strTitle#: str.
     assert(reader.cpp.size() == CPP_UNI_CASE);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         std::string str = state0.str0;
         if (str == "") {
             state0.ret = garnishObject(reader0, str);
         } else {
             auto ch = charAt(str, 0);
             if (!ch) {
                 state0.err0 = true;
             } else {
                 switch (state0.num0.asSmallInt()) {
                 case 0:
                     state0.ret = garnishObject(reader0, static_cast<std::string>(ch->toLower()));
                     break;
                 case 1:
                     state0.ret = garnishObject(reader0, static_cast<std::string>(ch->toUpper()));
                     break;
                 case 2:
                     state0.ret = garnishObject(reader0, static_cast<std::string>(ch->toTitle()));
                     break;
                 }
             }
         }
     });
     sys->put(Symbols::get()["strLower#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::INT, 0),
                                   makeAssemblerLine(Instr::CPP, CPP_UNI_CASE),
                                   makeAssemblerLine(Instr::THROA, "Internal error CPP_UNI_CASE"))));
     sys->put(Symbols::get()["strUpper#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::INT, 1),
                                   makeAssemblerLine(Instr::CPP, CPP_UNI_CASE),
                                   makeAssemblerLine(Instr::THROA, "Internal error CPP_UNI_CASE"))));
     sys->put(Symbols::get()["strTitle#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::EXPD, Reg::STR0),
                                   makeAssemblerLine(Instr::THROA, "String expected"),
                                   makeAssemblerLine(Instr::INT, 2),
                                   makeAssemblerLine(Instr::CPP, CPP_UNI_CASE),
                                   makeAssemblerLine(Instr::THROA, "Internal error CPP_UNI_CASE"))));

     // CPP_RANDOM (puts a random integer into %ret)
     // random#.
     assert(reader.cpp.size() == CPP_RANDOM);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         // TODO Use the C++ random generator here, and try to make things thread-safe and not global
         state0.ret = garnishObject(reader0, rand());
     });
     sys->put(Symbols::get()["random#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::CPP, CPP_RANDOM))));

     // CPP_PANIC (panic, then kill)
     // panic#.
     assert(reader.cpp.size() == CPP_PANIC);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         std::cerr << "Panic!" << std::endl;
         dumpEverything(std::cerr, state0);
         hardKill(state0, reader0);
     });
     sys->put(Symbols::get()["panic#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::CPP, CPP_PANIC))));

     // CPP_HAS_SLOT (checks whether %slf has %sym as a slot directly, without checking `missing` or parents)
     // slotCheck#: obj, slot.
     assert(reader.cpp.size() == CPP_HAS_SLOT);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
        ObjectPtr value = (*state0.slf)[state0.sym];
        bool result = (value != nullptr);
        state0.ret = garnishObject(reader0, result);
     });
     sys->put(Symbols::get()["slotCheck#"],
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
                                   makeAssemblerLine(Instr::CPP, CPP_HAS_SLOT))));

     // CPP_WHILE_REGS
     // CPP_WHILE_REGS_ZERO
     // whileDo#: cond, block.
     assert(reader.cpp.size() == CPP_WHILE_REGS);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.mthd = Method(reader0.gtu, { GTU_WHILE_AGAIN });
         state0.mthdz = Method(reader0.gtu, { GTU_POP_TWO });
     });
     assert(reader.cpp.size() == CPP_WHILE_REGS_ZERO);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         state0.mthd = Method(reader0.gtu, { GTU_WHILE_DO });
     });
     sys->put(Symbols::get()["whileDo#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::YLD, Lit::NIL, Reg::SLF),
                                   makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                   makeAssemblerLine(Instr::CPP, CPP_WHILE_REGS_ZERO),
                                   makeAssemblerLine(Instr::GOTO))));

     // CPP_FRESH (allocates a fresh object with *no* slots into %ret; BE CAREFUL!)
     // freshObject#.
     assert(reader.cpp.size() == CPP_FRESH);
     reader.cpp.push_back([](IntState& state0, const ReadOnlyState& reader0) {
         // WARNING: This bypasses the protection to delete the parent
         // slot. This is a bad idea. Don't do this. Seriously. Like,
         // the VM uses it for certain optimizations in careful cases
         // only; NEVER call this.
         state0.ret = clone(reader0.lit[Lit::OBJECT]);
         state0.ret->remove(Symbols::parent());
     });
     sys->put(Symbols::get()["freshObject#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::CPP, CPP_FRESH))));

     // directly#: obj, slot.
     sys->put(Symbols::get()["directly#"],
              defineMethod(unit, global, method,
                           asmCode(makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$1"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                   makeAssemblerLine(Instr::GETD, Reg::SLF),
                                   makeAssemblerLine(Instr::SYMN, Symbols::get()["$2"].index),
                                   makeAssemblerLine(Instr::RTRV),
                                   makeAssemblerLine(Instr::ECLR),
                                   makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                   makeAssemblerLine(Instr::EXPD, Reg::SYM),
                                   makeAssemblerLine(Instr::THROA, "Symbol expected"),
                                   makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                   makeAssemblerLine(Instr::RTRVD),
                                   makeAssemblerLine(Instr::THROA, "Slot not found"))));

     // GTU METHODS //

     // These methods MUST be pushed in the correct order or the standard library
     // code will fail spectacularly. The assertions here verify that that is the
     // case.

     FunctionIndex temp;

     // GTU_EMPTY
     reader.gtu->method(0) = asmCode();

     // GTU_LOOP_DO
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::STO),
                                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                           makeAssemblerLine(Instr::CALL, 0L),
                                           makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                           makeAssemblerLine(Instr::CPP, CPP_LOOP_DO)));
     assert(temp.index == GTU_LOOP_DO);

     // GTU_RETURN
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::RET)));
     assert(temp.index == GTU_RETURN);

     // GTU_THROW
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::THROW)));
     assert(temp.index == GTU_THROW);

     // GTU_TERMINATE
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::CPP, CPP_TERMINATE)));
     assert(temp.index == GTU_TERMINATE);

     // GTU_HANDLER
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::POP, Reg::SLF, Reg::HAND),
                                           makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::ARG),
                                           makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                           makeAssemblerLine(Instr::CALL, 1L)));
     assert(temp.index == GTU_HANDLER);

     // GTU_CALL_ONE
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::CALL, 1L)));
     assert(temp.index == GTU_CALL_ONE);

     // GTU_CALL_ZERO
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::CALL, 0L)));
     assert(temp.index == GTU_CALL_ZERO);

     // GTU_MISSING (assumes %slf contains the appropriate caller, %ret the missing method, and %sym the symbol)
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::YLDC, Lit::SYMBOL, Reg::PTR),
                                           makeAssemblerLine(Instr::LOAD, Reg::SYM),
                                           makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG),
                                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                           makeAssemblerLine(Instr::CALL, 1L)));
     assert(temp.index == GTU_MISSING);

     // GTU_TRUE
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::YLD, Lit::TRUE, Reg::RET)));
     assert(temp.index == GTU_TRUE);

     // GTU_FALSE
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::YLD, Lit::FALSE, Reg::RET)));
     assert(temp.index == GTU_FALSE);

     // GTU_NIL
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::YLD, Lit::NIL, Reg::RET)));
     assert(temp.index == GTU_NIL);

     // GTU_ERROR_MESSAGE
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                           makeAssemblerLine(Instr::YLD, Lit::ERR, Reg::SLF),
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
     assert(temp.index == GTU_ERROR_MESSAGE);

     // GTU_ERROR
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                                           makeAssemblerLine(Instr::YLD, Lit::ERR, Reg::SLF),
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
     assert(temp.index == GTU_ERROR);

     // GTU_KEYS
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::PEEK, Reg::PTR, Reg::STO),
                                           makeAssemblerLine(Instr::CALL, 1L)));
     assert(temp.index == GTU_KEYS);

     // GTU_KEY_TERM
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::POP, Reg::RET, Reg::STO)));
     assert(temp.index == GTU_KEY_TERM);

     // GTU_THROW_OBJ
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                           makeAssemblerLine(Instr::LOCRT),
                                           makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                           makeAssemblerLine(Instr::SYMN, Symbols::get()["stack"].index),
                                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                           makeAssemblerLine(Instr::SETF),
                                           makeAssemblerLine(Instr::THROW)));
     assert(temp.index == GTU_THROW_OBJ);

     // GTU_PANIC
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::CPP, CPP_PANIC)));
     assert(temp.index == GTU_PANIC);

     // GTU_WHILE_DO
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::POP, Reg::RET, Reg::STO),
                                           makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                           makeAssemblerLine(Instr::PEEK, Reg::PTR, Reg::STO),
                                           makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                                           makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                           makeAssemblerLine(Instr::CALL, 0L),
                                           makeAssemblerLine(Instr::YLD, Lit::TRUE, Reg::PTR),
                                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                           makeAssemblerLine(Instr::TEST),
                                           makeAssemblerLine(Instr::CPP, CPP_WHILE_REGS),
                                           makeAssemblerLine(Instr::BRANCH)));
     assert(temp.index == GTU_WHILE_DO);

     // GTU_POP_TWO
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::POP, Reg::RET, Reg::STO),
                                           makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                                           makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)));
     assert(temp.index == GTU_POP_TWO);

     // GTU_WHILE_AGAIN
     temp = reader.gtu->pushMethod(asmCode(makeAssemblerLine(Instr::POP, Reg::RET, Reg::STO),
                                           makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                           makeAssemblerLine(Instr::PEEK, Reg::PTR, Reg::STO),
                                           makeAssemblerLine(Instr::CALL, 0L),
                                           makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                                           makeAssemblerLine(Instr::CPP, CPP_WHILE_REGS_ZERO),
                                           makeAssemblerLine(Instr::GOTO)));

}

void bindArgv(ObjectPtr argv_, ObjectPtr string, int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        std::string name = "$" + to_string(i - 1);
        ObjectPtr curr = clone(string);
        curr->prim(argv[i]);
        argv_->put(Symbols::get()[name], curr);
    }
}

ObjectPtr spawnObjects(IntState& state, ReadOnlyState& reader, int argc, char** argv) {

    ObjectPtr object(GC::get().allocate());
    ObjectPtr meta(clone(object));
    ObjectPtr global(clone(object));

    ObjectPtr proc(clone(object));
    ObjectPtr method(clone(proc));
    ObjectPtr number(clone(object));
    ObjectPtr string(clone(object));
    ObjectPtr symbol(clone(object));

    ObjectPtr cont(clone(proc));

    ObjectPtr err(clone(object));
    ObjectPtr exception(clone(object));
    ObjectPtr systemError(clone(exception));

    ObjectPtr process(clone(object));
    ObjectPtr stream(clone(object));
    ObjectPtr stdout_(clone(stream));
    ObjectPtr stdin_(clone(stream));
    ObjectPtr stderr_(clone(stream));

    ObjectPtr array_(clone(object));
    ObjectPtr dict(clone(object));

    ObjectPtr sys(clone(object));
    ObjectPtr stackFrame(clone(object));
    ObjectPtr fileHeader(clone(object));
    ObjectPtr kernel(clone(object));

    ObjectPtr nil(clone(object));
    ObjectPtr boolean(clone(object));
    ObjectPtr true_(clone(boolean));
    ObjectPtr false_(clone(boolean));

    ObjectPtr argv_(clone(object));
    ObjectPtr fresh(clone(object));

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
    global->put(Symbols::get()["Exception"], exception);
    global->put(Symbols::get()["SystemError"], systemError);
    global->put(Symbols::get()["Array"], array_);
    global->put(Symbols::get()["Dict"], dict);
    global->put(Symbols::get()["Kernel"], kernel);
    global->put(Symbols::get()["StackFrame"], stackFrame);
    global->put(Symbols::get()["FileHeader"], fileHeader);

    // Object is its own parent
    object->put(Symbols::parent(), object);
    object->addProtection(Symbols::parent(), Protection::PROTECT_DELETE);

    // Meta linkage
    meta->put(Symbols::get()["meta"], meta);
    object->put(Symbols::get()["meta"], meta);
    meta->put(Symbols::get()["sys"], sys);
    global->put(Symbols::get()["err"], err);
    // (This one will be replaced fairly early in the load process but
    // needs to have a value at least)
    meta->put(Symbols::get()["operators"], dict);

    // Meta Protection
    meta->addProtection(Symbols::get()["meta"], Protection::PROTECT_DELETE);
    object->addProtection(Symbols::get()["meta"], Protection::PROTECT_DELETE);
    meta->addProtection(Symbols::get()["sys"],
                        Protection::PROTECT_DELETE | Protection::PROTECT_ASSIGN);
    meta->addProtection(Symbols::get()["operators"],
                        Protection::PROTECT_DELETE);

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

    // argv
    global->put(Symbols::get()["$argv"], argv_);
    bindArgv(argv_, string, argc, argv);

    // Dictionary setup
    fresh->remove(Symbols::parent());
    dict->put(Symbols::get()["&impl"], fresh);

    // Spawn the literal objects table
    assert(reader.lit.size() == Lit::NIL   );
    reader.lit.emplace_back(nil       );
    assert(reader.lit.size() == Lit::FALSE );
    reader.lit.emplace_back(false_    );
    assert(reader.lit.size() == Lit::TRUE  );
    reader.lit.emplace_back(true_     );
    assert(reader.lit.size() == Lit::BOOL  );
    reader.lit.emplace_back(boolean   );
    assert(reader.lit.size() == Lit::STRING);
    reader.lit.emplace_back(string    );
    assert(reader.lit.size() == Lit::NUMBER);
    reader.lit.emplace_back(number    );
    assert(reader.lit.size() == Lit::SYMBOL);
    reader.lit.emplace_back(symbol    );
    assert(reader.lit.size() == Lit::SFRAME);
    reader.lit.emplace_back(stackFrame);
    assert(reader.lit.size() == Lit::METHOD);
    reader.lit.emplace_back(method    );
    assert(reader.lit.size() == Lit::FHEAD );
    reader.lit.emplace_back(fileHeader);
    assert(reader.lit.size() == Lit::ERR   );
    reader.lit.emplace_back(err       );
    assert(reader.lit.size() == Lit::ARRAY );
    reader.lit.emplace_back(array_    );
    assert(reader.lit.size() == Lit::DICT  );
    reader.lit.emplace_back(dict      );
    assert(reader.lit.size() == Lit::OBJECT);
    reader.lit.emplace_back(object    );

    // The core libraries (this is done in runREPL now)
    //readFile("std/latitude.lat", { global, global }, state);

    return global;
}

void throwError(IntState& state, const ReadOnlyState& reader, std::string name, std::string msg) {
    state.stack = pushNode(state.stack, state.cont);
    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_ERROR_MESSAGE }));
    state.sym = Symbols::get()[name];
    state.ret = garnishObject(reader, msg);
}

void throwError(IntState& state, const ReadOnlyState& reader, std::string name) {
    state.stack = pushNode(state.stack, state.cont);
    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_ERROR }));
    state.sym = Symbols::get()[name];
}

void throwError(IntState& state, const ReadOnlyState& reader, ObjectPtr obj) {
    state.stack = pushNode(state.stack, state.cont);
    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_THROW_OBJ }));
    state.slf = obj;
}
