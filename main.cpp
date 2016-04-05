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
              xx := Object clone....
              xx a := 1.
              xx c := 3.
              ({
                stdout println: xx a.
                stdout println: xx b.
                stdout println: xx c.
              }) catch: SlotError, {
                stdout putln: "Bazinga".
              }.
            }) me.)",
             global, global);
        GC::get().garbageCollect(global);
    } catch (ProtoError& e) {
        auto stream = errStream();
        stream->writeLine("*** UNCAUGHT EXCEPTION ***");
        dumpObject(global, global, *stream, e.getObject());
    }

    return 0;
}
