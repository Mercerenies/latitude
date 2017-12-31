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

int main(int argc, char** argv) {

    std::cout << "Latitude [Development Version]" << std::endl;

    CmdArgs args = parseArgs(argc, argv);

    switch (args.output) {
    case OutputMode::NONE: {
        // Do nothing here
        break;
    }
    case OutputMode::VERSION: {
        // This will print an actual version number once the language has been
        // formally released. For now, it's a placeholder string.
        std::cout << "Latitude [Development Version]" << std::endl;
        break;
    }
    case OutputMode::HELP: {
        outputHelp();
        break;
    }
    }

    switch (args.run) {
    case RunMode::DEFAULT: {
        IntState state = intState();
        ReadOnlyState reader = readOnlyState();
        ObjectPtr global = spawnObjects(state, reader, argc, argv);
        if (argc > 1) {
            ObjectPtr lex = clone(global);
            ObjectPtr dyn = clone(global);
            std::string pathname = stripFilename(getExecutablePathname());
            readFile(pathname + "std/runner.lats", {lex, dyn}, state, reader);
            // TODO cmd args
            // TODO the garbage collector won't run in this case
            while (!isIdling(state))
                doOneStep(state, reader);
        } else {
            runREPL(global, state, reader);
        }
        break;
    }
    case RunMode::EXIT: {
        // Do nothing and exit
        break;
    }
    }

    return 0;
}
