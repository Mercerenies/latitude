
extern "C" {
#include "lex.yy.h"
#include "Parser.tab.h"
}
#include "Reader.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "GC.hpp"
#include "Symbol.hpp"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {

    auto global = spawnObjects();
    eval(R"(({
              stdout println: (True is: Boolean).
              stdout println: (True is: Symbol).
            }) me.)",
         global, global);
    auto stream = outStream();
    auto val = getInheritedSlot(global, Symbols::get()["Global"]);
    GC::get().garbageCollect(global);

    return 0;
}
