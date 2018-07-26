//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef ARGS_HPP
#define ARGS_HPP

/// \file
///
/// \brief Parsing tools and structures for command line arguments.

/// \brief The run mode determines what the Latitude VM will do
/// after starting up.
enum class RunMode {

    /// Enter a read-eval-print loop, into which the user can input
    /// latitude expression.
    REPL,

    /// Look at the first command line argument and run the file with
    /// that name as a Latitude script.
    RUNNER,

    /// Look at the first command line argument and compile (but do
    /// not run) the file with that name as a Latitude script.
    COMPILE,

    /// Exit immediately. Used for printing out the version or help
    /// text and then exiting.
    EXIT

};

/// \brief The output mode determines what information is printed.
///
/// The output mode determines what information is printed after the
/// Latitude VM is started but before it performs its run mode.
enum class OutputMode {

    /// Print no information.
    NONE,

    /// Print the Latitude version.
    VERSION,

    /// Print the basic help text.
    HELP

};

/// The result of parsing command line arguments.
struct CmdArgs {
    RunMode run;
    OutputMode output;
};

/// Initializes the C random number generator by calling `srand` and
/// providing it with some seed, such as the current time.
void initRandom();

/// Parses the command line arguments into a CmdArgs instance. Any
/// arguments which are used by this procedure are consumed and
/// removed from the argument list.
///
/// \param argc the number of arguments
/// \param argv the command line arguments themselves
/// \return the parsed CmdArgs object
CmdArgs parseArgs(int& argc, char**& argv);

/// \brief Outputs, to stdout, help text for correctly invoking the
/// executable.
void outputHelp();

/// \brief Outputs, to stdout, the Latitude version header.
void outputVersion();

#endif // ARGS_HPP
