#include "Garnish.hpp"
#include "Reader.hpp"
#include <sstream>

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
    val->prim(value);
    return val;
}

ObjectPtr garnish(ObjectPtr global, double value) {
    ObjectPtr num = eval("meta Number.", global, global);
    ObjectPtr val = clone(num);
    val->prim(value);
    return val;
}

class PrimToStringVisitor : public boost::static_visitor<std::string> {
public:
    std::string operator()(boost::blank& val) const {
        return "()";
    }
    std::string operator()(double& val) const {
        ostringstream oss;
        oss << val;
        return oss.str();
    }
    std::string operator()(std::string& val) const {
        ostringstream oss;
        oss << '"';
        for (char ch : val) {
            if (ch == '"')
                oss << "\\\"";
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
};

std::string primToString(ObjectPtr obj) {
    return boost::apply_visitor(PrimToStringVisitor(), obj->prim());
}
