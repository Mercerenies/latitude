//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

#include "Args.hpp"
#include "Proto.hpp"
#include "Bytecode.hpp"

ObjectPtr* globalScope;
VMState* globalVM;

int main(int argc, char** argv) {
  initRandom();

  ObjectPtr global;
  VMState vm { VMState::createAndInit(&global, argc, argv) };

  globalScope = &global;
  globalVM = &vm;

  int result = Catch::Session().run(argc, argv);

  return result;

}
