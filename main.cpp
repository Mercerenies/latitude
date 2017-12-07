extern "C" {
#include "lex.yy.h"
#include "Parser.tab.h"
}
#include "Reader.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "GC.hpp"
#include "Symbol.hpp"
#include "Bytecode.hpp"
#include "REPL.hpp"
#include "Args.hpp"
#include <iostream>
#include <cstring>

int main(int argc, char** argv) {

    std::cout << "Latitude ALPHA" << std::endl;

    /* CmdArgs args = */ parseArgs(argc, argv);

    IntState state = intState();
    ReadOnlyState reader = readOnlyState();
    ObjectPtr global = spawnObjects(state, reader);
    runREPL(global, state, reader);

    return 0;
}
