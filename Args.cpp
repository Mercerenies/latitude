
#include "Args.hpp"
#include <cstring>
#include <iostream>

CmdArgs parseArgs(int& argc, char**& argv) {
    int len = argc; // Copied since we're going to update argc
    int j = 1;
    CmdArgs result;

    result.run = RunMode::REPL;
    result.output = OutputMode::NONE;

    for (int i = 1; i < len; i++) {
        if (std::strcmp(argv[i], "--version") == 0) {
            // Print version and exit
            result.run = RunMode::EXIT;
            result.output = OutputMode::VERSION;
            argc--;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            // Print help text and exit
            result.run = RunMode::EXIT;
            result.output = OutputMode::HELP;
            argc--;
        } else {
            // Unrecognized command, so keep it
            argv[j++] = argv[i];
        }
    }

    return result;
}

void outputHelp() {
    std::cout << "<help text>" << std::endl;
}
