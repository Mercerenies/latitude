#include "Bytecode.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Assembler.hpp"
#include "GC.hpp"

//#define DEBUG_INSTR 1

using namespace std;

#ifdef PROFILE_INSTR

Profiling Profiling::instance;

Profiling& Profiling::get() noexcept {
    return instance;
}

void Profiling::_begin(unsigned char x) {
    current = x;
    tick = std::chrono::high_resolution_clock::now();
}

void Profiling::_end(unsigned char x) {
    auto now = std::chrono::high_resolution_clock::now();
    assert(current == x);
    times[current] += (now - tick);
    ++counts[current];
}

void Profiling::etcBegin() {
    _begin(0);
}

void Profiling::etcEnd() {
    _end(0);
}

void Profiling::instructionBegin(Instr x) {
    _begin((unsigned char)x);
}

void Profiling::instructionEnd(Instr x) {
    _end((unsigned char)x);
}

void Profiling::dumpData() {
    std::cout << "Profile data:" << std::endl;
    std::cout << "ins\tcount\ttime" << std::endl;
    for (int ch = 0; ch <= 255; ch++) {
        auto count = std::chrono::duration_cast<std::chrono::nanoseconds>(times[ch]);
        std::cout << ch << "\t" << counts[ch] << "\t" << count.count() << " ns" << std::endl;
    }
}

#endif

Thunk::Thunk(Method code)
    : Thunk(code, nullptr, nullptr) {}

Thunk::Thunk(Method code, ObjectPtr lex, ObjectPtr dyn)
    : code(code), lex(lex), dyn(dyn) {}

WindFrame::WindFrame(const Thunk& before, const Thunk& after)
    : before(before), after(after) {}

IntState intState() {
    IntState state;
    state.ptr = state.slf = state.ret = nullptr;
    // lex, dyn, arg, sto default to empty
    // cont default to empty
    // stack default to empty
    // err0, err1 default to false
    state.sym = Symbols::get()[""];
    // num0, num1 default to smallint(0)
    // str0, str1 default to empty string
    // mthd default to empty method
    // strm default to null
    // prcs default to null
    // mthdz default to empty method
    // flag default to false
    // wind default to empty
    // line default to zero
    // file default to empty string
    // trace default to empty stack
    // trns default to empty stack
    return state;
}

StatePtr statePtr(const IntState& state) {
    return make_shared<IntState>(state);
}

StatePtr statePtr(IntState&& state) {
    return make_shared<IntState>(forward<IntState&&>(state));
}

ReadOnlyState readOnlyState() {
    ReadOnlyState reader;
    reader.gtu = make_shared<TranslationUnit>();
    return reader;
}

void hardKill(IntState& state, const ReadOnlyState& reader) {
    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_EMPTY }));
    state.stack = NodePtr<MethodSeek>();
}

void resolveThunks(IntState& state, const ReadOnlyState& reader, NodePtr<WindPtr> oldWind, NodePtr<WindPtr> newWind) {
    deque<WindPtr> exits;
    deque<WindPtr> enters;
    while (oldWind) {
        exits.push_back(oldWind->get());
        oldWind = popNode(oldWind);
    }
    while (newWind) {
        enters.push_front(newWind->get());
        newWind = popNode(newWind);
    }
    // Determine what the two scopes have in common and remove them
    // (There's no reason to run an 'after' and then a corresponding 'before' from the same thunk)
    while ((!exits.empty()) && (!enters.empty()) && (exits.back() == enters.front())) {
        exits.pop_back();
        enters.pop_front();
    }
    state.stack = pushNode(state.stack, state.cont);
    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_EMPTY }));
    for (WindPtr ptr : exits) {
        state.lex.push( clone(ptr->after.lex) );
        state.dyn.push( clone(ptr->after.dyn) );
        state.stack = pushNode(state.stack, MethodSeek(ptr->after.code));
    }
    for (WindPtr ptr : enters) {
        state.lex.push( clone(ptr->before.lex) );
        state.dyn.push( clone(ptr->before.dyn) );
        state.stack = pushNode(state.stack, MethodSeek(ptr->before.code));
    }
}

