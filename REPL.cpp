#include "GC.hpp"
#include "REPL.hpp"
#include "Symbol.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Reader.hpp"
#include "Pathname.hpp"
#include <string>

using namespace std;

void runREPL(ObjectPtr global, IntState& state) {

    cout << "Latitude ALPHA" << endl;

    string pathname = stripFilename(getExecutablePathname());
    readFile(pathname + "std/repl.lats", { clone(global), clone(global) }, state);

    while (!isIdling(state))
        doOneStep(state);

}
