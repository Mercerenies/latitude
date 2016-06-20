
#include "Instructions.hpp"
#include <utility>

using namespace std;

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
    props[Instr::UNTR] = { };
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
    return (bool)(boost::get<FunctionIndex>(&arg));
}

AssemblerError::AssemblerError()
    : AssemblerError("Assembler error") {}

AssemblerError::AssemblerError(string message)
    : message(message) {}

string AssemblerError::getMessage() {
    return message;
}

struct AppendVisitor {
    InstrSeq* instructions;

    void operator()(const Reg& reg) {
        instructions->push_back((unsigned char)reg);
    }

    void operator()(const std::string& str) {
        for (char ch : str)
            instructions->push_back(ch);
        instructions->push_back('\0');
    }

    // TODO I'm using a whole byte for the sign right now; minimize that as best as possible
    void operator()(const long& val) {
        long val1 = val;
        if (val1 < 0)
            instructions->push_back(0xFF);
        else
            instructions->push_back(0x00);
        val1 = abs(val1);
        for (int i = 0; i < 4; i++) {
            instructions->push_back((unsigned char)(val1 % 256));
            val1 /= 256;
        }
    }

    void operator()(const FunctionIndex& ind) {
        // No need for a sign bit; this is an index so it's always nonnegative
        int val1 = ind.index;
        for (int i = 0; i < 4; i++) {
            instructions->push_back((unsigned char)(val1 % 256));
            val1 /= 256;
        }
    }

};

void appendRegisterArg(const RegisterArg& arg, InstrSeq& seq) {
    AppendVisitor visitor { &seq };
    boost::apply_visitor(visitor, arg);
}

void appendInstruction(const Instr& instr, InstrSeq& seq) {
    seq.push_back((unsigned char)instr);
}

void AssemblerLine::setCommand(Instr str) {
    command = str;
}

void AssemblerLine::addRegisterArg(const RegisterArg& arg) {
    args.push_back(arg);
}

void AssemblerLine::validate() {
    InstructionSet iset = InstructionSet::getInstance();
    if (!iset.hasInstruction(command))
        throw AssemblerError("Unknown instruction " + to_string((int)command));
    InstructionSet::ValidList parms = iset.getParams(command);
    if (parms.size() != args.size())
        throw AssemblerError("Wrong number of arguments to " + to_string((int)command));
    auto parmIter = parms.begin();
    auto argIter = args.begin();
    while ((parmIter != parms.end()) && (argIter != args.end())) {
        if (!(*parmIter)(*argIter))
            throw AssemblerError("Unexpected argument type at " + to_string((int)command));
        ++parmIter;
        ++argIter;
    }
}

void AssemblerLine::appendOnto(InstrSeq& seq) const {
    appendInstruction(command, seq);
    for (auto& arg : args)
        appendRegisterArg(arg, seq);
}

TranslationUnit::TranslationUnit()
    : seq(), methods() {}

TranslationUnit::TranslationUnit(const InstrSeq& seq)
    : seq(seq), methods() {}

InstrSeq& TranslationUnit::instructions() {
    return seq;
}

InstrSeq& TranslationUnit::method(int index) {
    return methods.at(index);
}

int TranslationUnit::methodCount() {
    return methods.size();
}

FunctionIndex TranslationUnit::pushMethod(const InstrSeq& mthd) {
    int size = methods.size();
    methods.push_back(mthd);
    return { size };
}

FunctionIndex TranslationUnit::pushMethod(InstrSeq&& mthd) {
    int size = methods.size();
    methods.push_back(forward<InstrSeq>(mthd));
    return { size };
}

Method::Method()
    : unit(), ind({ 0 }) {}

Method::Method(const TranslationUnitPtr& ptr, FunctionIndex index)
    : unit(ptr), ind(index) {}

InstrSeq& Method::instructions() {
    // TODO What if unit is the null pointer?
    return unit->method(ind.index);
}

TranslationUnitPtr Method::translationUnit() {
    return unit;
}

FunctionIndex Method::index() {
    return ind;
}