unsigned char popChar(SerialInstrSeq& state) {
    if (state.empty())
        return 0;
    char val = state.front();
    state.pop_front();
    return val;
}

long popLong(SerialInstrSeq& state) {
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

string popString(SerialInstrSeq& state) { // TODO Null-safety here (null characters will confuse it)
    string str;
    unsigned char ch;
    while (true) {
        ch = popChar(state);
        if (ch == '\0') {
            ch = popChar(state);
            if (ch == '.')
                str += '\0';
            else if (ch == '\0')
                break;
        } else {
            str += ch;
        }
    }
    return str;
}

Reg popReg(SerialInstrSeq& state) {
    unsigned char ch = popChar(state);
    return (Reg)ch;
}

Instr popInstr(SerialInstrSeq& state) {
    unsigned char ch = popChar(state);
    return (Instr)ch;
}

FunctionIndex popFunction(SerialInstrSeq& state) {
    int value = 0;
    int pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)popChar(state);
        pow <<= 8;
    }
    return { value };
}

void executeInstr(Instr instr, IntState& state, const ReadOnlyState& reader) {
    switch (instr) {
    case Instr::MOV: {
        Reg src = state.cont.readReg(0);
        Reg dest = state.cont.readReg(1);
#if DEBUG_INSTR > 0
        cout << "MOV " << (long)src << " " << (long)dest << endl;
#endif
        ObjectPtr mid = nullptr;
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
            mid = nullptr;
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
            state.err0 = true;
            break;
        }
    }
        break;
    case Instr::PUSH: {
        Reg src = state.cont.readReg(0);
        Reg stack = state.cont.readReg(1);
#if DEBUG_INSTR > 0
        cout << "PUSH " << (long)src << " " << (long)stack << endl;
#endif
        ObjectPtr mid = nullptr;
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
            mid = nullptr;
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
            state.err0 = true;
            break;
        }
    }
        break;
    case Instr::POP: {
        stack<ObjectPtr>* stack;
        Reg dest = state.cont.readReg(0);
        Reg reg = state.cont.readReg(1);
        ObjectPtr mid = nullptr;
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
            stack = nullptr;
            state.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                mid = stack->top();
                stack->pop();
            } else {
                state.err0 = true;
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
                state.err0 = true;
                break;
            }
        }
    }
        break;
    case Instr::GETL: {
#if DEBUG_INSTR > 0
        cout << "GETL" << endl;
#endif
        Reg dest = state.cont.readReg(0);
        if (state.lex.empty()) {
            state.err0 = true;
        } else {
            switch (dest) {
            case Reg::PTR:
                state.ptr = state.lex.top();
                break;
            case Reg::SLF:
                state.slf = state.lex.top();
                break;
            case Reg::RET:
                state.ret = state.lex.top();
                break;
            default:
                state.err0 = true;
                break;
            }
        }
    }
        break;
    case Instr::GETD: {
#if DEBUG_INSTR > 0
        cout << "GETD" << endl;
#endif
        Reg dest = state.cont.readReg(0);
        if (state.dyn.empty()) {
            state.err0 = true;
        } else {
            switch (dest) {
            case Reg::PTR:
                state.ptr = state.dyn.top();
                break;
            case Reg::SLF:
                state.slf = state.dyn.top();
                break;
            case Reg::RET:
                state.ret = state.dyn.top();
                break;
            default:
                state.err0 = true;
                break;
            }
        }
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
        string str = state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "SYM \"" << str << "\"" << endl;
#endif
        state.sym = Symbols::get()[str];
    }
        break;
    case Instr::NUM: {
        string str = state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "NUM \"" << str << "\"" << endl;
#endif
        auto temp = parseInteger(str.c_str());
        assert(temp);
        state.num0 = *temp;
    }
        break;
    case Instr::INT: {
        long val = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "INT " << val << endl;
#endif
        state.num0 = Number(val);
    }
        break;
    case Instr::FLOAT: {
        string str = state.cont.readString(0);
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
        long args = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "CALL " << args << " (" << Symbols::get()[state.sym] << ")" << endl;
#if DEBUG_INSTR > 2
        cout << "* Method Properties " << state.ptr << endl;
#endif
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&state.ptr->prim());
        ObjectPtr closure = (*state.ptr)[ Symbols::get()["closure"] ];
