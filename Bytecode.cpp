#include "Bytecode.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"

#define DEBUG_INSTR 0

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

    void operator()(const InstrSeq& seq) {
        unsigned long length = seq.size();
        unsigned long length2 = 0;
        unsigned long temp = length;
        while (temp > 0) {
            temp /= 256;
            length2++;
        }
        instructions->push_back((unsigned char)length2);
        for (unsigned long i = 0; i < length2; i++) {
            instructions->push_back((unsigned char)(length % 256));
            length /= 256;
        }
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
    // str0, str1 default to empty string
    state.mthd = asmCode(makeAssemblerLine(Instr::RET));
    // cpp default to empty map
    // strm default to null
    // prcs default to null
    state.mthdz = asmCode(makeAssemblerLine(Instr::RET));
    // flag default to false
    // wind default to empty
    return state;
}

StatePtr statePtr(const IntState& state) {
    return make_shared<IntState, const IntState&>(state);
}

void resolveThunks(IntState& state, stack<WindPtr> oldWind, stack<WindPtr> newWind) {
    deque<WindPtr> exits;
    deque<WindPtr> enters;
    while (!oldWind.empty()) {
        exits.push_back(oldWind.top());
        oldWind.pop();
    }
    while (!newWind.empty()) {
        enters.push_front(newWind.top());
        newWind.pop();
    }
    // Determine what the two scopes have in common and remove them
    // (There's no reason to run an 'after' and then a corresponding 'before' from the same thunk)
    while ((!exits.empty()) && (!enters.empty()) && (exits.back() == enters.front())) {
        exits.pop_back();
        enters.pop_front();
    }
    state.stack.push(state.cont);
    state.cont.clear();
    for (WindPtr ptr : exits) {
        state.lex.push( clone(ptr->after.lex) );
        state.dyn.push( clone(ptr->after.dyn) );
        state.stack.push(ptr->after.code);
    }
    for (WindPtr ptr : enters) {
        state.lex.push( clone(ptr->before.lex) );
        state.dyn.push( clone(ptr->before.dyn) );
        state.stack.push(ptr->before.code);
    }
}

unsigned char popChar(InstrSeq& state) {
    if (state.empty())
        return 0;
    char val = state.front();
    state.pop_front();
    return val;
}

long popLong(InstrSeq& state) {
    int sign = 1;
    if (popChar(state) > 0)
        sign *= -1;
    long value = 0;
    long pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)popChar(state);
        pow <<= 8;
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

InstrSeq popLine(InstrSeq& state) {
    InstrSeq result;
    unsigned long length2 = (unsigned long)popChar(state);
    unsigned long length = 0;
    unsigned long pow = 1;
    for (unsigned long i = 0; i < length2; i++) {
        length += pow * (unsigned long)popChar(state);
        pow <<= 8;
    }
    for (unsigned long i = 0; i < length; i++)
        result.push_back(popChar(state));
    return result;
}

