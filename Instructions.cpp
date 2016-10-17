
#include "Instructions.hpp"
#include <utility>
#include <memory>

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
    return methods[index];
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

size_t Method::size() {
    return instructions().size();
}

TranslationUnitPtr Method::translationUnit() {
    return unit;
}

FunctionIndex Method::index() {
    return ind;
}

InstrSeek::InstrSeek() : pos(0L) {}

unsigned long InstrSeek::position() {
    return pos;
}

void InstrSeek::advancePosition(unsigned long val) {
    pos += val;
    if (pos < 0)
        pos = 0;
    if (pos > size())
        pos = size();
}

unsigned long InstrSeek::size() {
    return instructions().size();
}

bool InstrSeek::atEnd() {
    return size() == position();
}

unsigned char InstrSeek::popChar() {
    if (atEnd())
        return 0;
    unsigned char val = instructions()[position()];
    advancePosition(1);
    return val;
}

long InstrSeek::popLong() {
    int sign = 1;
    if (popChar() > 0)
        sign *= -1;
    long value = 0;
    long pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)popChar();
        pow <<= 8;
    }
    return sign * value;
}

string InstrSeek::popString() {
    string str;
    unsigned char ch;
    while ((ch = popChar()) != 0)
        str += ch;
    return str;
}

Reg InstrSeek::popReg() {
    unsigned char ch = popChar();
    return (Reg)ch;
}

Instr InstrSeek::popInstr() {
    unsigned char ch = popChar();
    return (Instr)ch;
}

FunctionIndex InstrSeek::popFunction() {
    int value = 0;
    int pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)popChar();
        pow <<= 8;
    }
    return { value };
}

CodeSeek::CodeSeek()
    : seq(make_shared<InstrSeq>()) {}

CodeSeek::CodeSeek(const InstrSeq& seq)
    : seq(make_shared<InstrSeq>(seq)) {}

CodeSeek::CodeSeek(InstrSeq&& seq)
    : seq(make_shared<InstrSeq>(forward<InstrSeq>(seq))) {}

#include <iostream>
unique_ptr<InstrSeek> CodeSeek::copy() {
    return unique_ptr<InstrSeek>(new CodeSeek(*this));
}

InstrSeq& CodeSeek::instructions() {
    return *seq;
}

MethodSeek::MethodSeek(Method m)
    : method(m) {}

unique_ptr<InstrSeek> MethodSeek::copy() {
    auto other = unique_ptr<InstrSeek>(new MethodSeek(*this));
    return other;
}

InstrSeq& MethodSeek::instructions() {
    return method.instructions();
}

SeekHolder::SeekHolder()
    : internal(unique_ptr<InstrSeek>(new CodeSeek())) {}

// Copy the unique_ptr and its contents when a SeekHolder is copied
SeekHolder::SeekHolder(const SeekHolder& other)
    : internal(unique_ptr<InstrSeek>(other.internal->copy()))
    , instr(&internal->instructions()) {}

SeekHolder& SeekHolder::operator=(const SeekHolder& other) {
    internal = unique_ptr<InstrSeek>(other.internal->copy());
    instr = &internal->instructions();
    return *this;
}

InstrSeq& SeekHolder::instructions() {
    return *instr;
}

unsigned long SeekHolder::position() {
    return internal->position();
}

void SeekHolder::advancePosition(unsigned long arg) {
    internal->advancePosition(arg);
}

unsigned char SeekHolder::popChar() {
    return internal->popChar();
}

long SeekHolder::popLong() {
    return internal->popLong();
}

std::string SeekHolder::popString() {
    return internal->popString();
}

Reg SeekHolder::popReg() {
    return internal->popReg();
}

Instr SeekHolder::popInstr() {
    return internal->popInstr();
}

FunctionIndex SeekHolder::popFunction() {
    return internal->popFunction();
}

bool SeekHolder::atEnd() {
    return internal->atEnd();
}