#if DEBUG_INSTR > 2
        cout << "* Method Properties " <<
            (closure != nullptr) << " " <<
            (stmt ? stmt->index().index : -1) << " " <<
            (stmt ? stmt->translationUnit() : nullptr) << endl;
#endif
        if ((closure != nullptr) && stmt) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!state.dyn.empty())
                state.dyn.push( clone(state.dyn.top()) );
            else
                state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            auto lex = state.lex.top();
            state.lex.push( clone(closure) );
            // (4) Bind all the local variables
            state.lex.top()->put(Symbols::get()["self"], state.slf);
            state.lex.top()->put(Symbols::get()["again"], state.ptr);
            state.lex.top()->put(Symbols::get()["caller"], lex);
            state.lex.top()->protectAll(Protection::PROTECT_ASSIGN | Protection::PROTECT_DELETE,
                                        Symbols::get()["self"], Symbols::get()["again"]);
            // (5) Push the trace information
            state.trace = pushNode(state.trace, make_tuple(state.line, state.file));
            // (6) Bind all of the arguments
            if (!state.dyn.empty()) {
                int index = args;
                for (long n = 0; n < args; n++) {
                    ObjectPtr arg = state.arg.top();
                    state.arg.pop();
                    state.dyn.top()->put(Symbols::get()[ "$" + to_string(index) ], arg);
                    index--;
                }
            }
            // (7) Push %cont onto %stack
            state.stack = pushNode(state.stack, state.cont);
            state.trns.push(stmt->translationUnit());
            // (8) Make a new %cont
            if (stmt) {
#if DEBUG_INSTR > 3
                cout << "* (cont) " << stmt->size() << endl;
                if (stmt->size() == 0) {
                    // What are we looking at...?
                    cout << "* * Where ptr has" << endl;
                    for (auto& x : keys(state.ptr))
                        cout << "  " << Symbols::get()[x];
                    cout << endl;
                    cout << "* * Directly" << endl;
                    for (auto& x : (state.ptr)->directKeys())
                        cout << "  " << Symbols::get()[x];
                    cout << endl;
                    cout << "* * Parents of ptr" << endl;
                    for (auto& x : hierarchy(state.ptr))
                        cout << "  " << x;
                    cout << endl;
                    cout << "* * Following the prims of ptr" << endl;
                    for (auto& x : hierarchy(state.ptr))
                        cout << "  " << x->prim().which();
                    cout << endl;
                }
#endif
                state.cont = MethodSeek(*stmt);
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
        auto stmt = boost::get<Method>(&state.ptr->prim());
        if (stmt) {
            // (6) Push %cont onto %stack
            state.stack = pushNode(state.stack, state.cont);
            state.trns.push(stmt->translationUnit());
            // (7) Make a new %cont
            if (stmt)
                state.cont = MethodSeek(*stmt);
        }
    }
        break;
    case Instr::XCALL0: {
        long args = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "XCALL0 " << args << " (" << Symbols::get()[state.sym] << ")" << endl;
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&state.ptr->prim());
        ObjectPtr closure = (*state.ptr)[ Symbols::get()["closure"] ];
        if ((closure != nullptr) && stmt) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!state.dyn.empty())
                state.dyn.push( clone(state.dyn.top()) );
            else
                state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            auto lex = state.lex.top();
            state.lex.push( clone(closure) );
            // (4) Bind all the local variables
            state.lex.top()->put(Symbols::get()["self"], state.slf);
            state.lex.top()->put(Symbols::get()["again"], state.ptr);
            state.lex.top()->put(Symbols::get()["caller"], lex);
            state.lex.top()->protectAll(Protection::PROTECT_ASSIGN | Protection::PROTECT_DELETE,
                                        Symbols::get()["self"], Symbols::get()["again"]);
            // (5) Push the trace information
            state.trace = pushNode(state.trace, make_tuple(state.line, state.file));
            // (6) Bind all of the arguments
            if (!state.dyn.empty()) {
                int index = args;
                for (long n = 0; n < args; n++) {
                    ObjectPtr arg = state.arg.top();
                    state.arg.pop();
                    state.dyn.top()->put(Symbols::get()[ "$" + to_string(index) ], arg);
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
        if (!state.trace)
            state.err0 = true;
        else
            state.trace = popNode(state.trace);
        if (state.trns.empty())
            state.err0 = true;
        else
            state.trns.pop();
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
        auto sym = Symbols::parent();
        list<ObjectPtr> parents;
        ObjectPtr curr = state.slf;
        Symbolic name = state.sym;
        Symbolic backup = state.sym;
        ObjectPtr value = nullptr;
        // Try to find the value itself
        while (find(parents.begin(), parents.end(), curr) == parents.end()) {
            parents.push_back(curr);
            ObjectPtr slot = (*curr)[name];
            if (slot != nullptr) {
                value = slot;
                break;
            }
            curr = (*curr)[ sym ];
        }
        if (value == nullptr) {
#if DEBUG_INSTR > 2
            cout << "* Looking for missing" << endl;
#if DEBUG_INSTR > 3
            cout << "* Information:" << endl;
            cout << "* * Lex: " << state.lex.top() << endl;
            cout << "* * Dyn: " << state.dyn.top() << endl;
            cout << "* * Slf: " << state.slf << endl;
#endif
#endif
            // Now try for missing
            name = Symbols::get()["missing"];
            parents.clear();
            curr = state.slf;
            while (find(parents.begin(), parents.end(), curr) == parents.end()) {
                parents.push_back(curr);
                ObjectPtr slot = (*curr)[name];
                if (slot != nullptr) {
                    value = slot;
                    break;
                }
                curr = (*curr)[ sym ];
            }
            state.ret = value;
#if DEBUG_INSTR > 1
            if (value == nullptr)
                cout << "* Found no missing" << endl;
            else
                cout << "* Found missing" << endl;
#endif
            if (value == nullptr) {
                ObjectPtr meta = nullptr;
                // If there is no `missing` either, fall back to the last resort
                name = Symbols::get()["meta"];
                parents.clear();
                if (!state.lex.empty()) {
                    curr = state.lex.top();
                    while (find(parents.begin(), parents.end(), curr) == parents.end()) {
                        parents.push_back(curr);
                        ObjectPtr slot = (*curr)[name];
                        if (slot != nullptr) {
                            value = slot;
                            break;
                        }
                        curr = (*curr)[ sym ];
                    }
                    state.ret = value;
                }
                if (value != nullptr) {
                    curr = value;
                    meta = value;
                    value = nullptr;
                    parents.clear();
                    name = Symbols::get()["missed"];
                    while (find(parents.begin(), parents.end(), curr) == parents.end()) {
                        parents.push_back(curr);
                        ObjectPtr slot = (*curr)[name];
                        if (slot != nullptr) {
                            value = slot;
                            break;
                        }
                        curr = (*curr)[ sym ];
                    }
                    state.ret = value;
                }
#if DEBUG_INSTR > 1
                if (value == nullptr)
                    cout << "* Found no missed" << endl;
                else
                    cout << "* Found missed" << endl;
#endif
                if (value == nullptr) {
                    // Abandon ship!
                    state.stack = pushNode(state.stack, state.cont);
                    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_TERMINATE }));
                } else {
                    state.slf = meta;
                    state.ptr = value;
                    state.stack = pushNode(state.stack, state.cont);
                    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_CALL_ZERO }));
                }
            } else {
                state.sym = backup;
                state.stack = pushNode(state.stack, state.cont);
                state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_MISSING }));
            }
        } else {
#if DEBUG_INSTR > 1
            cout << "* Found " << value << endl;
#if DEBUG_INSTR > 2
            auto stmt = boost::get<Method>(&value->prim());
            cout << "* Method Properties " <<
                (stmt ? stmt->index().index : -1) << " " <<
                (stmt ? stmt->translationUnit() : nullptr) << endl;
#endif
#endif
            state.ret = value;
        }
    }
        break;
    case Instr::RTRVD: {
#if DEBUG_INSTR > 0
        cout << "RTRVD (" << Symbols::get()[state.sym] << ")" << endl;
#endif
        ObjectPtr slot = (*state.slf)[state.sym];
        if (slot != nullptr)
            state.ret = slot;
        else
            state.err0 = true;
    }
        break;
    case Instr::STR: {
        string str = state.cont.readString(0);
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
        Reg expd = state.cont.readReg(0);
#if DEBUG_INSTR > 0
        cout << "EXPD " << (long)expd << endl;
#endif
        switch (expd) {
        case Reg::SYM: {
            auto test = boost::get<Symbolic>(&state.ptr->prim());
            if (test)
                state.sym = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::NUM0: {
            auto test = boost::get<Number>(&state.ptr->prim());
            if (test)
                state.num0 = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::NUM1: {
            auto test = boost::get<Number>(&state.ptr->prim());
            if (test)
                state.num1 = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::STR0: {
            auto test = boost::get<string>(&state.ptr->prim());
            if (test)
                state.str0 = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::STR1: {
            auto test = boost::get<string>(&state.ptr->prim());
            if (test)
                state.str1 = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::MTHD: {
            auto test = boost::get<Method>(&state.ptr->prim());
            if (test)
                state.mthd = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::STRM: {
            auto test = boost::get<StreamPtr>(&state.ptr->prim());
            if (test)
                state.strm = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::PRCS: {
            auto test = boost::get<ProcessPtr>(&state.ptr->prim());
            if (test)
                state.prcs = *test;
            else
                state.err0 = true;
        }
            break;
        case Reg::MTHDZ: {
            auto test = boost::get<Method>(&state.ptr->prim());
            if (test)
                state.mthdz = *test;
            else
                state.err0 = true;
        }
            break;
        default:
            state.err0 = true;
            break;
        }
    }
        break;
    case Instr::MTHD: {
#if DEBUG_INSTR > 0
        cout << "MTHD ..." << endl;
#endif
        FunctionIndex index = state.cont.readFunction(0);
        state.mthd = Method(state.trns.top(), index);
    }
        break;
    case Instr::LOAD: {
        Reg ld = state.cont.readReg(0);
#if DEBUG_INSTR > 0
        cout << "LOAD " << (long)ld << endl;
#endif
        switch (ld) {
        case Reg::SYM: {
            state.ptr->prim(state.sym);
        }
            break;
        case Reg::NUM0: {
            state.ptr->prim(state.num0);
        }
            break;
        case Reg::NUM1: {
            state.ptr->prim(state.num1);
        }
            break;
        case Reg::STR0: {
            state.ptr->prim(state.str0);
        }
            break;
        case Reg::STR1: {
            state.ptr->prim(state.str1);
        }
            break;
        case Reg::MTHD: {
#if DEBUG_INSTR > 1
            cout << "* Method Length " << state.mthd.instructions().size() << endl;
#endif
            state.ptr->prim(state.mthd);
        }
            break;
        case Reg::STRM: {
            state.ptr->prim(state.strm);
        }
            break;
        case Reg::PRCS: {
            state.ptr->prim(state.prcs);
        }
            break;
        case Reg::MTHDZ: {
#if DEBUG_INSTR > 1
            cout << "* Method Length " << state.mthdz.instructions().size() << endl;
#endif
            state.ptr->prim(state.mthdz);
        }
            break;
        default:
            state.err0 = true;
            break;
        }
    }
        break;
    case Instr::SETF: {
#if DEBUG_INSTR > 0
        cout << "SETF (" << Symbols::get()[state.sym] << ")" << endl;
#if DEBUG_INSTR > 2
        cout << "* Information:" << endl;
        cout << "* * Lex: " << state.lex.top() << endl;
        cout << "* * Dyn: " << state.dyn.top() << endl;
        cout << "* * Slf: " << state.slf << endl;
#endif
#endif
        if (state.slf == nullptr)
            state.err0 = true;
        else if (state.slf->isProtected(state.sym, Protection::PROTECT_ASSIGN))
            throwError(state, reader, "ProtectedError");
        else
            state.slf->put(state.sym, state.ptr);
    }
        break;
    case Instr::PEEK: {
        stack<ObjectPtr>* stack;
        Reg dest = state.cont.readReg(0);
        Reg reg = state.cont.readReg(1);
        ObjectPtr mid = nullptr;
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
            stack = nullptr;
            state.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                mid = stack->top();
            } else {
                state.err0 = true;
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
                state.err0 = true;
                break;
            }
        }
    }
        break;
    case Instr::SYMN: {
        long val = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "SYMN " << val << " (" << Symbols::get()[Symbolic{val}] << ")" << endl;
#endif
        state.sym = { val };
    }
        break;
    case Instr::CPP: {
        long val = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "CPP " << val << endl;
#endif
        auto func = reader.cpp.at(val);
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
        state.ret = garnishObject(reader, state.flag);
    }
        break;
    case Instr::TEST: {
#if DEBUG_INSTR > 0
        cout << "TEST" << endl;
#endif
        if (state.slf == nullptr || state.ptr == nullptr)
            state.flag = false;
        else
            state.flag = (state.slf == state.ptr);
    }
        break;
    case Instr::BRANCH: {
#if DEBUG_INSTR > 0
        cout << "BRANCH (" << state.flag << ")" << endl;
#endif
        state.stack = pushNode(state.stack, state.cont);
        if (state.flag) {
#if DEBUG_INSTR > 2
            cout << "* Method ( ) Length " << state.mthd.instructions().size() << endl;
#endif
            state.cont = MethodSeek(state.mthd);
        } else {
#if DEBUG_INSTR > 2
            cout << "* Method (Z) Length " << state.mthdz.instructions().size() << endl;
#endif
            state.cont = MethodSeek(state.mthdz);
        }
    }
        break;
    case Instr::CCALL: {
#if DEBUG_INSTR > 0
        cout << "CCALL" << endl;
#endif
        if (state.slf == nullptr) {
            state.err0 = true;
        } else {
            state.slf->prim( statePtr(state) );
            state.arg.push(state.slf);
            state.stack = pushNode(state.stack, state.cont);
            state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_CALL_ONE }));
        }
    }
        break;
    case Instr::CGOTO: {
#if DEBUG_INSTR > 0
        cout << "CGOTO" << endl;
#endif
        auto cont = boost::get<StatePtr>( state.ptr->prim() );
        if (cont) {
            auto oldWind = state.wind;
            auto newWind = cont->wind;
            state = *cont;
            resolveThunks(state, reader, oldWind, newWind);
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
        auto cont = boost::get<StatePtr>( state.ptr->prim() );
        auto ret = state.ret;
        if (cont) {
            auto oldWind = state.wind;
            auto newWind = cont->wind;
            state = *cont;
            state.ret = ret;
            resolveThunks(state, reader, oldWind, newWind);
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
        auto before = boost::get<Method>(&state.slf->prim()),
             after  = boost::get<Method>(&state.ptr->prim());
        if (before && after) {
            WindPtr frame = WindPtr(new WindFrame(Thunk(*before), Thunk(*after)));
            frame->before.lex = (*state.slf)[ Symbols::get()["closure"] ];
            frame->before.dyn = state.dyn.top();
            frame->after.lex = (*state.ptr)[ Symbols::get()["closure"] ];
            frame->after.dyn = state.dyn.top();
            state.wind = pushNode(state.wind, frame);
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
        state.wind = popNode(state.wind);
    }
        break;
    case Instr::THROW: {
#if DEBUG_INSTR > 0
        cout << "THROW" << endl;
#endif
        ObjectPtr exc = state.slf;
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
#if DEBUG_INSTR > 1
        cout << "* Got handlers: " << handlers.size() << endl;
#endif
        state.stack = pushNode(state.stack, state.cont);
        state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_TERMINATE }));
        for (ObjectPtr handler : handlers) {
            state.arg.push(exc);
            state.sto.push(handler);
            state.stack = pushNode(state.stack, state.cont);
            state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_HANDLER }));
        }
    }
        break;
    case Instr::THROQ: {
#if DEBUG_INSTR > 0
        cout << "THROQ (" << state.err0 << ")" << endl;
#endif
        if (state.err0) {
            state.stack = pushNode(state.stack, state.cont);
            state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_THROW }));
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
        long val = state.cont.readLong(0);
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
        case 7L:
            state.num0 &= state.num1;
            break;
        case 8L:
            state.num0 |= state.num1;
            break;
        case 9L:
            state.num0 ^= state.num1;
            break;
        default:
            state.err0 = true;
            break;
        }
    }
        break;
    case Instr::THROA: {
        string msg = state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "THROA \"" << msg << "\"" << endl;
#endif
        if (state.err0)
            throwError(state, reader, "TypeError", msg);
    }
        break;
    case Instr::LOCFN: {
        string msg = state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "LOCFN \"" << msg << "\"" << endl;
#endif
        state.file = msg;
    }
        break;
    case Instr::LOCLN: {
        long num = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "LOCLN " << num << endl;
#endif
        state.line = num;
    }
        break;
    case Instr::LOCRT: {
#if DEBUG_INSTR > 0
        cout << "LOCRT" << endl;
#endif
        auto stck = state.trace;
        if (stck) {
            ObjectPtr sframe = reader.lit.at(Lit::SFRAME);
            ObjectPtr frame = nullptr;
            ObjectPtr top = nullptr;
            while (stck) {
                ObjectPtr temp;
                long line;
                string file;
                auto elem = stck->get();
                tie(line, file) = elem;
                temp = clone(sframe);
                if (frame == nullptr) {
                    top = temp;
                } else {
                    frame->put(Symbols::parent(), temp);
                }
                frame = temp;
                frame->put(Symbols::get()["line"], garnishObject(reader, line));
                frame->put(Symbols::get()["file"], garnishObject(reader, file));
                stck = popNode(stck);
            }
            assert(top != nullptr); // Should always be non-null since the loop must run once
            state.ret = top;
        } else {
            // The %trace stack was empty; this should not happen...
            // Honestly, this should probably be an assert failure,
            // but %trace is so weird right now that I'm hesitant to
            // rely on it.
            state.ret = reader.lit.at(Lit::NIL);
            // TODO This *should* be an assertion failure, once %trace is reliable
        }
    }
        break;
    case Instr::NRET: {
#if DEBUG_INSTR > 0
        cout << "NRET" << endl;
#endif
        state.trace = pushNode(state.trace, make_tuple(0L, string("")));
        state.stack = pushNode(state.stack, state.cont);
        state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_RETURN }));
    }
        break;
    case Instr::UNTR: {
#if DEBUG_INSTR > 0
        cout << "UNTR" << endl;
#endif
        if (state.trns.empty())
            state.err0 = true;
        else
            state.trns.pop();
    }
        break;
    case Instr::CMPLX: {
        string str0 = state.cont.readString(0);
        string str1 = state.cont.readString(1);
#if DEBUG_INSTR > 0
        cout << "CMPLX" << endl;
#endif
        double rl = strtod(str0.c_str(), NULL);
        double im = strtod(str1.c_str(), NULL);
        state.num0 = Number(Number::complex(rl, im));
    }
        break;
    case Instr::YLD: {
        long val = state.cont.readLong(0);
        Reg reg = state.cont.readReg(1);
#if DEBUG_INSTR > 0
        cout << "YLD " << val << " " << (long)reg << endl;
#endif
        auto obj = reader.lit.at(val);
        if (obj != nullptr) {
            switch (reg) {
            case Reg::PTR:
                state.ptr = obj;
                break;
            case Reg::SLF:
                state.slf = obj;
                break;
            case Reg::RET:
                state.ret = obj;
                break;
            default:
                state.err0 = true;
                break;
            }
        } else {
            state.err0 = true;
        }
    }
        break;
    case Instr::YLDC: {
        long val = state.cont.readLong(0);
        Reg reg = state.cont.readReg(1);
#if DEBUG_INSTR > 0
        cout << "YLDC " << val << " " << (long)reg << endl;
#endif
        auto obj = reader.lit.at(val);
        if (obj != nullptr) {
            obj = clone(obj);
            switch (reg) {
            case Reg::PTR:
                state.ptr = obj;
                break;
            case Reg::SLF:
                state.slf = obj;
                break;
            case Reg::RET:
                state.ret = obj;
                break;
            default:
                state.err0 = true;
                break;
            }
        } else {
            state.err0 = true;
        }
    }
        break;
    case Instr::DEL: {
#if DEBUG_INSTR > 0
        cout << "DEL" << endl;
#endif
        if (state.slf == nullptr) {
            state.err0 = true;
        } else if (state.slf->isProtected(state.sym, Protection::PROTECT_DELETE)) {
            throwError(state, reader, "ProtectedError", "Delete-protected variable");
        } else {
            state.slf->remove(state.sym);
        }
    }
        break;
    case Instr::ARR: {
        long val = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "ARR " << val << endl;
#endif
        ObjectPtr arr = clone(reader.lit.at(Lit::ARRAY));
        int j = 2 * (int)val - 1;
        for (long i = 0; i < val; i++, j -= 2) {
            arr->put(Symbols::natural(j), state.arg.top());
            state.arg.pop();
        }
        arr->put(Symbols::get()["lowerBound"], garnishObject(reader, 0));
        arr->put(Symbols::get()["upperBound"], garnishObject(reader, val));
        state.ret = arr;
    }
        break;
    case Instr::DICT: {
        long val = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "DICT " << val << endl;
#endif
        ObjectPtr dict = reader.lit.at(Lit::DICT);
        ObjectPtr impl = clone((*dict)[Symbols::get()["&impl"]]);
        dict = clone(dict);
        dict->put(Symbols::get()["&impl"], impl);
        for (long i = 0; i < val; i++) {
            ObjectPtr value = state.arg.top();
            state.arg.pop();
            ObjectPtr key = state.arg.top();
            state.arg.pop();
            auto key0 = boost::get<Symbolic>(&key->prim());
            if (key0) {
                if (*key0 == Symbols::get()["missing"]) {
                    dict->put(Symbols::get()["impl0"], value);
                } else if (*key0 == Symbols::get()["parent"]) {
                    dict->put(Symbols::get()["impl1"], value);
                } else {
                    impl->put(*key0, value);
                }
            } else {
                throwError(state, reader, "TypeError", "Symbol expected");
                return;
            }
        }
        state.ret = dict;
    }
        break;
    case Instr::XXX: {
        long val = state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "XXX " << val << endl;
#endif
        static NodePtr<BacktraceFrame> trace_marker = nullptr;
        switch (val) {
        case 0:
            // Store
            trace_marker = state.trace;
            break;
        case 1: {
            // Compare
            auto temp = state.trace;
            while ((trace_marker != nullptr) && (temp != nullptr)) {
                if (trace_marker->get() != temp->get()) {
                    // Welp
                    cerr << "Trace assertion failure!" << endl;
                    hardKill(state, reader);
                }
            }
            if (trace_marker != temp) {
                // Welp
                cerr << "Trace assertion failure!" << endl;
                hardKill(state, reader);
            }
            trace_marker = nullptr;
        }
            break;
        }
    }
        break;
    }
}

void doOneStep(IntState& state, const ReadOnlyState& reader) {
    state.cont.advancePosition(1);
    if (state.cont.atEnd()) {
        // Pop off the stack
#if DEBUG_INSTR > 1
        cout << "<><><>" << endl;
#endif
        if (state.stack) {
#ifdef PROFILE_INSTR
            Profiling::get().etcBegin();
#endif
            state.cont = state.stack->get();
            state.stack = popNode(state.stack);
#ifdef PROFILE_INSTR
            Profiling::get().etcEnd();
#endif
            doOneStep(state, reader);
        }
    } else {
        // Run one command
        Instr instr = state.cont.readInstr();
#if DEBUG_INSTR > 1
        cout << "<" << (long)instr << ">" << endl;
#endif
#ifdef PROFILE_INSTR
        Profiling::get().instructionBegin(instr);
#endif
        executeInstr(instr, state, reader);
#ifdef PROFILE_INSTR
        Profiling::get().instructionEnd(instr);
#endif
        GC::get().tick(state, reader);
    }
}

bool isIdling(IntState& state) {
    return (state.cont.atEnd() && !state.stack);
}