void executeInstr(Instr instr, IntState& state) {
    switch (instr) {
    case Instr::MOV: {
        Reg src = popReg(state.cont);
        Reg dest = popReg(state.cont);
#if DEBUG_INSTR > 0
        cout << "MOV " << (long)src << " " << (long)dest << endl;
#endif
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
#if DEBUG_INSTR > 0
        cout << "PUSH " << (long)src << " " << (long)stack << endl;
#endif
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
            state.arg.push(mid);
            break;
        case Reg::STO:
            state.sto.push(mid);
            break;
        case Reg::HAND:
            state.hand.push(mid);
            break;
        default:
            // TODO Error handling?
            state.err0 = true;
        }
    }
        break;
    case Instr::POP: {
        stack<ObjectPtr>* stack;
        Reg reg = popReg(state.cont);
#if DEBUG_INSTR > 0
        cout << "POP " << (long)reg << endl;
#endif
        switch (reg) {
        case Reg::LEX:
            stack = &state.lex;
            break;
        case Reg::DYN:
            stack = &state.dyn;
            break;
        case Reg::ARG:
            stack = &state.arg;
            break;
        case Reg::STO:
            stack = &state.sto;
            break;
        case Reg::HAND:
            stack = &state.hand;
            break;
        default:
            // TODO Error handling?
            stack = nullptr;
            state.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                state.ptr = stack->top();
                stack->pop();
            } else {
                state.err0 = true;
            }
        }
    }
        break;
    case Instr::GETL: {
#if DEBUG_INSTR > 0
        cout << "GETL" << endl;
        cout << "* " << state.lex.top().lock() << endl;
#endif
        if (state.lex.empty())
            state.err0 = true;
        else
            state.ptr = state.lex.top();
    }
        break;
    case Instr::GETD: {
#if DEBUG_INSTR > 0
        cout << "GETD" << endl;
#if DEBUG_INSTR > 1
        cout << "* " << state.dyn.top().lock() << endl;
#endif
#endif
        if (state.dyn.empty())
            state.err0 = true;
        else
            state.ptr = state.dyn.top();
    }
        break;
    case Instr::ESWAP: {
#if DEBUG_INSTR > 0
        cout << "ESWAP" << endl;
#endif
        swap(state.err0, state.err1);
    }
        break;
    case Instr::ECLR: {
#if DEBUG_INSTR > 0
        cout << "ECLR" << endl;
#endif
        state.err0 = false;
    }
        break;
    case Instr::ESET: {
#if DEBUG_INSTR > 0
        cout << "ESET" << endl;
#endif
        state.err0 = true;
    }
        break;
    case Instr::SYM: {
        string str = popString(state.cont);
#if DEBUG_INSTR > 0
        cout << "SYM \"" << str << "\"" << endl;
#endif
        state.sym = Symbols::get()[str];
    }
        break;
    case Instr::NUM: {
        string str = popString(state.cont);
#if DEBUG_INSTR > 0
        cout << "NUM \"" << str << "\"" << endl;
#endif
        state.num0 = Number(static_cast<Number::bigint>(str));
    }
        break;
    case Instr::INT: {
        long val = popLong(state.cont);
#if DEBUG_INSTR > 0
        cout << "INT " << val << endl;
#endif
        state.num0 = Number(val);
    }
        break;
    case Instr::FLOAT: {
        string str = popString(state.cont);
#if DEBUG_INSTR > 0
        cout << "FLOAT \"" << str << "\"" << endl;
#endif
        double dd = strtod(str.c_str(), NULL);
        state.num0 = Number(dd);
    }
        break;
    case Instr::NSWAP: {
#if DEBUG_INSTR > 0
        cout << "NSWAP" << endl;
#endif
        swap(state.num0, state.num1);
    }
        break;
    case Instr::CALL: {
        long args = popLong(state.cont);
#if DEBUG_INSTR > 0
        cout << "CALL " << args << " (" << Symbols::get()[state.sym] << ")" << endl;
#if DEBUG_INSTR > 1
        cout << "* Method Properties " << state.ptr.lock() << endl;
#endif
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&state.ptr.lock()->prim());
        auto stmtNew = boost::get<NewMethod>(&state.ptr.lock()->prim());
        Slot closure = (*state.ptr.lock())[ Symbols::get()["closure"] ];
#if DEBUG_INSTR > 2
        cout << "* Method Properties " <<
            (closure.getType() == SlotType::PTR) << " " <<
            stmt << " " << stmtNew << endl;
