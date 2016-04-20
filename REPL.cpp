#include "GC.hpp"
#include "REPL.hpp"
#include "Symbol.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Reader.hpp"

using namespace std;

ObjectPtr spawnREPLObjects(ObjectPtr& global) {
    ObjectPtr object = getInheritedSlot(meta(global), Symbols::get()["Object"]);
    ObjectPtr repl = clone(object);

    repl.lock()->put(Symbols::get()["toString"],
                     eval("\"REPL\".", global, global));

    global.lock()->put(Symbols::get()["REPL"], repl);

    return repl;
}

ObjectPtr spawnREPLObjects(ObjectPtr& global, ObjectPtr& cont) {
    ObjectPtr repl = spawnREPLObjects(global);

    repl.lock()->put(Symbols::get()["quitter"], cont);
    repl.lock()->put(Symbols::get()["quit"],
                     eval("{ self quitter call: meta Nil. }.",
                          global, global));
    repl.lock()->put(Symbols::get()["exception"], eval("meta Nil.", global, global));
    repl.lock()->put(Symbols::get()["lastResult"], eval("meta Nil.", global, global));
    global.lock()->put(Symbols::get()["quit"],
                       eval("{ self REPL quit. }.",
                            global, global));
    global.lock()->put(Symbols::get()["$except"], eval("{ self REPL exception. }.", global, global));
    global.lock()->put(Symbols::get()["$it"], eval("{ self REPL lastResult. }.", global, global));

    return repl;
}

void runREPL(ObjectPtr& global) {
    // Simulate a callCC
    shared_ptr<SignalValidator> livingTag(new SignalValidator());
    Symbolic sym = Symbols::gensym("QUIT");
    ObjectPtr symObj = getInheritedSlot(meta(global),
                                        Symbols::get()["Symbol"]);
    ObjectPtr symObj1 = clone(symObj);
    symObj1.lock()->prim(sym);
    ObjectPtr validator = getInheritedSlot(meta(global),
                                           Symbols::get()["ContValidator"]);
    ObjectPtr validator1 = clone(validator);
    validator1.lock()->prim(weak_ptr<SignalValidator>(livingTag));
    global.lock()->put(sym, validator1);
    ObjectPtr cont = getInheritedSlot(meta(global),
                                      Symbols::get()["Cont"]);
    ObjectPtr cont1 = clone(cont);
    cont1.lock()->put(Symbols::get()["tag"], symObj1);

    ObjectPtr repl = spawnREPLObjects(global, cont1);
    try {
        auto out = outStream();
        ObjectPtr result;
        string current;
        while (true) {
            try {
                try {
                    // TODO Allow multi-line statements in REPL
                    cout << "> ";
                    getline(cin, current);
                    result = eval(current, global, global);
                    repl.lock()->put(Symbols::get()["lastResult"], result);
                    simplePrintObject(global, global, *out, result);
                } catch (std::string parseException) {
                    throw doParseError(global, parseException);
                }
                // TODO Don't garbage collect at every step
                GC::get().garbageCollect(global);
            } catch (ProtoError& err) {
                auto stream = errStream();
                stream->writeLine("*** EXCEPTION ***");
                repl.lock()->put(Symbols::get()["exception"], err.getObject());
                dumpObject(global, global, *stream, err.getObject());
            }
        }
    } catch (Signal& signal) {
        if (signal.match(sym)) {
            cout << "Goodbye." << endl;
        } else {
            throw;
        }
    }
}
