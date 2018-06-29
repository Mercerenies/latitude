//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef REPL_HPP
#define REPL_HPP

#include "Proto.hpp"
#include "Bytecode.hpp"

void runREPL(ObjectPtr global, VMState& vm);

void runRunner(ObjectPtr global, VMState& vm);

void runCompiler(ObjectPtr global, VMState& vm);

#endif // REPL_HPP