#endif
        if ((closure.getType() == SlotType::PTR) && (stmt || stmtNew)) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!state.dyn.empty())
                state.dyn.push( clone(state.dyn.top()) );
            else
                state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            state.lex.push( clone(closure.getPtr()) );
            // (4) Bind all the local variables
            state.lex.top().lock()->put(Symbols::get()["self"], state.slf);
            state.lex.top().lock()->put(Symbols::get()["again"], state.ptr);
            state.lex.top().lock()->put(Symbols::get()["lexical"], state.lex.top());
            if (!state.dyn.empty()) {
                state.lex.top().lock()->put(Symbols::get()["dynamic"], state.dyn.top());
                state.dyn.top().lock()->put(Symbols::get()["$lexical"], state.lex.top());
                state.dyn.top().lock()->put(Symbols::get()["$dynamic"], state.dyn.top());
            }
            // (5) Push the trace information
            state.trace.push( make_tuple(state.line, state.file) );
            // (6) Bind all of the arguments
            if (!state.dyn.empty()) {
                int index = args;
                for (long n = 0; n < args; n++) {
                    ObjectPtr arg = state.arg.top();
                    state.arg.pop();
                    state.dyn.top().lock()->put(Symbols::get()[ "$" + to_string(index) ], arg);
                    index--;
                }
            }
            // (7) Push %cont onto %stack
            state.stack.push(state.cont);
            // (8) Make a new %cont
            if (stmtNew) {
#if DEBUG_INSTR > 3
                cout << "* (cont) " << stmtNew->size() << endl;
                if (stmtNew->size() == 0) {
                    // What are we looking at...?
                    cout << "* * Where ptr has" << endl;
                    for (auto& x : keys(state.ptr))
                        cout << "  " << Symbols::get()[x];
                    cout << endl;
                    cout << "* * Directly" << endl;
                    for (auto& x : (state.ptr.lock())->directKeys())
                        cout << "  " << Symbols::get()[x];
                    cout << endl;
                    cout << "* * Parents of ptr" << endl;
                    for (auto& x : hierarchy(state.ptr))
                        cout << "  " << x.lock();
                    cout << endl;
                    cout << "* * Following the prims of ptr" << endl;
                    for (auto& x : hierarchy(state.ptr))
                        cout << "  " << x.lock()->prim().which();
                    cout << endl;
                }
#endif
                state.cont = *stmtNew;
            } else {
                state.cont = InstrSeq();
                for (auto stmt0 : *stmt) {
                    auto ref = stmt0->translate();
                    state.cont.insert(state.cont.end(), ref.begin(), ref.end());
                }
                // TODO Add a `ret` instruction here
            }
        } else {
            // It's not a method; just return it
            for (long n = 0; n < args; n++) {
                state.arg.pop(); // For consistency, we must pop and discard these anyway
            }
            state.ret = state.ptr;
        }
    }
        break;
    case Instr::XCALL: {
#if DEBUG_INSTR > 0
        cout << "XCALL (" << Symbols::get()[state.sym] << ")" << endl;
#endif
        auto stmt = boost::get<Method>(&state.ptr.lock()->prim());
        auto stmtNew = boost::get<NewMethod>(&state.ptr.lock()->prim());
        if (stmt || stmtNew) {
            // (6) Push %cont onto %stack
            state.stack.push(state.cont);
            // (7) Make a new %cont
            if (stmtNew) {
                state.cont = *stmtNew;
            } else {
                state.cont = InstrSeq();
                for (auto stmt0 : *stmt) {
                    auto ref = stmt0->translate();
                    state.cont.insert(state.cont.end(), ref.begin(), ref.end());
                }
            }
        }
    }
        break;
    case Instr::XCALL0: {
        long args = popLong(state.cont);
#if DEBUG_INSTR > 0
        cout << "XCALL0 " << args << " (" << Symbols::get()[state.sym] << ")" << endl;
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&state.ptr.lock()->prim());
        auto stmtNew = boost::get<NewMethod>(&state.ptr.lock()->prim());
        Slot closure = (*state.ptr.lock())[ Symbols::get()["closure"] ];
        if ((closure.getType() == SlotType::PTR) && (stmt || stmtNew)) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!state.dyn.empty())
                state.dyn.push( clone(state.dyn.top()) );
            else
                state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            state.lex.push( clone(closure.getPtr()) );
            // (4) Bind all the local variables
            state.lex.top().lock()->put(Symbols::get()["self"], state.slf);
            state.lex.top().lock()->put(Symbols::get()["again"], state.ptr);
            state.lex.top().lock()->put(Symbols::get()["lexical"], state.lex.top());
            if (!state.dyn.empty()) {
                state.lex.top().lock()->put(Symbols::get()["dynamic"], state.dyn.top());
                state.dyn.top().lock()->put(Symbols::get()["$lexical"], state.lex.top());
                state.dyn.top().lock()->put(Symbols::get()["$dynamic"], state.dyn.top());
            }
            // (5) Push the trace information
            state.trace.push( make_tuple(state.line, state.file) );
            // (6) Bind all of the arguments
            if (!state.dyn.empty()) {
                int index = args;
                for (long n = 0; n < args; n++) {
                    ObjectPtr arg = state.arg.top();
                    state.arg.pop();
                    state.dyn.top().lock()->put(Symbols::get()[ "$" + to_string(index) ], arg);
                    index--;
                }
            }
        } else {
            // It's not a method; just return it
            for (long n = 0; n < args; n++) {
                state.arg.pop(); // For consistency, we must pop and discard these anyway
            }
            state.ret = state.ptr;
        }
    }
        break;
    case Instr::RET: {
#if DEBUG_INSTR > 0
        cout << "RET" << endl;
#endif
        if (state.lex.empty())
            state.err0 = true;
        else
            state.lex.pop();
        if (state.dyn.empty())
            state.err0 = true;
        else
            state.dyn.pop();
        if (state.trace.empty())
            state.err0 = true;
        else
            state.trace.pop();
    }
        break;
    case Instr::CLONE: {
#if DEBUG_INSTR > 0
        cout << "CLONE" << endl;
#endif
        state.ret = clone(state.slf);
    }
        break;
    case Instr::RTRV: {
#if DEBUG_INSTR > 0
        cout << "RTRV (" << Symbols::get()[state.sym] << ")" << endl;
#endif
        list<ObjectSPtr> parents;
        ObjectSPtr curr = state.slf.lock();
        Symbolic name = state.sym;
        ObjectPtr value;
        // Try to find the value itself
        while (find(parents.begin(), parents.end(), curr) == parents.end()) {
            parents.push_back(curr);
            Slot slot = (*curr)[name];
            if (slot.getType() == SlotType::PTR) {
                value = slot.getPtr();
                break;
            }
            curr = (*curr)[ Symbols::get()["parent"] ].getPtr().lock();
        }
        if (value.expired()) {
#if DEBUG_INSTR > 2
            cout << "* Looking for missing" << endl;
#if DEBUG_INSTR > 3
            cout << "* Information:" << endl;
            cout << "* * Lex: " << state.lex.top().lock() << endl;
            cout << "* * Dyn: " << state.dyn.top().lock() << endl;
            cout << "* * Slf: " << state.slf.lock() << endl;
#endif
#endif
            // Now try for missing
            name = Symbols::get()["missing"];
            parents.clear();
            curr = state.slf.lock();
            while (find(parents.begin(), parents.end(), curr) == parents.end()) {
                parents.push_back(curr);
                Slot slot = (*curr)[name];
                if (slot.getType() == SlotType::PTR) {
                    value = slot.getPtr();
                    break;
                }
                curr = (*curr)[ Symbols::get()["parent"] ].getPtr().lock();
            }
            state.ret = value;
            // TODO What if we don't find a `missing`?
#if DEBUG_INSTR > 1
            if (value.expired())
                cout << "* Found no missing" << endl;
            else
                cout << "* Found missing" << endl;
#endif
            InstrSeq seq0;
            // Find the literal object to use for the argument
            (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq0);
            (makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO)).appendOnto(seq0);
            (makeAssemblerLine(Instr::GETL)).appendOnto(seq0);
            (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(seq0);
            (makeAssemblerLine(Instr::SYM, "meta")).appendOnto(seq0);
            (makeAssemblerLine(Instr::RTRV)).appendOnto(seq0);
            (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq0);
            (makeAssemblerLine(Instr::SYM, "Symbol")).appendOnto(seq0);
            (makeAssemblerLine(Instr::RTRV)).appendOnto(seq0);
            (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq0);
            // Clone and put a prim() onto it
            (makeAssemblerLine(Instr::CLONE)).appendOnto(seq0);
            (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq0);
            (makeAssemblerLine(Instr::SYMN, name.index)).appendOnto(seq0);
            (makeAssemblerLine(Instr::LOAD, Reg::SYM)).appendOnto(seq0);
            (makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG)).appendOnto(seq0);
            (makeAssemblerLine(Instr::POP, Reg::STO)).appendOnto(seq0);
            // Call it
            (makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF)).appendOnto(seq0);
            (makeAssemblerLine(Instr::POP, Reg::STO)).appendOnto(seq0);
            (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq0);
            state.stack.push(state.cont);
            state.cont = seq0;
        } else {
#if DEBUG_INSTR > 1
            cout << "* Found " << value.lock() << endl;
#endif
            state.ret = value;
        }
    }
        break;
    case Instr::RTRVD: {
#if DEBUG_INSTR > 0
        cout << "RTRVD (" << Symbols::get()[state.sym] << ")" << endl;
#endif
        Slot slot = (*state.slf.lock())[state.sym];
        if (slot.getType() == SlotType::PTR)
            state.ret = slot.getPtr();
        else
            state.err0 = true;
    }
        break;
    case Instr::STR: {
        string str = popString(state.cont);
#if DEBUG_INSTR > 0
        cout << "STR \"" << str << "\"" << endl;
#endif
        state.str0 = str;
    }
        break;
    case Instr::SSWAP: {
#if DEBUG_INSTR > 0
        cout << "SSWAP" << endl;
#endif
        swap(state.str0, state.str1);
    }
        break;
    case Instr::EXPD: {
        Reg expd = popReg(state.cont);
#if DEBUG_INSTR > 0
        cout << "EXPD " << (long)expd << endl;
#endif
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
        case Reg::MTHD: {
            auto test = boost::get<NewMethod>(&state.ptr.lock()->prim());
            if (test)
                state.mthd = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::STRM: {
            auto test = boost::get<StreamPtr>(&state.ptr.lock()->prim());
            if (test)
                state.strm = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::PRCS: {
            auto test = boost::get<ProcessPtr>(&state.ptr.lock()->prim());
            if (test)
                state.prcs = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::MTHDZ: {
            auto test = boost::get<NewMethod>(&state.ptr.lock()->prim());
            if (test)
                state.mthdz = *test;
            else
                state.err0 = true;
        }
            break;
        default:
            state.err0 = true; // TODO Error handling?
            break;
        }
    }
        break;
    case Instr::MTHD: {
#if DEBUG_INSTR > 0
        cout << "MTHD ..." << endl;
#endif
        InstrSeq seq = popLine(state.cont);
        state.mthd = std::move(seq);
    }
        break;
    case Instr::LOAD: {
        Reg ld = popReg(state.cont);
#if DEBUG_INSTR > 0
        cout << "LOAD " << (long)ld << endl;
#endif
        switch (ld) {
        case Reg::SYM: {
            state.ptr.lock()->prim(state.sym);
        }
            break;
        case Reg::NUM0: {
            state.ptr.lock()->prim(state.num0);
        }
            break;
        case Reg::NUM1: {
            state.ptr.lock()->prim(state.num1);
        }
            break;
        case Reg::STR0: {
            state.ptr.lock()->prim(state.str0);
        }
            break;
        case Reg::STR1: {
            state.ptr.lock()->prim(state.str1);
        }
            break;
        case Reg::MTHD: {
#if DEBUG_INSTR > 1
            cout << "* Method Length " << state.mthd.size() << endl;
#endif
            state.ptr.lock()->prim(state.mthd);
        }
            break;
        case Reg::STRM: {
            state.ptr.lock()->prim(state.strm);
        }
            break;
        case Reg::PRCS: {
            state.ptr.lock()->prim(state.prcs);
        }
            break;
        case Reg::MTHDZ: {
#if DEBUG_INSTR > 1
            cout << "* Method Length " << state.mthdz.size() << endl;
#endif
            state.ptr.lock()->prim(state.mthdz);
        }
            break;
        default:
            state.err0 = true; // TODO Error handling?
            break;
        }
    }
        break;
    case Instr::SETF: {
#if DEBUG_INSTR > 0
        cout << "SETF (" << Symbols::get()[state.sym] << ")" << endl;
#if DEBUG_INSTR > 2
        cout << "* Information:" << endl;
        cout << "* * Lex: " << state.lex.top().lock() << endl;
        cout << "* * Dyn: " << state.dyn.top().lock() << endl;
        cout << "* * Slf: " << state.slf.lock() << endl;
#endif
#endif
        if (state.slf.expired())
            state.err0 = true;
        else
            state.slf.lock()->put(state.sym, state.ptr);
    }
        break;
    case Instr::PEEK: {
        stack<ObjectPtr>* stack;
        Reg reg = popReg(state.cont);
#if DEBUG_INSTR > 0
        cout << "PEEK " << (long)reg << endl;
#endif
        switch (reg) {
        case Reg::LEX:
            stack = &state.lex;
            break;
        case Reg::DYN:
            stack = &state.dyn;
            break;
        case Reg::ARG:
            stack = &state.arg;
            break;
        case Reg::STO:
            stack = &state.sto;
            break;
        case Reg::HAND:
            stack = &state.hand;
            break;
        default:
            // TODO Error handling?
            stack = nullptr;
            state.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                state.ptr = stack->top();
            } else {
                state.err0 = true;
            }
        }

    }
        break;
    case Instr::SYMN: {
        long val = popLong(state.cont);
#if DEBUG_INSTR > 0
        cout << "SYMN " << val << endl;
#endif
        state.sym = { val };
    }
        break;
    case Instr::CPP: {
        long val = popLong(state.cont);
#if DEBUG_INSTR > 0
        cout << "CPP " << val << endl;
#endif
        auto func = state.cpp[val];
        if (func)
            func(state);
        else
            state.err0 = true;
    }
        break;
    case Instr::BOL: {
#if DEBUG_INSTR > 0
        cout << "BOL (" << state.flag << ")" << endl;
#endif
        garnishBegin(state, state.flag);
    }
        break;
    case Instr::TEST: {
#if DEBUG_INSTR > 0
        cout << "TEST" << endl;
#endif
        if (state.slf.expired() || state.ptr.expired())
            state.flag = false;
        else
            state.flag = (state.slf.lock() == state.ptr.lock());
    }
        break;
    case Instr::BRANCH: {
#if DEBUG_INSTR > 0
        cout << "BRANCH (" << state.flag << ")" << endl;
#endif
        state.stack.push(state.cont);
        if (state.flag) {
#if DEBUG_INSTR > 2
            cout << "* Method ( ) Length " << state.mthd.size() << endl;
#endif
            state.cont = state.mthd;
        } else {
#if DEBUG_INSTR > 2
            cout << "* Method (Z) Length " << state.mthdz.size() << endl;
#endif
            state.cont = state.mthdz;
        }
    }
        break;
    case Instr::CCALL: {
#if DEBUG_INSTR > 0
        cout << "CCALL" << endl;
#endif
        // TODO What if %slf is null?
        state.slf.lock()->prim( statePtr(state) );
        state.arg.push(state.slf);
        InstrSeq seq = asmCode( makeAssemblerLine(Instr::CALL, 1L) );
        state.cont.insert(state.cont.begin(), seq.begin(), seq.end());
    }
        break;
    case Instr::CGOTO: {
#if DEBUG_INSTR > 0
        cout << "CGOTO" << endl;
#endif
        auto cont = boost::get<StatePtr>( state.ptr.lock()->prim() );
        if (cont) {
            auto oldWind = state.wind;
            auto newWind = cont->wind;
            state = *cont;
            resolveThunks(state, oldWind, newWind);
        } else {
#if DEBUG_INSTR > 0
            cout << "* Not a continuation" << endl;
#endif
        }
    }
        break;
    case Instr::CRET: {
#if DEBUG_INSTR > 0
        cout << "CRET" << endl;
#endif
        auto cont = boost::get<StatePtr>( state.ptr.lock()->prim() );
        auto ret = state.ret;
        if (cont) {
            auto oldWind = state.wind;
            auto newWind = cont->wind;
            state = *cont;
            state.sto.push(ret);
            InstrSeq pop = asmCode( makeAssemblerLine(Instr::POP, Reg::STO),
                                    makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::RET));
            state.cont.insert(state.cont.begin(), pop.begin(), pop.end());
            resolveThunks(state, oldWind, newWind);
        } else {
#if DEBUG_INSTR > 0
            cout << "* Not a continuation" << endl;
#endif
        }
    }
        break;
    case Instr::WND: {
#if DEBUG_INSTR > 0
        cout << "WND" << endl;
#endif
        // TODO What if %slf / %ptr are null
        auto before = boost::get<NewMethod>(&state.slf.lock()->prim()),
             after  = boost::get<NewMethod>(&state.ptr.lock()->prim());
        if (before && after) {
            // TODO What if empty dyn stack?
            WindPtr frame = make_shared<WindFrame>();
            frame->before.code = *before;
            frame->before.lex = (*state.slf.lock())[ Symbols::get()["closure"] ].getPtr();
            frame->before.dyn = state.dyn.top();
            frame->after.code = *after;
            frame->after.lex = (*state.ptr.lock())[ Symbols::get()["closure"] ].getPtr();
            frame->before.dyn = state.dyn.top();
            state.wind.push(frame);
        } else {
#if DEBUG_INSTR > 0
        cout << "* Not methods" << endl;
#endif
            state.err0 = true;
        }
    }
        break;
    case Instr::UNWND: {
#if DEBUG_INSTR > 0
        cout << "UNWND" << endl;
#endif
        state.wind.pop();
    }
        break;
    case Instr::THROW: {
#if DEBUG_INSTR > 0
        cout << "THROW" << endl;
#endif
        ObjectPtr exc = state.slf;
        state.stack.push(state.cont);
        deque<ObjectPtr> handlers;
        std::function<void(stack<ObjectPtr>&)> recurse = [&recurse, &handlers](stack<ObjectPtr>& st) {
            if (!st.empty()) {
                ObjectPtr temp = st.top();
                handlers.push_front(temp);
                st.pop();
                recurse(st);
                st.push(temp);
            }
        };
        recurse(state.hand);
        state.cont.clear();
        InstrSeq seq = asmCode(makeAssemblerLine(Instr::PEEK, Reg::ARG),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::POP, Reg::STO),
                               makeAssemblerLine(Instr::CALL, 1L));
        for (ObjectPtr handler : handlers) {
            state.arg.push(exc);
            state.sto.push(handler);
            state.cont.insert(state.cont.begin(), seq.begin(), seq.end());
        }
        InstrSeq term = asmCode(makeAssemblerLine(Instr::CPP, 0L)); // CPP 0 should always be a terminate function
        state.cont.insert(state.cont.end(), term.begin(), term.end());
    }
        break;
    case Instr::THROQ: {
#if DEBUG_INSTR > 0
        cout << "THROQ (" << state.err0 << ")" << endl;
#endif
        if (state.err0) {
            state.stack.push(state.cont);
            state.cont = asmCode( makeAssemblerLine(Instr::THROW) );
        }
    }
        break;
    case Instr::ADDS: {
#if DEBUG_INSTR > 0
        cout << "ADDS" << endl;
#endif
        state.str0 += state.str1;
    }
        break;
    case Instr::ARITH: {
        long val = popLong(state.cont);
#if DEBUG_INSTR > 0
        cout << "ARITH " << val << endl;
#endif
        switch (val) {
        case 1L:
            state.num0 += state.num1;
            break;
        case 2L:
            state.num0 -= state.num1;
            break;
        case 3L:
            state.num0 *= state.num1;
            break;
        case 4L:
            state.num0 /= state.num1;
            break;
        case 5L:
            state.num0 %= state.num1;
            break;
        case 6L:
            state.num0 = state.num0.pow(state.num1);
            break;
        default:
            state.err0 = true;
            break;
        }
    }
        break;
    case Instr::THROA: {
        string msg = popString(state.cont);
#if DEBUG_INSTR > 0
        cout << "THROA \"" << msg << "\"" << endl;
#endif
        if (state.err0)
            throwError(state, "TypeError", msg);
    }
        break;
    case Instr::LOCFN: {
        string msg = popString(state.cont);
#if DEBUG_INSTR > 0
        cout << "LOCFN \"" << msg << "\"" << endl;
#endif
        state.file = msg;
        /*
        InstrSeq seq = asmCode(makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                               makeAssemblerLine(Instr::GETL),
                               makeAssemblerLine(Instr::SYM, "meta"),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::RTRV),
                               makeAssemblerLine(Instr::SYM, "fileStorage"),
                               makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                               makeAssemblerLine(Instr::RTRV),
                               makeAssemblerLine(Instr::GETD),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                               makeAssemblerLine(Instr::EXPD, Reg::SYM),
                               makeAssemblerLine(Instr::POP, Reg::STO),
                               makeAssemblerLine(Instr::SETF));
        state.cont.insert(state.cont.begin(), seq.begin(), seq.end());
        garnishBegin(state, msg);
        */
    }
        break;
    case Instr::LOCLN: {
        long num = popLong(state.cont);
#if DEBUG_INSTR > 0
        cout << "LOCLN " << num << endl;
#endif
        state.line = num;
        /*
        InstrSeq seq = asmCode(makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                               makeAssemblerLine(Instr::GETL),
                               makeAssemblerLine(Instr::SYM, "meta"),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::RTRV),
                               makeAssemblerLine(Instr::SYM, "lineStorage"),
                               makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                               makeAssemblerLine(Instr::RTRV),
                               makeAssemblerLine(Instr::GETD),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                               makeAssemblerLine(Instr::EXPD, Reg::SYM),
                               makeAssemblerLine(Instr::POP, Reg::STO),
                               makeAssemblerLine(Instr::SETF));
        state.cont.insert(state.cont.begin(), seq.begin(), seq.end());
        garnishBegin(state, num);
        */
    }
        break;
    case Instr::LOCRT: {
#if DEBUG_INSTR > 0
        cout << "LOCRT" << endl;
#endif
        function<void(stack<BacktraceFrame>&)> stackUnwind = [&state, &stackUnwind](stack<BacktraceFrame>& stck) {
            if (!stck.empty()) {
                long line;
                string file;
                auto& elem = stck.top();
                tie(line, file) = elem;
                stck.pop();

                InstrSeq step0 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO));
                InstrSeq step1 = garnishSeq(line);
                InstrSeq step2 = asmCode(makeAssemblerLine(Instr::PEEK, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::SYM, "line"),
                                         makeAssemblerLine(Instr::SETF));
                InstrSeq step3 = garnishSeq(file);
                InstrSeq step4 = asmCode(makeAssemblerLine(Instr::POP, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::SYM, "file"),
                                         makeAssemblerLine(Instr::SETF),
                                         makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET));
                state.cont.insert(state.cont.begin(), step4.begin(), step4.end());
                state.cont.insert(state.cont.begin(), step3.begin(), step3.end());
                state.cont.insert(state.cont.begin(), step2.begin(), step2.end());
                state.cont.insert(state.cont.begin(), step1.begin(), step1.end());
                state.cont.insert(state.cont.begin(), step0.begin(), step0.end());

                stackUnwind(stck);

                stck.push(elem);
            }
        };
        state.stack.push(state.cont);
        state.cont.clear();
        stackUnwind(state.trace);
        InstrSeq intro = asmCode(makeAssemblerLine(Instr::GETL),
                                 makeAssemblerLine(Instr::SYM, "meta"),
                                 makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                                 makeAssemblerLine(Instr::RTRV),
                                 makeAssemblerLine(Instr::SYM, "StackFrame"),
                                 makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                 makeAssemblerLine(Instr::RTRV));
        state.cont.insert(state.cont.begin(), intro.begin(), intro.end());
    }
        break;
    }
}

void doOneStep(IntState& state) {
    if (state.cont.empty()) {
        // Pop off the stack
#if DEBUG_INSTR > 0
        cout << "<><><>" << endl;
#endif
        if (!state.stack.empty()) {
            state.cont = state.stack.top();
            state.stack.pop();
            doOneStep(state);
        }
    } else {
        // Run one command
        Instr instr = popInstr(state.cont);
#if DEBUG_INSTR > 0
        cout << (long)instr << endl;
#endif
        executeInstr(instr, state);
    }
}

bool isIdling(IntState& state) {
    return (state.cont.empty() && state.stack.empty());
}

