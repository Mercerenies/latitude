
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
    props[Instr::ARR] = { isLongRegisterArg };
    props[Instr::DICT] = { isLongRegisterArg };
    props[Instr::XXX] = { isLongRegisterArg };
    props[Instr::GOTO] = { };
    props[Instr::MSWAP] = { };
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

const char* AssemblerError::what() {
    return message.c_str();
}

struct AppendVisitor {
    SerialInstrSeq* instructions;

    void operator()(const Reg& reg) {
        instructions->push_back((unsigned char)reg);
    }

    void operator()(const std::string& str) {
        for (char ch : str) {
            if (ch == 0) {
                instructions->push_back('\0');
                instructions->push_back('.');
            } else {
                instructions->push_back(ch);
            }
        }
        instructions->push_back('\0');
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

AssemblerLineArgs::AssemblerLineArgs(const AssemblerLine& inner)
    : impl(inner) {}

AssemblerLine::const_iterator AssemblerLineArgs::begin() const {
    return impl.iterBegin();
}

AssemblerLine::const_iterator AssemblerLineArgs::end() const {
    return impl.iterEnd();
}

AssemblerLine::AssemblerLine(Instr code)
    : command(code), args() {}

Instr AssemblerLine::getCommand() const noexcept {
    return command;
}

void AssemblerLine::setCommand(Instr str) {
    command = str;
}

void AssemblerLine::addRegisterArg(const RegisterArg& arg) {
    args.push_back(arg);
}

void AssemblerLine::clearRegisterArgs() {
    args.clear();
}

AssemblerLineArgs AssemblerLine::arguments() const {
    return AssemblerLineArgs(*this);
}

RegisterArg AssemblerLine::argument(int n) const {
    return args[n];
}

RegisterArg AssemblerLine::argumentCount() const {
    return args.size();
}

auto AssemblerLine::iterBegin() const -> const_iterator {
    return args.begin();
}

auto AssemblerLine::iterEnd() const -> const_iterator {
    return args.end();
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
    seq.emplace_back(*this);
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

MethodSeek::MethodSeek()
    : pos(-1L), _size(0L), method(Method(nullptr, { 0 })) {}

MethodSeek::MethodSeek(Method m)
    : pos(-1L), _size(m.instructions().size()), method(m) {}

unsigned long MethodSeek::position() {
    return pos;
}

void MethodSeek::advancePosition(unsigned long val) {
    pos += val;
    if (pos < 0)
        pos = 0;
    if (pos > size())
        pos = size();
}

unsigned long MethodSeek::size() {
    return _size;
}

bool MethodSeek::atEnd() {
    return size() == position();
}

long MethodSeek::readLong(int n) {
    return boost::get<long>(instructions()[position()].argument(n));
}

string MethodSeek::readString(int n) {
    return boost::get<string>(instructions()[position()].argument(n));
}

Reg MethodSeek::readReg(int n) {
    return boost::get<Reg>(instructions()[position()].argument(n));
}

Instr MethodSeek::readInstr() {
    return instructions()[position()].getCommand();
}

FunctionIndex MethodSeek::readFunction(int n) {
    return boost::get<FunctionIndex>(instructions()[position()].argument(n));
}

InstrSeq& MethodSeek::instructions() {
    return method.instructions();
}
