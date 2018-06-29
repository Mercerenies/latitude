//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

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
#include "Pathname.hpp"
#include "REPL.hpp"
#include "Args.hpp"
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdlib>

void initRandom() {
    srand(time(NULL));
}

int main(int argc, char** argv) {
    initRandom();

    CmdArgs args = parseArgs(argc, argv);

    switch (args.output) {
    case OutputMode::NONE: {
        // Do nothing here
        break;
    }
    case OutputMode::VERSION: {
        outputVersion();
        break;
    }
    case OutputMode::HELP: {
        outputHelp();
        break;
    }
    }

    IntState state = intState();
    ReadOnlyState reader = readOnlyState();

    // Overwrite the nullary register
    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_EMPTY }));

    ObjectPtr global = spawnObjects(state, reader, argc, argv);
    VMState vm { state, reader };

    switch (args.run) {
    case RunMode::REPL: {
        outputVersion();
        runREPL(global, vm);
        break;
    }
    case RunMode::RUNNER: {
        runRunner(global, vm);
        break;
    }
    case RunMode::COMPILE: {
        runCompiler(global, vm);
        break;
    }
    case RunMode::EXIT: {
        // Do nothing and exit
        break;
    }
    }

#ifdef PROFILE_INSTR
    Profiling::get().dumpData();
#endif

    return 0;
}
