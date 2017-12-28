#include "Bytecode.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Assembler.hpp"

#define DEBUG_INSTR 0

/*
 * X 1. Add `caller` to `lexical`.
 * X 2. Change standard library to not jump between scopes.
 * X 3. Cut out `$lexical` and `dynamic`.
 * X 4. Eliminate `pairScopes`.
 * X 5. Ref counting.
 *   6. Update docs before merging.
 *
 * X *. Also, `scope` and `$scope` aren't really necessary
 */

using namespace std;

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
    return ReadOnlyState();
}

void hardKill(IntState& state) {
    state.cont.killSelf();
    state.stack = NodePtr<SeekHolder>();
}

void resolveThunks(IntState& state, NodePtr<WindPtr> oldWind, NodePtr<WindPtr> newWind) {
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
    state.cont = CodeSeek();
    for (WindPtr ptr : exits) {
        state.lex.push( clone(ptr->after.lex) );
        state.dyn.push( clone(ptr->after.dyn) );
        state.stack = pushNode(state.stack, SeekHolder(MethodSeek(ptr->after.code)));
    }
    for (WindPtr ptr : enters) {
        state.lex.push( clone(ptr->before.lex) );
        state.dyn.push( clone(ptr->before.dyn) );
        state.stack = pushNode(state.stack, SeekHolder(MethodSeek(ptr->before.code)));
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

string popString(SerialInstrSeq& state) {
    string str;
    unsigned char ch;
    while ((ch = popChar(state)) != 0)
        str += ch;
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
        cout << "* " << state.lex.top() << endl;
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
#if DEBUG_INSTR > 1
        cout << "* " << state.dyn.top() << endl;
#endif
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
        state.num0 = Number(static_cast<Number::bigint>(str));
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
#if DEBUG_INSTR > 1
        cout << "* Method Properties " << state.ptr << endl;
#endif
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&state.ptr->prim());
        Slot closure = (*state.ptr)[ Symbols::get()["closure"] ];
#if DEBUG_INSTR > 2
        cout << "* Method Properties " <<
            (closure.getType() == SlotType::PTR) << " " <<
            (stmt ? stmt->index().index : -1) << " " <<
            (stmt ? stmt->translationUnit() : nullptr) << endl;
#endif
        if ((closure.getType() == SlotType::PTR) && stmt) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!state.dyn.empty())
                state.dyn.push( clone(state.dyn.top()) );
            else
                state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            auto lex = state.lex.top(); // TODO Possible empty stack error?
            state.lex.push( clone(closure.getPtr()) );
            // (4) Bind all the local variables
            state.lex.top()->put(Symbols::get()["self"], state.slf);
            state.lex.top()->put(Symbols::get()["again"], state.ptr);
            state.lex.top()->put(Symbols::get()["lexical"], state.lex.top());
            state.lex.top()->put(Symbols::get()["caller"], lex);
            if (!state.dyn.empty()) {
                state.dyn.top()->put(Symbols::get()["$dynamic"], state.dyn.top());
                state.dyn.top()->protectAll(PROTECT_ASSIGN | PROTECT_DELETE,
                                            Symbols::get()["$dynamic"]);
            }
            state.lex.top()->protectAll(PROTECT_ASSIGN | PROTECT_DELETE,
                                        Symbols::get()["self"], Symbols::get()["again"],
                                        Symbols::get()["lexical"]);
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
        Slot closure = (*state.ptr)[ Symbols::get()["closure"] ];
        if ((closure.getType() == SlotType::PTR) && stmt) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!state.dyn.empty())
                state.dyn.push( clone(state.dyn.top()) );
            else
                state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            auto lex = state.lex.top(); // TODO Possible empty stack error?
            state.lex.push( clone(closure.getPtr()) );
            // (4) Bind all the local variables
            state.lex.top()->put(Symbols::get()["self"], state.slf);
            state.lex.top()->put(Symbols::get()["again"], state.ptr);
            state.lex.top()->put(Symbols::get()["lexical"], state.lex.top());
            state.lex.top()->put(Symbols::get()["caller"], lex);
            if (!state.dyn.empty()) {
                state.dyn.top()->put(Symbols::get()["$dynamic"], state.dyn.top());
                state.dyn.top()->protectAll(PROTECT_ASSIGN | PROTECT_DELETE,
                                            Symbols::get()["$dynamic"]);
            }
            state.lex.top()->protectAll(PROTECT_ASSIGN | PROTECT_DELETE,
                                        Symbols::get()["self"], Symbols::get()["again"],
                                        Symbols::get()["lexical"]);
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
        auto sym = Symbols::get()["parent"];
        list<ObjectPtr> parents;
        ObjectPtr curr = state.slf;
        Symbolic name = state.sym;
        Symbolic backup = state.sym;
        ObjectPtr value = nullptr;
        // Try to find the value itself
        while (find(parents.begin(), parents.end(), curr) == parents.end()) {
            parents.push_back(curr);
            Slot slot = (*curr)[name];
            if (slot.getType() == SlotType::PTR) {
                value = slot.getPtr();
                break;
            }
            curr = (*curr)[ sym ].getPtr();
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
                Slot slot = (*curr)[name];
                if (slot.getType() == SlotType::PTR) {
                    value = slot.getPtr();
                    break;
                }
                curr = (*curr)[ sym ].getPtr();
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
                        Slot slot = (*curr)[name];
                        if (slot.getType() == SlotType::PTR) {
                            value = slot.getPtr();
                            break;
                        }
                        curr = (*curr)[ sym ].getPtr();
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
                        Slot slot = (*curr)[name];
                        if (slot.getType() == SlotType::PTR) {
                            value = slot.getPtr();
                            break;
                        }
                        curr = (*curr)[ sym ].getPtr();
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
                    InstrSeq term = asmCode(makeAssemblerLine(Instr::CPP, 0));
                    state.stack = pushNode(state.stack, state.cont);
                    state.cont = CodeSeek(term);
                } else {
                    InstrSeq seq0;
                    state.slf = meta;
                    state.ptr = value;
                    (makeAssemblerLine(Instr::CALL, 0L)).appendOnto(seq0);
                    state.stack = pushNode(state.stack, state.cont);
                    state.cont = CodeSeek(move(seq0));
                }
            } else {
                InstrSeq seq0;
                // Find the literal object to use for the argument
                (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq0);
                (makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO)).appendOnto(seq0);
                (makeAssemblerLine(Instr::YLDC, Lit::SYMBOL, Reg::PTR)).appendOnto(seq0);
                // Clone and put a prim() onto it
                (makeAssemblerLine(Instr::SYMN, backup.index)).appendOnto(seq0);
                (makeAssemblerLine(Instr::LOAD, Reg::SYM)).appendOnto(seq0);
                (makeAssemblerLine(Instr::PUSH, Reg::PTR, Reg::ARG)).appendOnto(seq0);
                (makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO)).appendOnto(seq0);
                // Call it
                (makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO)).appendOnto(seq0);
                (makeAssemblerLine(Instr::CALL, 1L)).appendOnto(seq0);
                state.stack = pushNode(state.stack, state.cont);
                state.cont = CodeSeek(move(seq0));
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
        Slot slot = (*state.slf)[state.sym];
        if (slot.getType() == SlotType::PTR)
            state.ret = slot.getPtr();
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
        else if (state.slf->isProtected(state.sym, PROTECT_ASSIGN))
            throwError(state, "ProtectedError");
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
        cout << "SYMN " << val << endl;
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
        garnishBegin(state, state.flag);
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
            InstrSeq seq = asmCode( makeAssemblerLine(Instr::CALL, 1L) );
            state.stack = pushNode(state.stack, state.cont);
            state.cont = CodeSeek(seq);
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
        auto cont = boost::get<StatePtr>( state.ptr->prim() );
        auto ret = state.ret;
        if (cont) {
            auto oldWind = state.wind;
            auto newWind = cont->wind;
            state = *cont;
            state.sto.push(ret);
            InstrSeq pop = asmCode(makeAssemblerLine(Instr::POP, Reg::RET, Reg::STO));
            state.stack = pushNode(state.stack, state.cont);
            state.cont = CodeSeek(pop);
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
        auto before = boost::get<Method>(&state.slf->prim()),
             after  = boost::get<Method>(&state.ptr->prim());
        if (before && after) {
            WindPtr frame = WindPtr(new WindFrame(Thunk(*before), Thunk(*after)));
            frame->before.lex = (*state.slf)[ Symbols::get()["closure"] ].getPtr();
            frame->before.dyn = state.dyn.top();
            frame->after.lex = (*state.ptr)[ Symbols::get()["closure"] ].getPtr();
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
#if DEBUG_INSTR > 0
        cout << "* Got handlers: " << handlers.size() << endl;
#endif
        InstrSeq term = asmCode(makeAssemblerLine(Instr::CPP, 0L)); // CPP 0 should always be a terminate function
        state.stack = pushNode(state.stack, state.cont);
        state.cont = CodeSeek(term);
        InstrSeq seq = asmCode(makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::ARG),
                               makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
                               makeAssemblerLine(Instr::CALL, 1L));
        for (ObjectPtr handler : handlers) {
            state.arg.push(exc);
            state.sto.push(handler);
            state.stack = pushNode(state.stack, state.cont);
            state.cont = CodeSeek(seq);
        }
    }
        break;
    case Instr::THROQ: {
#if DEBUG_INSTR > 0
        cout << "THROQ (" << state.err0 << ")" << endl;
#endif
        if (state.err0) {
            state.stack = pushNode(state.stack, state.cont);
            state.cont = CodeSeek( asmCode( makeAssemblerLine(Instr::THROW) ) );
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
            throwError(state, "TypeError", msg);
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
        InstrSeq total;
        auto stck = state.trace;
        while (stck) {
            long line;
            string file;
            auto elem = stck->get();
            tie(line, file) = elem;
            stck = popNode(stck);

            InstrSeq step0 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                     makeAssemblerLine(Instr::CLONE),
                                     makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO));
            InstrSeq step1 = garnishSeq(line);
            InstrSeq step2 = asmCode(makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                     makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                     makeAssemblerLine(Instr::SYMN, Symbols::get()["line"].index),
                                     makeAssemblerLine(Instr::SETF));
            InstrSeq step3 = garnishSeq(file);
            InstrSeq step4 = asmCode(makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                     makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                     makeAssemblerLine(Instr::SYMN, Symbols::get()["file"].index),
                                     makeAssemblerLine(Instr::SETF),
                                     makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET));
            total.insert(total.begin(), step4.begin(), step4.end());
            total.insert(total.begin(), step3.begin(), step3.end());
            total.insert(total.begin(), step2.begin(), step2.end());
            total.insert(total.begin(), step1.begin(), step1.end());
            total.insert(total.begin(), step0.begin(), step0.end());

        }
        InstrSeq intro = asmCode(makeAssemblerLine(Instr::YLD, Lit::SFRAME, Reg::RET));
        total.insert(total.begin(), intro.begin(), intro.end());
        state.stack = pushNode(state.stack, state.cont);
        state.cont = CodeSeek(move(total));
    }
        break;
    case Instr::NRET: {
#if DEBUG_INSTR > 0
        cout << "NRET" << endl;
#endif
        state.trace = pushNode(state.trace, make_tuple(0L, string("")));
        InstrSeq seq = asmCode(makeAssemblerLine(Instr::RET));
        state.stack = pushNode(state.stack, state.cont);
        state.cont = CodeSeek(seq);
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
        } else if (state.slf->isProtected(state.sym, PROTECT_DELETE)) {
            throwError(state, "ProtectedError", "Delete-protected variable");
        } else {
            state.slf->remove(state.sym);
        }
    }
        break;
    }
}

void doOneStep(IntState& state, const ReadOnlyState& reader) {
    state.cont.advancePosition(1);
    if (state.cont.atEnd()) {
        // Pop off the stack
#if DEBUG_INSTR > 0
        cout << "<><><>" << endl;
#endif
        if (state.stack) {
            state.cont = state.stack->get();
            state.stack = popNode(state.stack);
            doOneStep(state, reader);
        }
    } else {
        // Run one command
        Instr instr = state.cont.readInstr();
#if DEBUG_INSTR > 0
        cout << (long)instr << endl;
#endif
        executeInstr(instr, state, reader);
    }
}

bool isIdling(IntState& state) {
    return (state.cont.atEnd() && !state.stack);
}

