#include "Bytecode.hpp"
#include "Reader.hpp"

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
    props[Instr::CALL] = { };
    props[Instr::XCALL] = { };
    props[Instr::XCALL0] = { };
    props[Instr::RET] = { };
    props[Instr::CLONE] = { };
    props[Instr::RTRV] = { };
    props[Instr::RTRVD] = { };
    props[Instr::STR] = { isStringRegisterArg };
    props[Instr::SSWAP] = { };
    props[Instr::EXPD] = { isRegister };
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

    // TODO Make sure the lexer is prepared to handle enforcing the 4-byte longs
    //      constraint even on a machine where sizeof(long) > 4
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

    void operator()(const std::vector<AssemblerLine>& line) {
        InstrSeq seq;
        for (auto& stmt : line)
            stmt.appendOnto(seq);
        for (auto& ch : seq)
            instructions->push_back((unsigned char)ch);
    }

};

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
    return (bool)boost::get<std::vector<AssemblerLine>>(&arg);
}

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

IntState intState() {
    IntState state;
    // ptr, slf, ret default to null
    // lex, dyn, arg, sto default to empty
    // cont default to empty
    // stack default to empty
    // err0, err1 default to false
    state.sym = Symbols::get()[""];
    // num0, num1 default to smallint(0)
    return state;
}

unsigned char popChar(InstrSeq& state) {
    if (state.empty())
        return 0;
    char val = state.back();
    state.pop_back();
    return val;
}

long popLong(InstrSeq& state) {
    int sign = 1;
    if (popChar(state) > 0)
        sign *= -1;
    int value = 0;
    for (int i = 0; i < 4; i++) {
        value *= 256;
        value += (long)popChar(state);
    }
    return sign * value;
}

string popString(InstrSeq& state) {
    string str;
    unsigned char ch;
    while ((ch = popChar(state)) != 0)
        str += ch;
    return str;
}

Reg popReg(InstrSeq& state) {
    unsigned char ch = popChar(state);
    return (Reg)ch;
}

Instr popInstr(InstrSeq& state) {
    unsigned char ch = popChar(state);
    return (Instr)ch;
}

