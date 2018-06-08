#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP
#include "Instructions.hpp"
#include "Base.hpp"
#include <type_traits>
#include <tuple>
#include <vector>

/// \file
///
/// \brief Argument type-checking for assembler-level VM instructions.

/// This enum lists the different types of arguments that can be
/// provided to assembler instructions, in a form accessible at
/// runtime.
enum class AsmType {
    LONG,
    STRING,
    REG,
    ASM
};

/// \cond

// Enter this namespace at your own risk. Good luck, my friend. :)
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
    struct Unify<T, T> {
        typedef char type;
    };

    template <typename Tuple0, typename Tuple1>
    struct UnifyEach;
    template <>
    struct UnifyEach<std::tuple<>, std::tuple<>> {
        typedef char type;
    };
    template <typename T, typename S, typename... Ts, typename... Ss>
    struct UnifyEach<std::tuple<T, Ts...>, std::tuple<S, Ss...>> {
        typedef typename Unify<T, S>::type _unify;
        typedef typename UnifyEach<std::tuple<Ts...>, std::tuple<Ss...>>::type _each;
        typedef std::tuple<_unify, _each> type;
    };

    template <Instr value>
    struct Necessary;
    template <>
    struct Necessary<Instr::MOV> { typedef std::tuple<VReg, VReg> type; };
    template <>
    struct Necessary<Instr::PUSH> { typedef std::tuple<VReg, VReg> type; };
    template <>
    struct Necessary<Instr::POP> { typedef std::tuple<VReg, VReg> type; };
    template <>
    struct Necessary<Instr::GETL> { typedef std::tuple<VReg> type; };
    template <>
    struct Necessary<Instr::GETD> { typedef std::tuple<VReg> type; };
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
    struct Necessary<Instr::PEEK> { typedef std::tuple<VReg, VReg> type; };
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
    template <>
    struct Necessary<Instr::CMPLX> { typedef std::tuple<VString, VString> type; };
    template <>
    struct Necessary<Instr::YLD> { typedef std::tuple<VLong, VReg> type; };
    template <>
    struct Necessary<Instr::YLDC> { typedef std::tuple<VLong, VReg> type; };
    template <>
    struct Necessary<Instr::DEL> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::ARR> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::DICT> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::XXX> { typedef std::tuple<VLong> type; };
    template <>
    struct Necessary<Instr::GOTO> { typedef std::tuple<> type; };
    template <>
    struct Necessary<Instr::MSWAP> { typedef std::tuple<> type; };

    template <typename T>
    struct ArgToEnum;

    template <>
    struct ArgToEnum<VLong> { constexpr static AsmType value = AsmType::LONG; };
    template <>
    struct ArgToEnum<VString> { constexpr static AsmType value = AsmType::STRING; };
    template <>
    struct ArgToEnum<VReg> { constexpr static AsmType value = AsmType::REG; };
    template <>
    struct ArgToEnum<VAsm> { constexpr static AsmType value = AsmType::ASM; };

    template <typename T>
    struct ArgPush;

    template <>
    struct ArgPush<std::tuple<>> {
        static void push(std::vector<AsmType>&) {}
    };

    template <typename T, typename... Ts>
    struct ArgPush<std::tuple<T, Ts...>> {
        static void push(std::vector<AsmType>& vec) {
            vec.push_back(ArgToEnum<T>::value);
            ArgPush<std::tuple<Ts...>>::push(vec);
        }
    };

}

/// \endcond

/// This procedure compiles an assembler line with the given
/// arguments, ignoring all of the coherence conditions defined. This
/// should only be used internally except in very special
/// circumstances, and even then it should be used very sparingly and
/// carefully.
///
/// \param instr a VM instruction
/// \param args the arguments to provide to the instruction
/// \return a compiled assembler line
template <typename... Ts>
AssemblerLine makeAssemblerLineUnchecked(Instr instr, Ts... args) {
    AssemblerLine line;
    line.setCommand(instr);
    std::array<RegisterArg, sizeof...(args)> args0 = { args... };
    for (auto& arg : args0)
        line.addRegisterArg(arg);
    return line;
}

