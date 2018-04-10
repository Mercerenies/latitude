#ifndef REPL_HPP
#define REPL_HPP

#include "Proto.hpp"
#include "Bytecode.hpp"

void runREPL(ObjectPtr global, IntState& state, ReadOnlyState& reader);

void runRunner(ObjectPtr global, IntState& state, ReadOnlyState& reader);

#endif // REPL_HPP
