//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef DUMP_HPP
#define DUMP_HPP

#include <ostream>
#include <stack>
#include "Proto.hpp"
#include "Bytecode.hpp"

/// \brief An adapter for printing debug values.
///
/// An adapter which provides useful dumping and printing
/// capabilities for object pointers. There exists an `operator<<`
/// overload for this class which performs introspection on it in an
/// attempt to identify the sort of object it has been passed.
struct DebugObject {
    ObjectPtr impl;
};

std::ostream& operator <<(std::ostream& out, const DebugObject& obj);

/// \brief An adapter for printing debug values in great detail.
///
/// This adapter prints objects in much more detail than DebugObject,
/// which is intended to provide only a summary.
struct DebugDetailObject {
    ObjectPtr impl;
};

std::ostream& operator <<(std::ostream& out, const DebugDetailObject& obj);

/// \brief An adapter for printing stacks of objects.
///
/// An adapter which provides useful dumping and printing capabilities
/// for stacks of object pointers. There exists an `operator<<`
/// overload for this class which performs introspection on all of its
/// values, using DebugObject.
struct DebugStackObject {
    std::stack<ObjectPtr>& impl;

};

std::ostream& operator <<(std::ostream& out, const DebugStackObject& obj);

void dumpEverything(std::ostream& out, VMState& state);

#endif // DUMP_HPP
