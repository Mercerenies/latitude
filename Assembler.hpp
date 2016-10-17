#ifndef _ASSEMBLER_HPP_
#define _ASSEMBLER_HPP_
#include "Instructions.hpp"
#include <type_traits>

template <typename... Ts>
AssemblerLine makeAssemblerLineUnchecked(Instr instr, Ts... args) {
    AssemblerLine line;
    line.setCommand(instr);
    std::array<RegisterArg, sizeof...(args)> args0 = { args... };
    for (auto& arg : args0)
        line.addRegisterArg(arg);
    return line;
}

template <typename... Ts>
AssemblerLine makeRuntimeAssemblerLine(Instr instr, Ts... args) {
    auto line = makeAssemblerLineUnchecked(instr, args...);
    line.validate();
    return line;
}

template <typename... Ts>
AssemblerLine makeAssemblerLine(const Instr instr, Ts... args) {
    auto line = makeAssemblerLineUnchecked(instr, args...);
    line.validate();
    return line;
}

template <typename... Ts>
InstrSeq asmCode(Ts... args) {
    InstrSeq seq;
    std::array<AssemblerLine, sizeof...(args)> args0 = { args... };
    for (auto& arg : args0)
        arg.appendOnto(seq);
    return seq;
}

#endif
