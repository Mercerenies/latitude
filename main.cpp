
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

    Symbolic gg = Symbols::gensym();
    Symbolic hh = Symbols::gensym();
    cout << gg.index << " " << hh.index << " " << Symbols::get()[gg.index]
         << " " << Symbols::get()[hh.index] << endl;

    auto global = spawnObjects();
    eval(R"(({
              xx := proc { stdout println: "I am printing.". }.
              xx call.
              stdout println: (xx has: 'call).
              stdout println: (xx has: 'mall).
            }) me.)",
         global, global);
    auto stream = outStream();
    GC::get().garbageCollect(global);

    return 0;
}