/// This procedure bypasses the compile-time argument checks, instead
/// opting to validate the assembler line at runtime. This is safer
/// than #makeAssemblerLineUnchecked but slower than
/// #makeAssemblerLine. The #makeAssemblerLine macro should be
/// preferred when compile-time checking is possible.
///
/// \param instr a VM instruction
/// \param args the arguments to provide to the instruction
/// \return a compiled assembler line
/// \throw AssemblerError if the compiled line contains an integrity error
template <typename... Ts>
AssemblerLine makeRuntimeAssemblerLine(Instr instr, Ts... args) {
    auto line = makeAssemblerLineUnchecked(instr, args...);
    line.validate();
    return line;
}

/// The CompileTimeAsmChecker (usually invoked using the
/// #makeAssemblerLine macro) performs type checking for the arguments
/// of the assembly instruction at compile-time rather than using
/// runtime validation. Note that the compile-time validation is
/// slightly weaker than the runtime check, in that it does not check
/// the specific register arguments, only that they are register
/// arguments. However, its performance is significantly better than
/// that of the runtime checker.
///
/// \param I the instruction
/// \param ... the arguments
/// \return a compiled assembler line
#define makeAssemblerLine(I, ...) (CompileTimeAsmChecker<(I)>::invoke(__VA_ARGS__))

/// \brief Static type checking for instruction arguments.
///
/// This structure is used statically to perform type checking for the
/// arguments given to an assembly instruction at compile-time rather
/// than using runtime validation.
///
/// \tparam instr the instruction to compile
/// \see makeAssemblerLine
template <Instr instr>
struct CompileTimeAsmChecker {
    /// Given an instruction known at compile-time and a sequence of
    /// argments, this static function returns the AssemblerLine
    /// constructed as a result, as though by
    /// #makeAssemblerLineUnchecked. However, calls to this function
    /// will only typecheck if the arguments are correct for the
    /// instruction.
    ///
    /// \tparam Ts the argument types
    /// \param args the arguments to provide to the instruction
    /// \return a compiled assembler line
    template <typename... Ts>
    static AssemblerLine invoke(Ts... args) {
        typedef typename _V::Necessary<instr>::type parms_t;
        typedef std::tuple<typename _V::Classify<Ts>::type...> args_t;
        _V::V<typename _V::UnifyEach<args_t, parms_t>::type>* value = NULL;
        value = value;
        auto line = makeAssemblerLineUnchecked(instr, args...);
        return line;
    };
};

/// Given a sequence of AssemblerLine objects, this function returns
/// an InstrSeq object containing all of the given lines compiled
/// together.
///
/// \tparam Ts the argument types
/// \param args the assembler lines
/// \return an InstrSeq containing the lines
template <typename... Ts>
InstrSeq asmCode(Ts... args) {
    InstrSeq seq;
    std::array<AssemblerLine, sizeof...(args)> args0 = { args... };
    for (auto& arg : args0)
        arg.appendOnto(seq);
    return seq;
}

/// Returns a vector containing all of the argument types expected by
/// the given instruction.
///
/// \param instr the instruction
/// \return a list of arguments
std::vector<AsmType> getAsmArguments(Instr instr);

/// Given an AsmType as an argument, calls the visitor with the
/// appropriate type passed as a template argument.
///
/// The visitor should have an operator() which takes a proxy argument
/// for each of the following proxy types.
///
/// * Proxy<long>
/// * Proxy<std::string>
/// * Proxy<Reg>
/// * Proxy<FunctionIndex>
///
/// \param visitor the visitor
/// \param type the type of argument
template <typename Visitor>
auto callOnAsmArgType(Visitor& visitor, AsmType type)
    -> decltype(visitor(Proxy<long>())) {
    switch (type) {
    case AsmType::LONG:
        return visitor(Proxy<long>());
    case AsmType::STRING:
        return visitor(Proxy<std::string>());
    case AsmType::REG:
        return visitor(Proxy<Reg>());
    case AsmType::ASM:
        return visitor(Proxy<AsmType>());
    default:
        assert(false);
    }
}

#endif // ASSEMBLER_HPP
