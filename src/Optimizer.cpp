//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details


#include "Optimizer.hpp"
#include "Symbol.hpp"

#include <iostream>

namespace optimize {

    void lookupSymbols(TranslationUnitPtr unit) {
        for (int i = 0; i < unit->methodCount(); i++) {
            InstrSeq& seq { unit->method(i) };
            for (AssemblerLine& line : seq) {
                if (line.getCommand() == Instr::SYM) {
                    // Assume the instruction is valid, so its first argument is a string.
                    auto sym = boost::get<std::string>(line.argument(0));
                    line.setCommand(Instr::SYMN);
                    line.clearRegisterArgs();
                    line.addRegisterArg(Symbols::get()[sym].index);
                }
            }
        }
    }

}
