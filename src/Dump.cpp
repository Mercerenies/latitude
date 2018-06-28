//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details


#include "Dump.hpp"
#include "Parents.hpp"
#include <boost/variant.hpp>
#include <list>

DebugObject::DebugObject(ObjectPtr ptr)
    : impl(ptr) {}

ObjectPtr DebugObject::get() const {
    return impl;
}

DebugStackObject::DebugStackObject(std::stack<ObjectPtr>& stack)
    : impl(stack) {}

std::stack<ObjectPtr>& DebugStackObject::get() const {
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
    ObjectPtr value = objectGet(obj, Symbols::get()["toString"]);
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

void dumpStack(std::ostream& out, std::stack<ObjectPtr>& stack) {
    if (stack.empty())
        return;
    auto value = stack.top();
    stack.pop();
    dumpStack(out, stack);
    out << DebugObject(value) << " ";
    stack.push(value);
}

std::ostream& operator <<(std::ostream& out, const DebugStackObject& obj) {
    if (obj.get().empty())
        return out;
    auto value = obj.get().top();
    obj.get().pop();
    dumpStack(out, obj.get());
    out << DebugObject(value);
    return out;
}

void dumpBT(std::ostream& out, NodePtr<BacktraceFrame> frame) {
    if (frame == nullptr)
        return;
    auto value = frame->get();
    out << " " << std::get<1>(value) << "(" << std::get<0>(value) << ")";
    dumpBT(out, popNode(frame));
}

void dumpEverything(std::ostream& out, IntState& state) {

    // Stacks are printed from bottom to top, so the rightmost element
    // is the top stack element

    out << "%ptr  : " <<      DebugObject(state.ptr ) << std::endl;
    out << "%slf  : " <<      DebugObject(state.slf ) << std::endl;
    out << "%ret  : " <<      DebugObject(state.ret ) << std::endl;

    out << "%lex  : " << DebugStackObject(state.lex ) << std::endl;
    out << "%dyn  : " << DebugStackObject(state.dyn ) << std::endl;
    out << "%arg  : " << DebugStackObject(state.arg ) << std::endl;
    out << "%sto  : " << DebugStackObject(state.sto ) << std::endl;
    out << "%hand : " << DebugStackObject(state.hand) << std::endl;

    out << "%err0 : " << state.err0 << std::endl;
    out << "%err1 : " << state.err1 << std::endl;

    out << "%sym  : " << Symbols::get()[state.sym] << std::endl;
    out << "%num0 : " << state.num0.asString() << std::endl;
    out << "%num1 : " << state.num1.asString() << std::endl;

    out << "%str0 : " << state.str0 << std::endl;
    out << "%str1 : " << state.str1 << std::endl;

    out << "%flag : " << state.flag << std::endl;
    out << "%trace:";
    dumpBT(out, pushNode(state.trace, std::make_tuple(state.line, state.file)));
    out << std::endl;

}
