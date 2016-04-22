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
#include "REPL.hpp"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {

    ObjectPtr global;
    try {
        global = spawnObjects();
    } catch (ProtoError& err) {
        cerr << "An error occurred while loading the standard library. "
             << "Please report this." << endl;
        if (hasInheritedSlot(err.getObject(), Symbols::get()["message"])) {
            ObjectPtr thing = getInheritedSlot(err.getObject(), Symbols::get()["message"]);
            if (auto str = boost::get<string>(&thing.lock()->prim()))
                cerr << "Details: " << *str << endl;
        }
        if (hasInheritedSlot(err.getObject(), Symbols::get()["slotName"])) {
            ObjectPtr thing = getInheritedSlot(err.getObject(), Symbols::get()["slotName"]);
            if (auto str = boost::get<string>(&thing.lock()->prim()))
                cerr << "Details: " << *str << endl;
        }
        return 1;
    }
    try {
        runREPL(global);
    } catch (ProtoError& err) {
        auto stream = errStream();
        stream->writeLine("*** TOPLEVEL EXCEPTION ***");
        //global.lock()->put(Symbols::get()["$except"], err.getObject());
        dumpObject(global, global, *stream, err.getObject());
        return 1;
    }

    return 0;
}
