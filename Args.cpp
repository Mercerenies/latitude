
#include "Args.hpp"
#include <cstring>
#include <iostream>

CmdArgs parseArgs(int& argc, char**& argv) {
    int len = argc; // Copied since we're going to update argc
    int j = 1;
    CmdArgs result;

    result.run = RunMode::DEFAULT;
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
    std::cout << "Usage: latitude [options...] [filename] [args...]" << std::endl;
    std::cout << "  --help     Show this message and exit" << std::endl;
    std::cout << "  --version  Print the current version and exit" << std::endl;
    std::cout << "If a filename is provided, that file will be executed," << std::endl;
    std::cout << "with the given command line arguments. If no additional" << std::endl;
    std::cout << "arguments are supplied, a REPL will be started." << std::endl;
}

void outputVersion() {
    // This will print an actual version number once the language has been
    // formally released. For now, it's a placeholder string.
    std::cout << "Latitude [Development Version]" << std::endl;
}
