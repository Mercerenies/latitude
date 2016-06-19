
#include "Instructions.hpp"

InstructionSet InstructionSet::iset;

InstructionSet& InstructionSet::getInstance() {
    if (!iset.initialized)
        iset.initialize();
    return iset;
}

auto InstructionSet::hasInstruction(Instr instr) -> bool {
    return (props.find(instr) != props.end());
}

auto InstructionSet::getParams(Instr instr) -> ValidList {
    return props[instr];
}

void InstructionSet::initialize() {
    initialized = true;
    props[Instr::MOV] = { isObjectRegister, isObjectRegister };
    props[Instr::PUSH] = { isObjectRegister, isStackRegister };
    props[Instr::POP] = { isStackRegister };
    props[Instr::GETL] = { };
    props[Instr::GETD] = { };
    props[Instr::ESWAP] = { };
    props[Instr::ECLR] = { };
    props[Instr::ESET] = { };
    props[Instr::SYM] = { isStringRegisterArg };
    props[Instr::NUM] = { isStringRegisterArg };
    props[Instr::INT] = { isLongRegisterArg };
    props[Instr::FLOAT] = { isStringRegisterArg };
    props[Instr::NSWAP] = { };
    props[Instr::CALL] = { isLongRegisterArg };
    props[Instr::XCALL] = { };
    props[Instr::XCALL0] = { isLongRegisterArg };
    props[Instr::RET] = { };
    props[Instr::CLONE] = { };
    props[Instr::RTRV] = { };
    props[Instr::RTRVD] = { };
    props[Instr::STR] = { isStringRegisterArg };
    props[Instr::SSWAP] = { };
    props[Instr::EXPD] = { isRegister };
    props[Instr::MTHD] = { isAsmRegisterArg };
    props[Instr::LOAD] = { isRegister };
    props[Instr::SETF] = { };
    props[Instr::PEEK] = { isStackRegister };
    props[Instr::SYMN] = { isLongRegisterArg };
    props[Instr::CPP] = { isLongRegisterArg };
    props[Instr::BOL] = { };
    props[Instr::TEST] = { };
    props[Instr::BRANCH] = { };
    props[Instr::CCALL] = { };
    props[Instr::CGOTO] = { };
    props[Instr::CRET] = { };
    props[Instr::WND] = { };
    props[Instr::UNWND] = { };
    props[Instr::THROW] = { };
    props[Instr::THROQ] = { };
    props[Instr::ADDS] = { };
    props[Instr::ARITH] = { isLongRegisterArg };
    props[Instr::THROA] = { isStringRegisterArg };
    props[Instr::LOCFN] = { isStringRegisterArg };
    props[Instr::LOCLN] = { isLongRegisterArg };
    props[Instr::LOCRT] = { };
    props[Instr::NRET] = { };
}

bool isRegister(const RegisterArg& arg) {
    return (bool)(boost::get<Reg>(&arg));
}

bool isObjectRegister(const RegisterArg& arg) {
    if (const Reg* reg = boost::get<Reg>(&arg)) {
        if ((unsigned char)(*reg) <= 0x03)
            return true;
        return false;
    }
    return false;
}

bool isStackRegister(const RegisterArg& arg) {
    if (const Reg* reg = boost::get<Reg>(&arg)) {
        if (*reg == Reg::HAND)
            return true;
        if (((unsigned char)(*reg) > 0x03) && ((unsigned char)(*reg) <= 0x07))
            return true;
        return false;
    }
    return false;
}

bool isStringRegisterArg(const RegisterArg& arg) {
    return (bool)(boost::get<std::string>(&arg));
}

bool isLongRegisterArg(const RegisterArg& arg) {
    return (bool)(boost::get<long>(&arg));
}

bool isAsmRegisterArg(const RegisterArg& arg) {
    return (bool)boost::get<InstrSeq>(&arg);
}
