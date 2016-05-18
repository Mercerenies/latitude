#include "GC.hpp"
#include "REPL.hpp"
#include "Symbol.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Reader.hpp"

using namespace std;

void runREPL(ObjectPtr global, IntState& state) {
    readFile("std/repl.lats", { global, global }, state);
    while (!isIdling(state))
        doOneStep(state);
}
