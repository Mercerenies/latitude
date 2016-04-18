#include "Garnish.hpp"
#include "Reader.hpp"
#include <sstream>
#include <type_traits>

using namespace std;

ObjectPtr garnish(ObjectPtr global, bool value) {
    if (value)
        return eval("meta True.", global, global);
    else
        return eval("meta False.", global, global);
}

ObjectPtr garnish(ObjectPtr global, boost::blank value) {
    return eval("meta Nil.", global, global);
}

ObjectPtr garnish(ObjectPtr global, std::string value) {
    ObjectPtr string = eval("meta String.", global, global);
    ObjectPtr val = clone(string);
    val.lock()->prim(value);
    return val;
}

ObjectPtr garnish(ObjectPtr global, double value) {
    ObjectPtr num = eval("meta Number.", global, global);
    ObjectPtr val = clone(num);
    val.lock()->prim(value);
    return val;
}

ObjectPtr garnish(ObjectPtr global, int value) {
    return garnish(global, (double)value);
}

ObjectPtr garnish(ObjectPtr global, Number value) {
    ObjectPtr num = eval("meta Number.", global, global);
    ObjectPtr val = clone(num);
    val.lock()->prim(value);
    return val;
}

ObjectPtr garnish(ObjectPtr global, Symbolic value) {
    ObjectPtr sym = eval("meta Symbol.", global, global);
    ObjectPtr val = clone(sym);
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

class DumpObjectVisitor : public boost::static_visitor<ObjectPtr> {
private:
    ObjectPtr curr, mthd, lex, dyn;
public:
    DumpObjectVisitor(ObjectPtr curr, ObjectPtr mthd, ObjectPtr lex, ObjectPtr dyn)
        : curr(curr) , mthd(mthd), lex(lex) , dyn(dyn) {}
    template <typename T>
    ObjectPtr operator()(T& val) const {
        return mthd;
    }
};

template <>
ObjectPtr DumpObjectVisitor::operator()<Method>(Method& val) const {
    return callMethod(curr.lock(), mthd, clone(dyn));
}
template <>
ObjectPtr DumpObjectVisitor::operator()<SystemCall>(SystemCall& val) const {
    return val(std::list<ObjectPtr>());
}

void dumpObject(ObjectPtr lex, ObjectPtr dyn, Stream& stream, ObjectPtr obj) {
    ObjectPtr toString0 = getInheritedSlot(obj, Symbols::get()["toString"]);
    ObjectPtr asString0 =
        boost::apply_visitor(DumpObjectVisitor(obj, toString0, lex, dyn),
                             toString0.lock()->prim());
    if (auto str = boost::get<std::string>(&asString0.lock()->prim()))
        stream.writeLine(*str);
    else
        stream.writeLine("<?>");
    for (auto key : keys(obj)) {
        auto value = getInheritedSlot(obj, key);
        ObjectPtr toString1 = getInheritedSlot(value, Symbols::get()["toString"]);
        ObjectPtr asString1 =
            boost::apply_visitor(DumpObjectVisitor(value, toString1, lex, dyn),
                                 toString1.lock()->prim());
        stream.writeText("  ");
        stream.writeText(Symbols::get()[key]);
        stream.writeText(": ");
        if (auto str = boost::get<std::string>(&asString1.lock()->prim()))
            stream.writeLine(*str);
        else
            stream.writeLine("<?>");
    }
}

void simplePrintObject(ObjectPtr lex, ObjectPtr dyn, Stream& stream, ObjectPtr obj) {
    ObjectPtr toString0 = getInheritedSlot(obj, Symbols::get()["toString"]);
    ObjectPtr asString0 =
        boost::apply_visitor(DumpObjectVisitor(obj, toString0, lex, dyn),
                             toString0.lock()->prim());
    if (auto str = boost::get<std::string>(&asString0.lock()->prim()))
        stream.writeLine(*str);
    else
        stream.writeLine("<?>");
}
