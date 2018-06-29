//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "GC.hpp"
#include "REPL.hpp"
#include "Symbol.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Reader.hpp"
#include "Pathname.hpp"
#include "Precedence.hpp"
#include <string>

using namespace std;

void runREPL(ObjectPtr global, VMState& vm) {

    OperatorTable table = getTable(global);

    string pathname = stripFilename(getExecutablePathname());
    readFile(pathname + "std/repl.lats", { clone(global), clone(global) }, vm, table);

    while (!isIdling(vm.state))
        doOneStep(vm);
}

void runRunner(ObjectPtr global, VMState& vm) {

    OperatorTable table = getTable(global);

    string pathname = stripFilename(getExecutablePathname());
    readFile(pathname + "std/runner.lats", { clone(global), clone(global) }, vm, table);

    while (!isIdling(vm.state))
        doOneStep(vm);
}

void runCompiler(ObjectPtr global, VMState& vm) {

    OperatorTable table = getTable(global);

    string pathname = stripFilename(getExecutablePathname());
    readFile(pathname + "std/compiler.lats", { clone(global), clone(global) }, vm, table);

    while (!isIdling(vm.state))
        doOneStep(vm);
}
