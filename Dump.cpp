
#include "Dump.hpp"
#include <boost/variant.hpp>
#include <list>

DebugObject::DebugObject(ObjectPtr ptr)
    : impl(ptr) {}

ObjectPtr DebugObject::get() const {
    return impl;
}

struct DebugPrimVisitor : boost::static_visitor<std::string> {

    std::string operator()(boost::blank) const {
        return "";
    }

    std::string operator()(Number n) const {
        return "#(" + n.asString() + ")";
    }

    std::string operator()(std::string str) const {
        return "$(" + str + ")";
    }

    std::string operator()(Symbolic sym) const {
        return "'(" + Symbols::get()[sym] + ")";
    }

    std::string operator()(StreamPtr) const {
        return "StreamPtr";
    }

    std::string operator()(ProcessPtr) const {
        return "ProcessPtr";
    }

    std::string operator()(Method) const {
        return "Method";
    }

    std::string operator()(StatePtr) const {
        return "StatePtr";
    }

};

std::string toStringInfo(ObjectPtr obj) {
    auto sym = Symbols::parent();
    std::list<ObjectPtr> parents;
    ObjectPtr curr = obj;
    Symbolic name = Symbols::get()["toString"];
    ObjectPtr value = nullptr;
    while (find(parents.begin(), parents.end(), curr) == parents.end()) {
        parents.push_back(curr);
        ObjectPtr slot = (*curr)[name];
        if (slot != nullptr) {
            value = slot;
            break;
        }
        curr = (*curr)[ sym ];
    }
    if (value == nullptr) {
        return "";
    } else if (std::string* curr = boost::get<std::string>(&value->prim())) {
        return *curr;
    }
    return "";
}

std::ostream& operator <<(std::ostream& out, const DebugObject& obj) {
    // First, we know we can safely print the pointer's numerical value.
    out << obj.get().get();
    // Get prim info
    std::string prim = boost::apply_visitor(DebugPrimVisitor(), obj.get()->prim());
    // Get toString info
    std::string str = toStringInfo(obj.get());
    // Print everything
    if (prim != "")
        out << "[" << prim << "]";
    else if (str != "")
        out << "[" << str << "]";
    // Finish.
    return out;
}
