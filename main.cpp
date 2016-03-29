
extern "C" {
#include "lex.yy.h"
#include "Parser.tab.h"
}
#include "Reader.hpp"
#include "Standard.hpp"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {

    auto global = spawnObjects();
    eval(R"(x := {
              if: True, {stdout putln: "TRUE".}, {stdout putln: "FALSE".}.
              if: False, {stdout putln: "TRUE".}, {stdout putln: "FALSE".}.
            }.)",
         global, global);
    auto xx = getInheritedSlot(global, "x");
    eval("x.", global, global);

    return 0;
}
