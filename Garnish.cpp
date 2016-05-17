#include "Garnish.hpp"
#include "Reader.hpp"
#include "Macro.hpp"
#include <sstream>
#include <type_traits>

using namespace std;

ObjectPtr garnish(Scope scope, bool value) {
    if (value)
        return getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["True"]);
    else
        return getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["False"]);
}

ObjectPtr garnish(Scope scope, boost::blank value) {
    return getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["Nil"]);
}

ObjectPtr garnish(Scope scope, std::string value) {
    ObjectPtr string = getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["String"]);
    ObjectPtr val = clone(string);
    val.lock()->prim(value);
    return val;
}

ObjectPtr garnish(Scope scope, double value) {
    ObjectPtr num = getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["Symbol"]);
    ObjectPtr val = clone(num);
    val.lock()->prim(value);
    return val;
}

ObjectPtr garnish(Scope scope, int value) {
    return garnish(scope, Number( (Number::smallint)value ));
}

ObjectPtr garnish(Scope scope, long value) {
    return garnish(scope, Number( (Number::smallint)value ));
}

ObjectPtr garnish(Scope scope, Number value) {
    ObjectPtr num = getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["Number"]);
    ObjectPtr val = clone(num);
    val.lock()->prim(value);
    return val;
}

ObjectPtr garnish(Scope scope, Symbolic value) {
    ObjectPtr sym = getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["Symbol"]);
    ObjectPtr val = clone(sym);
    val.lock()->prim(value);
    return val;
}

ObjectPtr garnish(Scope scope, StreamPtr value) {
    ObjectPtr stream = getInheritedSlot(scope, meta(scope, scope.lex), Symbols::get()["Stream"]);
    ObjectPtr val = clone(stream);
    val.lock()->prim(value);
    return val;
}

class PrimToStringVisitor : public boost::static_visitor<std::string> {
public:
    std::string operator()(boost::blank& val) const {
        return "()";
    }
    std::string operator()(Number& val) const {
        return val.asString();
    }
    std::string operator()(std::string& val) const {
        ostringstream oss;
        oss << '"';
        for (char ch : val) {
            if (ch == '"')
                oss << "\\\"";
            else if (ch == '\\')
                oss << "\\\\";
            else
                oss << ch;
        }
        oss << '"';
        return oss.str();
    }
    std::string operator()(Method& val) const {
        ostringstream oss;
        oss << &val;
        return oss.str();
    }
    std::string operator()(NewMethod& val) const {
        ostringstream oss;
        oss << &val;
        return oss.str();
    }
    std::string operator()(SystemCall& val) const {
        ostringstream oss;
        oss << &val;
        return oss.str();
    }
    std::string operator()(StreamPtr& val) const {
        ostringstream oss;
        oss << val;
        return oss.str();
    }
    std::string operator()(Symbolic& val) const {
        ostringstream oss;
        std::string str = Symbols::get()[val];
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
        return oss.str();
    }
    std::string operator()(std::weak_ptr<SignalValidator>& val) const {
        ostringstream oss;
        oss << &val;
        return oss.str();
    }
    std::string operator()(ProcessPtr& val) const {
        ostringstream oss;
        oss << val;
        return oss.str();
    }
    std::string operator()(StatePtr& val) const {
        ostringstream oss;
        oss << val;
        return oss.str();
    }
};

std::string primToString(ObjectPtr obj) {
    return boost::apply_visitor(PrimToStringVisitor(), obj.lock()->prim());
}

template <typename U, typename V>
struct PrimEquals_ {
    static bool call(U& first, V& second) {
        return false;
    }
};

class PrimEqualsVisitor : public boost::static_visitor<bool> {
public:
    template <typename U,
              class = decltype(declval<U>() == declval<U>())>
    bool operator()(U& first, U& second) const {
        return first == second;
    }
    template <typename U, typename V>
    bool operator()(U& first, V& second) const {
        return false;
    }
};

bool primEquals(ObjectPtr obj1, ObjectPtr obj2) {
    return boost::apply_visitor(PrimEqualsVisitor(),
                                obj1.lock()->prim(),
                                obj2.lock()->prim());
}

class PrimLTVisitor : public boost::static_visitor<bool> {
public:
    bool operator()(string first, string second) const {
        return first < second;
    }
    bool operator()(Number first, Number second) const {
        return first < second;
    }
    bool operator()(Symbolic first, Symbolic second) const {
        return first < second;
    }
    template <typename U, typename V>
    bool operator()(U& first, V& second) const {
        return false;
    }
};

bool primLT(ObjectPtr obj1, ObjectPtr obj2) {
    return boost::apply_visitor(PrimLTVisitor(),
                                obj1.lock()->prim(),
                                obj2.lock()->prim());
}

void dumpObject(Scope scope, Stream& stream, ObjectPtr obj) {
    ObjectPtr toString0 = getInheritedSlot(scope, obj, Symbols::get()["toString"]);
    ObjectPtr asString0 = doCallWithArgs(scope, obj, toString0);
    if (auto str = boost::get<std::string>(&asString0.lock()->prim()))
        stream.writeLine(*str);
    else
        stream.writeLine("<?>");
    for (auto key : keys(obj)) {
        auto value = getInheritedSlot(scope, obj, key);
        ObjectPtr toString1 = getInheritedSlot(scope, value, Symbols::get()["toString"]);
        ObjectPtr asString1 = doCallWithArgs(scope, value, toString1);
        stream.writeText("  ");
        stream.writeText(Symbols::get()[key]);
        stream.writeText(": ");
        if (auto str = boost::get<std::string>(&asString1.lock()->prim()))
            stream.writeLine(*str);
        else
            stream.writeLine("<?>");
    }
}

void simplePrintObject(Scope scope, Stream& stream, ObjectPtr obj) {
    ObjectPtr toString0 = getInheritedSlot(scope, obj, Symbols::get()["toString"]);
    ObjectPtr asString0 = doCallWithArgs(scope, obj, toString0);
    if (auto str = boost::get<std::string>(&asString0.lock()->prim()))
        stream.writeLine(*str);
    else
        stream.writeLine("<?>");
}

InstrSeq garnishSeq(bool value) {
    InstrSeq seq;
    if (value) {
        seq = asmCode(makeAssemblerLine(Instr::GETL),
                      makeAssemblerLine(Instr::SYM, "meta"),
                      makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                      makeAssemblerLine(Instr::RTRV),
                      makeAssemblerLine(Instr::SYM, "True"),
                      makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                      makeAssemblerLine(Instr::RTRV));
    } else {
        seq = asmCode(makeAssemblerLine(Instr::GETL),
                      makeAssemblerLine(Instr::SYM, "meta"),
                      makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                      makeAssemblerLine(Instr::RTRV),
                      makeAssemblerLine(Instr::SYM, "False"),
                      makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                      makeAssemblerLine(Instr::RTRV));
    }
    return seq;
}

InstrSeq garnishSeq(boost::blank value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL),
                           makeAssemblerLine(Instr::SYM, "meta"),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::SYM, "Nil"),
                           makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV));
    return seq;
}

InstrSeq garnishSeq(std::string value) {
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL),
                           makeAssemblerLine(Instr::SYM, "meta"),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::SYM, "String"),
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
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL),
                           makeAssemblerLine(Instr::SYM, "meta"),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::SYM, "Number"),
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
    InstrSeq seq = asmCode(makeAssemblerLine(Instr::GETL),
                           makeAssemblerLine(Instr::SYM, "meta"),
                           makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                           makeAssemblerLine(Instr::RTRV),
                           makeAssemblerLine(Instr::SYM, "Symbol"),
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
