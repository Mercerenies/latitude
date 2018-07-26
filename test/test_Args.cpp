//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "catch2/catch.hpp"
#include "Args.hpp"
#include <string>

// We're doing *slightly* dangerous const_casts here to mock argv.
// It's fine since parseArgs never actually modifies the underlying
// strings, merely their order.

TEST_CASE( "parseArgs removes parsed arguments", "" ) {

  int argc = 4;
  const char* argv[] { "EXE_NAME", "--version", "extra_argument1", "extra_argument2" };
  char** argv1 = const_cast<char**>(argv);

  parseArgs(argc, argv1);
  REQUIRE( argc == 3 );
  REQUIRE( std::string(argv[0]) == "EXE_NAME" );
  REQUIRE( std::string(argv[1]) == "extra_argument1" );
  REQUIRE( std::string(argv[2]) == "extra_argument2" );

}

TEST_CASE( "parseArgs returns the appropriate values", "" ) {

  SECTION( "--version" ) {
    int argc = 2;
    const char* argv[] { "EXE_NAME", "--version" };
    char** argv1 = const_cast<char**>(argv);

    CmdArgs result = parseArgs(argc, argv1);
    REQUIRE( argc == 1 );
    REQUIRE( result.run == RunMode::EXIT );
    REQUIRE( result.output == OutputMode::VERSION );
  }

  SECTION( "--help" ) {
    int argc = 2;
    const char* argv[] { "EXE_NAME", "--help" };
    char** argv1 = const_cast<char**>(argv);

    CmdArgs result = parseArgs(argc, argv1);
    REQUIRE( argc == 1 );
    REQUIRE( result.run == RunMode::EXIT );
    REQUIRE( result.output == OutputMode::HELP );
  }

  SECTION( "--compile" ) {
    int argc = 2;
    const char* argv[] { "EXE_NAME", "--compile" };
    char** argv1 = const_cast<char**>(argv);

    CmdArgs result = parseArgs(argc, argv1);
    REQUIRE( argc == 1 );
    REQUIRE( result.run == RunMode::COMPILE );
    REQUIRE( result.output == OutputMode::NONE );
  }

  SECTION( "No arguments" ) {
    int argc = 1;
    const char* argv[] { "EXE_NAME" };
    char** argv1 = const_cast<char**>(argv);

    CmdArgs result = parseArgs(argc, argv1);
    REQUIRE( argc == 1 );
    REQUIRE( result.run == RunMode::REPL );
    REQUIRE( result.output == OutputMode::NONE );
  }

  SECTION( "Filename argument" ) {
    int argc = 2;
    const char* argv[] { "EXE_NAME", "dummy_filename.lats" };
    char** argv1 = const_cast<char**>(argv);

    CmdArgs result = parseArgs(argc, argv1);
    REQUIRE( argc == 2 );
    REQUIRE( result.run == RunMode::RUNNER );
    REQUIRE( result.output == OutputMode::NONE );
  }

}

