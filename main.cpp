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
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {

    IntState state = intState();
    ObjectPtr global = spawnObjects(state);
    runREPL(global, state);

    return 0;
}
