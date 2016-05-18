#include "GC.hpp"
#include "REPL.hpp"
#include "Symbol.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Reader.hpp"

using namespace std;

void runREPL(ObjectPtr global, IntState& state) {
    // Run all the "standard library" loading code first
    while (!isIdling(state))
        doOneStep(state);
    readFile("std/repl.lat", { global, global }, state);
    while (!isIdling(state))
        doOneStep(state);
    eval(state, "REPL loop.");
    while (!isIdling(state))
        doOneStep(state);
}
