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
#include "Bytecode.hpp"
#include "REPL.hpp"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {

    // TODO Teach the new REPL to call the garbage collector
    // TODO Have the new REPL perform the entire print operation in the assembler, as opposed
    //      to having the system print out a toString'd form.
    IntState state = intState();
    ObjectPtr global = spawnObjectsNew(state);

    bool first = true;
    string current;
    while (true) {
        while (!isIdling(state))
            doOneStep(state);
        auto str = boost::get<string>(&state.ret.lock()->prim());
        if (!first) {
            if (str)
                cout << *str << endl;
            else
                cout << "(Invalid toString)" << endl;
        }
        first = false;
        cout << "> ";
        getline(cin, current);
        if (current == "quit.") {
            break;
        } else {
            evalNew(state, current);
            if (!state.stack.empty()) {
                // Append the toString instructions
                InstrSeq& seq = state.stack.top();
                seq = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                              makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO),
                              makeAssemblerLine(Instr::SYM, "toString"),
                              makeAssemblerLine(Instr::RTRV),
                              makeAssemblerLine(Instr::POP, Reg::STO),
                              makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                              makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                              makeAssemblerLine(Instr::CALL, 0L));
            }
        }
        // TODO Make it print the result
    }

    /*
    ObjectPtr global;
    try {
        try {
            try {
                global = spawnObjects();
            } catch (ProtoError& err) {
                cerr << "An error occurred while loading the standard library. "
                     << "Please report this." << endl;
                auto stream = errStream();
                stream->writeLine("*** STD EXCEPTION ***");
                dumpObject({ err.getObject(), err.getObject() }, *stream, err.getObject());
                return 1;
            }
            runREPL(global);
        } catch (ProtoError& err) {
            auto stream = errStream();
            stream->writeLine("*** TOPLEVEL EXCEPTION ***");
            //auto bloop = getInheritedSlot({ global, global }, err.getObject(), Symbols::get()["message"]);
            //auto bleep = getInheritedSlot({ global, global }, err.getObject(), Symbols::get()["slotName"]);
            //cerr << boost::get<string>(bloop.lock()->prim()) << endl;
            //cerr << Symbols::get()[boost::get<Symbolic>(bleep.lock()->prim())] << endl;
            global.lock()->put(Symbols::get()["$except"], err.getObject());
            dumpObject({ global, global }, *stream, err.getObject());
            return 1;
        }
    } catch (...) {
        return 1;
    }
    */

    return 0;
}
