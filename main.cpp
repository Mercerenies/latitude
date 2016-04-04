
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
                stdout println: (callCC: {
                    $1 call: 1.
                    2.
                  }).
                stdout println: (callCC: {
                    1.
                    2.
                  }).
                stdout dump: True.
            }) me.)",
         global, global);
    auto stream = outStream();
    auto val = getInheritedSlot(global, Symbols::get()["Global"]);
    GC::get().garbageCollect(global);

    auto expr0 = eval("True toBool.", global, global);
    auto expr1 = eval("False toBool.", global, global);
    dumpObject(global, global, *stream, expr0);
    dumpObject(global, global, *stream, expr1);
    auto parents = hierarchy(expr0);
    for (auto x : parents)
        dumpObject(global, global, *stream, x);

    return 0;
}

///// Garbage collector isn't working; after that, test hierarchy()
