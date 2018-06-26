#ifndef DUMP_HPP
#define DUMP_HPP

#include <ostream>
#include <stack>
#include "Proto.hpp"
#include "Bytecode.hpp"

/// \brief An adaptor class for printing debug values.
///
/// An adaptor class which provides useful dumping and printing
/// capabilities for object pointers. There exists an `operator<<`
/// overload for this class which performs introspection on it in an
/// attempt to identify the sort of object it has been passed.
class DebugObject {
private:
    ObjectPtr impl;
public:

    /// Constructs a DebugObject.
    ///
    /// \param ptr the object pointer
    DebugObject(ObjectPtr ptr);

    /// Accesses the object pointer within the debug object.
    ///
    /// \return the object pointer
    ObjectPtr get() const;

    friend std::ostream& operator <<(std::ostream& out, const DebugObject& obj);

};

std::ostream& operator <<(std::ostream& out, const DebugObject& obj);

/// \brief An adaptor class for printing stacks of objects.
///
/// An adaptor class which provides useful dumping and printing
/// capabilities for stacks of object pointers. There exists an
/// `operator<<` overload for this class which performs introspection
/// on all of its values, using DebugObject.
class DebugStackObject {
private:
    std::stack<ObjectPtr>& impl;
public:

    /// Constructs a DebugStackObject.
    ///
    /// \param stack the stack to wrap
    DebugStackObject(std::stack<ObjectPtr>& stack);

    /// Returns the wrapped stack.
    ///
    /// \return the stack
    std::stack<ObjectPtr>& get() const;

    friend std::ostream& operator <<(std::ostream& out, const DebugStackObject& obj);

};

std::ostream& operator <<(std::ostream& out, const DebugStackObject& obj);

void dumpEverything(std::ostream& out, IntState& state);

#endif // DUMP_HPP
