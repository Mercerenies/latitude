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

    switch (args.run) {
    case RunMode::REPL: {
        IntState state = intState();
        ReadOnlyState reader = readOnlyState();
        state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_EMPTY })); // Overwrite the nullary register
        ObjectPtr global = spawnObjects(state, reader, argc, argv);
        outputVersion();
        runREPL(global, state, reader);
        break;
    }
    case RunMode::RUNNER: {
        IntState state = intState();
        ReadOnlyState reader = readOnlyState();
        state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_EMPTY })); // Overwrite the nullary register
        ObjectPtr global = spawnObjects(state, reader, argc, argv);
        runRunner(global, state, reader);
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
