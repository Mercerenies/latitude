
#include "Assembler.hpp"

std::vector<AsmType> argPush(Instr instr) {
    std::vector<AsmType> vec;
    // Is this horrible? Yes. Am I sorry? No.
    switch (instr) {
    case Instr::MOV:
        _V::ArgPush<typename _V::Necessary<Instr::MOV>::type>::push(vec);
        break;
    case Instr::PUSH:
        _V::ArgPush<typename _V::Necessary<Instr::PUSH>::type>::push(vec);
        break;
    case Instr::POP:
        _V::ArgPush<typename _V::Necessary<Instr::POP>::type>::push(vec);
        break;
    case Instr::GETL:
        _V::ArgPush<typename _V::Necessary<Instr::GETL>::type>::push(vec);
        break;
    case Instr::GETD:
        _V::ArgPush<typename _V::Necessary<Instr::GETD>::type>::push(vec);
        break;
    case Instr::ESWAP:
        _V::ArgPush<typename _V::Necessary<Instr::ESWAP>::type>::push(vec);
        break;
    case Instr::ECLR:
        _V::ArgPush<typename _V::Necessary<Instr::ECLR>::type>::push(vec);
        break;
    case Instr::ESET:
        _V::ArgPush<typename _V::Necessary<Instr::ESET>::type>::push(vec);
        break;
    case Instr::SYM:
        _V::ArgPush<typename _V::Necessary<Instr::SYM>::type>::push(vec);
        break;
    case Instr::NUM:
        _V::ArgPush<typename _V::Necessary<Instr::NUM>::type>::push(vec);
        break;
    case Instr::INT:
        _V::ArgPush<typename _V::Necessary<Instr::INT>::type>::push(vec);
        break;
    case Instr::FLOAT:
        _V::ArgPush<typename _V::Necessary<Instr::FLOAT>::type>::push(vec);
        break;
    case Instr::NSWAP:
        _V::ArgPush<typename _V::Necessary<Instr::NSWAP>::type>::push(vec);
        break;
    case Instr::CALL:
        _V::ArgPush<typename _V::Necessary<Instr::CALL>::type>::push(vec);
        break;
    case Instr::XCALL:
        _V::ArgPush<typename _V::Necessary<Instr::XCALL>::type>::push(vec);
        break;
    case Instr::XCALL0:
        _V::ArgPush<typename _V::Necessary<Instr::XCALL0>::type>::push(vec);
        break;
    case Instr::RET:
        _V::ArgPush<typename _V::Necessary<Instr::RET>::type>::push(vec);
        break;
    case Instr::CLONE:
        _V::ArgPush<typename _V::Necessary<Instr::CLONE>::type>::push(vec);
        break;
    case Instr::RTRV:
        _V::ArgPush<typename _V::Necessary<Instr::RTRV>::type>::push(vec);
        break;
    case Instr::RTRVD:
        _V::ArgPush<typename _V::Necessary<Instr::RTRVD>::type>::push(vec);
        break;
    case Instr::STR:
        _V::ArgPush<typename _V::Necessary<Instr::STR>::type>::push(vec);
        break;
    case Instr::SSWAP:
        _V::ArgPush<typename _V::Necessary<Instr::SSWAP>::type>::push(vec);
        break;
    case Instr::EXPD:
        _V::ArgPush<typename _V::Necessary<Instr::EXPD>::type>::push(vec);
        break;
    case Instr::MTHD:
        _V::ArgPush<typename _V::Necessary<Instr::MTHD>::type>::push(vec);
        break;
    case Instr::LOAD:
        _V::ArgPush<typename _V::Necessary<Instr::LOAD>::type>::push(vec);
        break;
    case Instr::SETF:
        _V::ArgPush<typename _V::Necessary<Instr::SETF>::type>::push(vec);
        break;
    case Instr::PEEK:
        _V::ArgPush<typename _V::Necessary<Instr::PEEK>::type>::push(vec);
        break;
    case Instr::SYMN:
        _V::ArgPush<typename _V::Necessary<Instr::SYMN>::type>::push(vec);
        break;
    case Instr::CPP:
        _V::ArgPush<typename _V::Necessary<Instr::CPP>::type>::push(vec);
        break;
    case Instr::BOL:
        _V::ArgPush<typename _V::Necessary<Instr::BOL>::type>::push(vec);
        break;
    case Instr::TEST:
        _V::ArgPush<typename _V::Necessary<Instr::TEST>::type>::push(vec);
        break;
    case Instr::BRANCH:
        _V::ArgPush<typename _V::Necessary<Instr::BRANCH>::type>::push(vec);
        break;
    case Instr::CCALL:
        _V::ArgPush<typename _V::Necessary<Instr::CCALL>::type>::push(vec);
        break;
    case Instr::CGOTO:
        _V::ArgPush<typename _V::Necessary<Instr::CGOTO>::type>::push(vec);
        break;
    case Instr::CRET:
        _V::ArgPush<typename _V::Necessary<Instr::CRET>::type>::push(vec);
        break;
    case Instr::WND:
        _V::ArgPush<typename _V::Necessary<Instr::WND>::type>::push(vec);
        break;
    case Instr::UNWND:
        _V::ArgPush<typename _V::Necessary<Instr::UNWND>::type>::push(vec);
        break;
    case Instr::THROW:
        _V::ArgPush<typename _V::Necessary<Instr::THROW>::type>::push(vec);
        break;
    case Instr::THROQ:
        _V::ArgPush<typename _V::Necessary<Instr::THROQ>::type>::push(vec);
        break;
    case Instr::ADDS:
        _V::ArgPush<typename _V::Necessary<Instr::ADDS>::type>::push(vec);
        break;
    case Instr::ARITH:
        _V::ArgPush<typename _V::Necessary<Instr::ARITH>::type>::push(vec);
        break;
    case Instr::THROA:
        _V::ArgPush<typename _V::Necessary<Instr::THROA>::type>::push(vec);
        break;
    case Instr::LOCFN:
        _V::ArgPush<typename _V::Necessary<Instr::LOCFN>::type>::push(vec);
        break;
    case Instr::LOCLN:
        _V::ArgPush<typename _V::Necessary<Instr::LOCLN>::type>::push(vec);
        break;
    case Instr::LOCRT:
        _V::ArgPush<typename _V::Necessary<Instr::LOCRT>::type>::push(vec);
        break;
    case Instr::NRET:
        _V::ArgPush<typename _V::Necessary<Instr::NRET>::type>::push(vec);
        break;
    case Instr::UNTR:
        _V::ArgPush<typename _V::Necessary<Instr::UNTR>::type>::push(vec);
        break;
    case Instr::CMPLX:
        _V::ArgPush<typename _V::Necessary<Instr::CMPLX>::type>::push(vec);
        break;
    case Instr::YLD:
        _V::ArgPush<typename _V::Necessary<Instr::YLD>::type>::push(vec);
        break;
    case Instr::YLDC:
        _V::ArgPush<typename _V::Necessary<Instr::YLDC>::type>::push(vec);
        break;
    case Instr::DEL:
        _V::ArgPush<typename _V::Necessary<Instr::DEL>::type>::push(vec);
        break;
    case Instr::ARR:
        _V::ArgPush<typename _V::Necessary<Instr::ARR>::type>::push(vec);
        break;
    case Instr::DICT:
        _V::ArgPush<typename _V::Necessary<Instr::DICT>::type>::push(vec);
        break;
    case Instr::XXX:
        _V::ArgPush<typename _V::Necessary<Instr::XXX>::type>::push(vec);
        break;
    case Instr::GOTO:
        _V::ArgPush<typename _V::Necessary<Instr::GOTO>::type>::push(vec);
            break;
    }
    return vec;
}

std::vector<AsmType> getAsmArguments(Instr instr) {
    return argPush(instr);
}

constexpr AsmType _V::ArgToEnum<_V::VLong>::value;
constexpr AsmType _V::ArgToEnum<_V::VString>::value;
constexpr AsmType _V::ArgToEnum<_V::VReg>::value;
constexpr AsmType _V::ArgToEnum<_V::VAsm>::value;