void executeInstr(Instr instr, IntState& state) {
    switch (instr) {
    case Instr::MOV: {
        Reg src = popReg(state.cont);
        Reg dest = popReg(state.cont);
        ObjectPtr mid;
        switch (src) {
        case Reg::PTR:
            mid = state.ptr;
            break;
        case Reg::SLF:
            mid = state.slf;
            break;
        case Reg::RET:
            mid = state.ret;
            break;
        default:
            mid = ObjectPtr(); // TODO Error handling?
            state.err0 = true;
            break;
        }
        switch (dest) {
        case Reg::PTR:
            state.ptr = mid;
            break;
        case Reg::SLF:
            state.slf = mid;
            break;
        case Reg::RET:
            state.ret = mid;
            break;
        default:
            // TODO Error handling?
            state.err0 = true;
            break;
        }
    }
        break;
    case Instr::PUSH: {
        Reg src = popReg(state.cont);
        Reg stack = popReg(state.cont);
        ObjectPtr mid;
        switch (src) {
        case Reg::PTR:
            mid = state.ptr;
            break;
        case Reg::SLF:
            mid = state.slf;
            break;
        case Reg::RET:
            mid = state.ret;
            break;
        default:
            mid = ObjectPtr(); // TODO Error handling?
            state.err0 = true;
            break;
        }
        switch (stack) {
        case Reg::LEX:
            state.lex.push(mid);
            break;
        case Reg::DYN:
            state.dyn.push(mid);
            break;
        case Reg::ARG:
            state.dyn.push(mid);
            break;
        case Reg::STO:
            state.dyn.push(mid);
            break;
        default:
            // TODO Error handling?
            state.err0 = true;
        }
    }
        break;
    case Instr::POP: {
        stack<ObjectPtr>* stack;
        switch (popReg(state.cont)) {
        case Reg::LEX:
            stack = &state.lex;
            break;
        case Reg::DYN:
            stack = &state.dyn;
            break;
        case Reg::ARG:
            stack = &state.dyn;
            break;
        case Reg::STO:
            stack = &state.dyn;
            break;
        default:
            // TODO Error handling?
            stack = nullptr;
            state.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                state.ret = stack->top();
                stack->pop();
            } else {
                state.err0 = true;
            }
        }
    }
        break;
    case Instr::GETL: {
        if (state.lex.empty())
            state.err0 = true;
        else
            state.ptr = state.lex.top();
    }
        break;
    case Instr::GETD: {
        if (state.dyn.empty())
            state.err0 = true;
        else
            state.ptr = state.dyn.top();
    }
        break;
    case Instr::ESWAP: {
        swap(state.err0, state.err1);
    }
        break;
    case Instr::ECLR: {
        state.err0 = false;
    }
        break;
    case Instr::ESET: {
        state.err0 = true;
    }
        break;
    case Instr::SYM: {
        string str = popString(state.cont);
        state.sym = Symbols::get()[str];
    }
        break;
    case Instr::NUM: {
        string str = popString(state.cont);
        state.num0 = Number(static_cast<Number::bigint>(str));
    }
        break;
    case Instr::INT: {
        long val = popLong(state.cont);
        state.num0 = Number(val);
    }
        break;
    case Instr::FLOAT: {
        string str = popString(state.cont);
        double dd = strtod(str.c_str(), NULL);
        state.num0 = Number(dd);
    }
        break;
    case Instr::NSWAP: {
        swap(state.num0, state.num1);
    }
        break;
    case Instr::CALL: {
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&state.ptr.lock()->prim());
        Slot closure = (*state.ptr.lock())[ Symbols::get()["closure"] ];
        if ((closure.getType() == SlotType::PTR) && (stmt)) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!state.dyn.empty())
                state.dyn.push( clone(state.dyn.top()) );
            else
                state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            state.lex.push( clone(closure.getPtr()) );
            // (4) Bind all of the arguments
            if (!state.dyn.empty()) {
                int index = 1;
                while (!state.arg.empty()) {
                    ObjectPtr arg = state.arg.top();
                    state.arg.pop();
                    state.dyn.top().lock()->put(Symbols::get()[ "$" + to_string(index) ], arg);
                    index++;
                }
            }
            // (5) Push %cont onto %stack
            state.stack.push(state.cont);
            // (6) Make a new %cont
            // TODO We want to make methods pre-compile, not compile here every time
            // This current (very inefficient) approach is for backward compatibility
            state.cont = InstrSeq();
            for (auto stmt0 : *stmt) {
                auto ref = stmt0->translate();
                state.cont.insert(state.cont.end(), ref.begin(), ref.end());
            }
        } else {
            // It's not a method; just return it
            state.ret = state.ptr;
        }
    }
        break;
    case Instr::XCALL: {
        // Reserved
    }
        break;
    case Instr::XCALL0: {
        ////
    }
        break;
    case Instr::RET: {
        if (state.lex.empty())
            state.err0 = true;
        else
            state.lex.pop();
        if (state.dyn.empty())
            state.err0 = true;
        else
            state.dyn.pop();
    }
        break;
    case Instr::CLONE: {
        state.ret = clone(state.slf);
    }
        break;
    case Instr::RTRV: {
        ////
    }
        break;
    case Instr::RTRVD: {
        Slot slot = (*state.slf.lock())[state.sym];
        if (slot.getType() == SlotType::PTR)
            state.ret = slot.getPtr();
        else
            state.err0 = true;
    }
        break;
    case Instr::STR: {
        string str = popString(state.cont);
        state.str0 = str;
    }
        break;
    case Instr::SSWAP: {
        swap(state.str0, state.str1);
    }
        break;
    case Instr::EXPD: {
        Reg expd = popReg(state.cont);
        switch (expd) {
        case Reg::SYM: {
            auto test = boost::get<Symbolic>(&state.ptr.lock()->prim());
            if (test)
                state.sym = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::NUM0: {
            auto test = boost::get<Number>(&state.ptr.lock()->prim());
            if (test)
                state.num0 = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::NUM1: {
            auto test = boost::get<Number>(&state.ptr.lock()->prim());
            if (test)
                state.num1 = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::STR0: {
            auto test = boost::get<string>(&state.ptr.lock()->prim());
            if (test)
                state.str0 = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::STR1: {
            auto test = boost::get<string>(&state.ptr.lock()->prim());
            if (test)
                state.str1 = *test;
            else
                state.err0 = true;
        }
            break;
        default:
            state.err0 = true; // TODO Error handling?
            break;
        }
        break;
    }
        break;
    }
}

void doOneStep(IntState& state) {
    if (state.cont.empty()) {
        // Pop off the stack
        if (!state.stack.empty()) {
            state.cont = state.stack.top();
            state.stack.pop();
            doOneStep(state);
        }
    } else {
        // Run one command
        Instr instr = popInstr(state.cont);
        executeInstr(instr, state);
    }
}

///// Some more instructions for moving around ASM instruction sets (need a new register for this, etc.)
