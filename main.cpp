
extern "C" {
#include "lex.yy.h"
#include "Parser.tab.h"
}
#include "Reader.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {

    auto global = spawnObjects();
    eval(R"(({
              True ifTrue: {stdout putln: "YAY!".}.
              True ifFalse: {stdout putln: "Uhhhhhh".}.
              False ifFalse: {stdout putln: "YAY!".}.
              False ifTrue: {stdout putln: "Uhhhhhh".}.
            }) me.)",
         global, global);
    auto stream = outStream();

    return 0;
}
