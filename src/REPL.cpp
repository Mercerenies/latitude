#include "GC.hpp"
#include "REPL.hpp"
#include "Symbol.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Reader.hpp"
#include "Pathname.hpp"
#include "Precedence.hpp"
#include <string>

using namespace std;

void runREPL(ObjectPtr global, IntState& state, ReadOnlyState& reader) {

    OperatorTable table = getTable(global);

    string pathname = stripFilename(getExecutablePathname());
    readFile(pathname + "std/repl.lats", { clone(global), clone(global) }, state, reader, table);

    while (!isIdling(state))
        doOneStep(state, reader);
}

void runRunner(ObjectPtr global, IntState& state, ReadOnlyState& reader) {

    OperatorTable table = getTable(global);

    string pathname = stripFilename(getExecutablePathname());
    readFile(pathname + "std/runner.lats", { clone(global), clone(global) }, state, reader, table);

    while (!isIdling(state))
        doOneStep(state, reader);
}

void runCompiler(ObjectPtr global, IntState& state, ReadOnlyState& reader) {

    OperatorTable table = getTable(global);

    string pathname = stripFilename(getExecutablePathname());
    readFile(pathname + "std/compiler.lats", { clone(global), clone(global) }, state, reader, table);

    while (!isIdling(state))
        doOneStep(state, reader);
}
