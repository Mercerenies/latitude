#ifndef DUMP_HPP
#define DUMP_HPP

#include <ostream>
#include <stack>
#include "Proto.hpp"
#include "Bytecode.hpp"

/// An adaptor class which provides useful dumping and printing
/// capabilities for object pointers.
class DebugObject {
private:
    ObjectPtr impl;
public:
    DebugObject(ObjectPtr ptr);
    ObjectPtr get() const;
    friend std::ostream& operator <<(std::ostream& out, const DebugObject& obj);
};

std::ostream& operator <<(std::ostream& out, const DebugObject& obj);

/// An adaptor class which provides useful dumping and printing
/// capabilities for stacks of object pointers
class DebugStackObject {
private:
    std::stack<ObjectPtr>& impl;
public:
    DebugStackObject(std::stack<ObjectPtr>& stack);
    std::stack<ObjectPtr>& get() const;
    friend std::ostream& operator <<(std::ostream& out, const DebugStackObject& obj);
};

std::ostream& operator <<(std::ostream& out, const DebugStackObject& obj);

void dumpEverything(std::ostream& out, IntState& state);

#endif // DUMP_HPP
