#ifndef _REPL_HPP_
#define _REPL_HPP_

#include "Proto.hpp"
#include "Bytecode.hpp"

void runREPL(ObjectPtr global, IntState& state, ReadOnlyState& reader);

#endif // _REPL_HPP_
