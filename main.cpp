
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
                xx := Object clone.
                xx put: 'abc, 1.
                xx put: ~abc, 1.
                stdout println: (xx has: 'abc).
                stdout println: (xx has: ~abc).
                stdout println: 'abc.
                stdout println: Symbol gensym.
                stdout println: Symbol gensym.
                stdout println: Symbol gensym.
                stdout println: Symbol gensym.
            }) me.)",
         global, global);
    auto stream = outStream();
    GC::get().garbageCollect(global);

    return 0;
}
