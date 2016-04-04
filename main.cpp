extern "C" {
#include "lex.yy.h"
#include "Parser.tab.h"
}
#include "Reader.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "GC.hpp"
#include "Symbol.hpp"
#include "Cont.hpp"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {

    auto global = spawnObjects();
    try {
        eval(R"(({
              stdout println: ({}) me.
            }) me.)",
             global, global);
        auto stream = outStream();
        auto val = getInheritedSlot(global, Symbols::get()["Global"]);
        GC::get().garbageCollect(global);
    } catch (ProtoError& e) {
        auto stream = errStream();
        stream->writeLine("*** UNCAUGHT EXCEPTION ***");
        dumpObject(global, global, *stream, e.getObject());
    }

    return 0;
}
