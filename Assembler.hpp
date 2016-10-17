#ifndef _ASSEMBLER_HPP_
#define _ASSEMBLER_HPP_
#include "Instructions.hpp"
#include <type_traits>
#include <tuple>

namespace _V {

    struct VLong { typedef std::false_type is_register; };
    struct VString { typedef std::false_type is_register; };
    struct VReg { typedef std::true_type is_register; };
    struct VAsm { typedef std::false_type is_register; };

    // This fixes a weird syntactic issue in the cases for Unify<T, S>
    template <typename T>
    struct V : T {};

    template <typename T>
    struct Classify;
    template <>
    struct Classify<int> { typedef VLong type; };
    template <>
    struct Classify<long> { typedef VLong type; };
    template <>
    struct Classify<std::string> { typedef VString type; };
    template <>
    struct Classify<const char*> { typedef VString type; };
    template <>
    struct Classify<FunctionIndex> { typedef VAsm type; };
    template <>
    struct Classify<Reg> { typedef VReg type; };

    template <typename T, typename S>
    struct Unify;
    template <typename T>
    struct Unify<T, T> {};

    template <typename Tuple0, typename Tuple1>
    struct UnifyEach;
    template <>
    struct UnifyEach<std::tuple<>, std::tuple<>> {};
    template <typename T, typename S, typename... Ts, typename... Ss>
    struct UnifyEach<std::tuple<T, Ts...>, std::tuple<S, Ss...>> {
        Unify<T, S>* u0;
        UnifyEach<std::tuple<Ts...>, std::tuple<Ss...>>* u1;
    };

    template <Instr value>
    struct Necessary;
    template <>
    struct Necessary<Instr::MOV> { typedef std::tuple<VReg, VReg> type; };
    template <>
    struct Necessary<Instr::PUSH> { typedef std::tuple<VReg, VReg> type; };
    template <>
    struct Necessary<Instr::POP> { typedef std::tuple<VReg> type; };
    template <>
    struct Necessary<Instr::GETL> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::GETD> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::ESWAP> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::ECLR> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::ESET> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::SYM> { typedef std::tuple<VString> type; };
    template <>
    struct Necessary<Instr::NUM> { typedef std::tuple<VString> type; };
    template <>
    struct Necessary<Instr::INT> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::FLOAT> { typedef std::tuple<VString> type; };
    template <>
    struct Necessary<Instr::NSWAP> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::CALL> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::XCALL> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::XCALL0> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::RET> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::CLONE> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::RTRV> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::RTRVD> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::STR> { typedef std::tuple<VString> type; };
    template <>
    struct Necessary<Instr::SSWAP> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::EXPD> { typedef std::tuple<VReg> type; };
    template <>
    struct Necessary<Instr::MTHD> { typedef std::tuple<VAsm> type; };
    template <>
    struct Necessary<Instr::LOAD> { typedef std::tuple<VReg> type; };
    template <>
    struct Necessary<Instr::SETF> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::PEEK> { typedef std::tuple<VReg> type; };
    template <>
    struct Necessary<Instr::SYMN> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::CPP> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::BOL> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::TEST> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::BRANCH> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::CCALL> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::CGOTO> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::CRET> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::WND> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::UNWND> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::THROW> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::THROQ> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::ADDS> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::ARITH> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::THROA> { typedef std::tuple<VString> type; };
    template <>
    struct Necessary<Instr::LOCFN> { typedef std::tuple<VString> type; };
    template <>
    struct Necessary<Instr::LOCLN> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::LOCRT> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::NRET> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::UNTR> { typedef std::tuple<> type; };

}

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

/*
 * The CompileTimeAsmChecker (usually invoked using the makeAssemblerLine macro) performs
 * type checking for the arguments of the assembly instruction at compile-time rather than
 * using runtime validation. Note that the compile-time validation is slightly weaker than
 * the runtime check, in that it does not check the specific register arguments, only that
 * they are register arguments. However, its performance is significantly better than that
 * of the runtime checker.
 */

#define makeAssemblerLine(I, ...) (CompileTimeAsmChecker<(I)>::invoke(__VA_ARGS__))

template <Instr instr>
struct CompileTimeAsmChecker {
    template <typename... Ts>
    static AssemblerLine invoke(Ts... args) {
        typedef typename _V::Necessary<instr>::type parms_t;
        typedef std::tuple<typename _V::Classify<Ts>::type...> args_t;
        _V::UnifyEach<args_t, parms_t> unify;
        unify = unify;
        auto line = makeAssemblerLineUnchecked(instr, args...);
        return line;
    };
};

template <typename... Ts>
InstrSeq asmCode(Ts... args) {
    InstrSeq seq;
    std::array<AssemblerLine, sizeof...(args)> args0 = { args... };
    for (auto& arg : args0)
        arg.appendOnto(seq);
    return seq;
}

#endif
