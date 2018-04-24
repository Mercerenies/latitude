#ifndef ARGS_HPP
#define ARGS_HPP

enum class RunMode {
    DEFAULT,
    EXIT
};

enum class OutputMode {
    NONE,
    VERSION,
    HELP
};

struct CmdArgs {
    RunMode run;
    OutputMode output;
};

CmdArgs parseArgs(int& argc, char**& argv);

void outputHelp();

void outputVersion();

#endif // ARGS_HPP
