//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details


#include "Dump.hpp"
#include "Parents.hpp"
#include <boost/variant.hpp>
#include <list>

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
    ObjectPtr value = objectGet(obj, Symbols::get()["toString"]);
    if (value == nullptr) {
        return "";
    } else if (std::string* curr = boost::get<std::string>(&value->prim())) {
        return *curr;
    }
    return "";
}

std::ostream& operator <<(std::ostream& out, const DebugObject& obj) {
    if (obj.impl == nullptr) {
        out << "nullptr";
        return out;
    }
    // First, we know we can safely print the pointer's numerical value.
    out << obj.impl.get();
    // Get prim info
    std::string prim = boost::apply_visitor(DebugPrimVisitor(), obj.impl->prim());
    // Get toString info
    std::string str = toStringInfo(obj.impl);
    // Print everything
    if (prim != "")
        out << "[" << prim << "]";
    else if (str != "")
        out << "[" << str << "]";
    // Finish.
    return out;
}

void dumpStack(std::ostream& out, std::stack<ObjectPtr>& stack) {
    if (stack.empty())
        return;
    auto value = stack.top();
    stack.pop();
    dumpStack(out, stack);
    out << DebugObject{ value } << " ";
    stack.push(value);
}

std::ostream& operator <<(std::ostream& out, const DebugStackObject& obj) {
    if (obj.impl.empty())
        return out;
    auto value = obj.impl.top();
    obj.impl.pop();
    dumpStack(out, obj.impl);
    out << DebugObject { value };
    return out;
}

void dumpBT(std::ostream& out, NodePtr<BacktraceFrame> frame) {
    if (frame == nullptr)
        return;
    auto value = frame->get();
    out << " " << std::get<1>(value) << "(" << std::get<0>(value) << ")";
    dumpBT(out, popNode(frame));
}

void dumpEverything(std::ostream& out, VMState& vm) {

    // Stacks are printed from bottom to top, so the rightmost element
    // is the top stack element

    out << "%ptr  : " <<      DebugObject{ vm.trans.ptr } << std::endl;
    out << "%slf  : " <<      DebugObject{ vm.trans.slf } << std::endl;
    out << "%ret  : " <<      DebugObject{ vm.trans.ret } << std::endl;

    out << "%lex  : " << DebugStackObject{ vm.state.lex  } << std::endl;
    out << "%dyn  : " << DebugStackObject{ vm.state.dyn  } << std::endl;
    out << "%arg  : " << DebugStackObject{ vm.state.arg  } << std::endl;
    out << "%sto  : " << DebugStackObject{ vm.state.sto  } << std::endl;
    out << "%hand : " << DebugStackObject{ vm.state.hand } << std::endl;

    out << "%err0 : " << vm.trans.err0 << std::endl;
    out << "%err1 : " << vm.trans.err1 << std::endl;

    out << "%sym  : " << Symbols::get()[vm.trans.sym] << std::endl;
    out << "%num0 : " << vm.trans.num0.asString() << std::endl;
    out << "%num1 : " << vm.trans.num1.asString() << std::endl;

    out << "%str0 : " << vm.trans.str0 << std::endl;
    out << "%str1 : " << vm.trans.str1 << std::endl;

    out << "%flag : " << vm.trans.flag << std::endl;
    out << "%trace:";
    dumpBT(out, pushNode(vm.state.trace, std::make_tuple(vm.state.line, vm.state.file)));
    out << std::endl;

}
