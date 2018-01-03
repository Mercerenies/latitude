
#include "Instructions.hpp"
#include <utility>
#include <memory>

using namespace std;

Instruction::Instruction(Instr i, const std::vector<RegisterArg> v)
    : instr(i), args(v) {}

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
    props[Instr::POP] = { isObjectRegister, isStackRegister };
    props[Instr::GETL] = { isObjectRegister };
    props[Instr::GETD] = { isObjectRegister };
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
    props[Instr::PEEK] = { isObjectRegister, isStackRegister };
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
    props[Instr::YLD] = { isLongRegisterArg, isObjectRegister };
    props[Instr::YLDC] = { isLongRegisterArg, isObjectRegister };
    props[Instr::DEL] = { };
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
    SerialInstrSeq* instructions;

    void operator()(const Reg& reg) {
        instructions->push_back((unsigned char)reg);
    }

    void operator()(const std::string& str) {
        for (char ch : str)
            instructions->push_back(ch);
        instructions->push_back('\0');
    }

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

void appendRegisterArg(const RegisterArg& arg, SerialInstrSeq& seq) {
    AppendVisitor visitor { &seq };
    boost::apply_visitor(visitor, arg);
}

void appendInstruction(const Instr& instr, SerialInstrSeq& seq) {
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
    seq.emplace_back(command, args);
}

// We may rename this function to overload with the above function in the future.
// Just using a different name now to avoid implicit casts messing us up.
void AssemblerLine::appendOntoSerial(SerialInstrSeq& seq) const {
    appendInstruction(command, seq);
    for (auto& arg : args)
        appendRegisterArg(arg, seq);
}

TranslationUnit::TranslationUnit()
    : methods() {
    methods.emplace_back();
}

TranslationUnit::TranslationUnit(const InstrSeq& seq)
    : methods() {
    methods.emplace_back(seq);
}

InstrSeq& TranslationUnit::instructions() {
    return methods[0];
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

// -1L will become the maximum unsigned long value
// TODO I can't believe I'm considering this corner case, but what if the number of instructions is exactly equal to std::numeric_limits<unsigned long>::max()
InstrSeek::InstrSeek() : pos(-1L), _size_set(false), _size(0L) {}

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
    if (!_size_set) {
        _size_set = true;
        _size = instructions().size();
    }
    return _size;
}

bool InstrSeek::atEnd() {
    return size() == position();
}

long InstrSeek::readLong(int n) {
    return boost::get<long>(instructions()[position()].args[n]);
}

string InstrSeek::readString(int n) {
    return boost::get<string>(instructions()[position()].args[n]);
}

Reg InstrSeek::readReg(int n) {
    return boost::get<Reg>(instructions()[position()].args[n]);
}

Instr InstrSeek::readInstr() {
    return instructions()[position()].instr;
}

FunctionIndex InstrSeek::readFunction(int n) {
    return boost::get<FunctionIndex>(instructions()[position()].args[n]);
}

CodeSeek::CodeSeek()
    : seq(make_shared<InstrSeq>()) {}

CodeSeek::CodeSeek(const InstrSeq& seq)
    : seq(make_shared<InstrSeq>(seq)) {}

CodeSeek::CodeSeek(InstrSeq&& seq)
    : seq(make_shared<InstrSeq>(forward<InstrSeq>(seq))) {}

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
    : internal(nullptr) {}

// Copy the unique_ptr and its contents when a SeekHolder is copied
SeekHolder::SeekHolder(const SeekHolder& other)
    : internal(other.internal ? unique_ptr<InstrSeek>(other.internal->copy()) : nullptr) // Band-aid solution
    , instr(other.internal ? &internal->instructions() : nullptr) {} // Band-aid solution

SeekHolder& SeekHolder::operator=(const SeekHolder& other) {
    internal = other.internal ? unique_ptr<InstrSeek>(other.internal->copy()) : nullptr; // Band-aid solution
    instr = other.internal ? &internal->instructions() : nullptr; // Band-aid solution
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

long SeekHolder::readLong(int n) {
    return boost::get<long>(instructions()[position()].args[n]);
}

string SeekHolder::readString(int n) {
    return boost::get<string>(instructions()[position()].args[n]);
}

Reg SeekHolder::readReg(int n) {
    return boost::get<Reg>(instructions()[position()].args[n]);
}

Instr SeekHolder::readInstr() {
    return instructions()[position()].instr;
}

FunctionIndex SeekHolder::readFunction(int n) {
    return boost::get<FunctionIndex>(instructions()[position()].args[n]);
}

bool SeekHolder::atEnd() {
    return internal->atEnd();
}
