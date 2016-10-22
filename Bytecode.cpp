#include "Bytecode.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Assembler.hpp"

// TODO Make some standard test cases that can be run as a module

//#define DEBUG_INSTR 2

using namespace std;

StackNode::StackNode(const SeekHolder& data0)
    : next(nullptr), data(data0) {}

const SeekHolder& StackNode::get() {
    return data;
}

NodePtr pushNode(NodePtr node, const SeekHolder& data) {
    NodePtr ptr = NodePtr(new StackNode(data));
    ptr->next = node;
    return ptr;
}

NodePtr popNode(NodePtr node) {
    return node->next;
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
    // mthd default to empty method
    // cpp default to empty map
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
    state.stack = pushNode(state.stack, state.cont);
    state.cont = CodeSeek();
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

FunctionIndex popFunction(InstrSeq& state) {
    int value = 0;
    int pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)popChar(state);
        pow <<= 8;
    }
    return { value };
}

void executeInstr(Instr instr, IntState& state) {
    switch (instr) {
    case Instr::MOV: {
        Reg src = state.cont.popReg();
        Reg dest = state.cont.popReg();
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
            mid = ObjectPtr();
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
        Reg src = state.cont.popReg();
        Reg stack = state.cont.popReg();
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
            mid = ObjectPtr();
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
        Reg dest = state.cont.popReg();
        Reg reg = state.cont.popReg();
        ObjectPtr mid;
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
        cout << "* " << state.lex.top().lock() << endl;
#endif
        Reg dest = state.cont.popReg();
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
        cout << "* " << state.dyn.top().lock() << endl;
#endif
#endif
        Reg dest = state.cont.popReg();
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
        string str = state.cont.popString();
#if DEBUG_INSTR > 0
        cout << "SYM \"" << str << "\"" << endl;
#endif
        state.sym = Symbols::get()[str];
    }
        break;
    case Instr::NUM: {
        string str = state.cont.popString();
#if DEBUG_INSTR > 0
        cout << "NUM \"" << str << "\"" << endl;
#endif
        state.num0 = Number(static_cast<Number::bigint>(str));
    }
        break;
    case Instr::INT: {
        long val = state.cont.popLong();
#if DEBUG_INSTR > 0
        cout << "INT " << val << endl;
#endif
        state.num0 = Number(val);
    }
        break;
    case Instr::FLOAT: {
        string str = state.cont.popString();
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
        long args = state.cont.popLong();
#if DEBUG_INSTR > 0
        cout << "CALL " << args << " (" << Symbols::get()[state.sym] << ")" << endl;
#if DEBUG_INSTR > 1
        cout << "* Method Properties " << state.ptr.lock() << endl;
#endif
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&state.ptr.lock()->prim());
        Slot closure = (*state.ptr.lock())[ Symbols::get()["closure"] ];
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
        auto stmt = boost::get<Method>(&state.ptr.lock()->prim());
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
        long args = state.cont.popLong();
#if DEBUG_INSTR > 0
        cout << "XCALL0 " << args << " (" << Symbols::get()[state.sym] << ")" << endl;
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&state.ptr.lock()->prim());
        Slot closure = (*state.ptr.lock())[ Symbols::get()["closure"] ];
        if ((closure.getType() == SlotType::PTR) && stmt) {
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
        list<ObjectSPtr> parents;
        ObjectSPtr curr = state.slf.lock();
        Symbolic name = state.sym;
        Symbolic backup = state.sym;
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
#if DEBUG_INSTR > 1
            if (value.expired())
                cout << "* Found no missing" << endl;
            else
                cout << "* Found missing" << endl;
#endif
            if (value.expired()) {
                // If there is no `missing` either, immediately terminate, as
                // something has gone horribly wrong
                InstrSeq term = asmCode(makeAssemblerLine(Instr::CPP, 0));
                state.stack = pushNode(state.stack, state.cont);
                state.cont = CodeSeek(term);
            } else {
                InstrSeq seq0;
                // Find the literal object to use for the argument
                (makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO)).appendOnto(seq0);
                (makeAssemblerLine(Instr::PUSH, Reg::SLF, Reg::STO)).appendOnto(seq0);
                (makeAssemblerLine(Instr::GETL, Reg::SLF)).appendOnto(seq0);
                (makeAssemblerLine(Instr::SYM, "meta")).appendOnto(seq0);
                (makeAssemblerLine(Instr::RTRV)).appendOnto(seq0);
                (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq0);
                (makeAssemblerLine(Instr::SYM, "Symbol")).appendOnto(seq0);
                (makeAssemblerLine(Instr::RTRV)).appendOnto(seq0);
                (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF)).appendOnto(seq0);
                // Clone and put a prim() onto it
                (makeAssemblerLine(Instr::CLONE)).appendOnto(seq0);
                (makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR)).appendOnto(seq0);
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
            cout << "* Found " << value.lock() << endl;
#if DEBUG_INSTR > 2
            auto stmt = boost::get<Method>(&value.lock()->prim());
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
        Slot slot = (*state.slf.lock())[state.sym];
        if (slot.getType() == SlotType::PTR)
            state.ret = slot.getPtr();
        else
            state.err0 = true;
    }
        break;
    case Instr::STR: {
        string str = state.cont.popString();
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
        Reg expd = state.cont.popReg();
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
            auto test = boost::get<Method>(&state.ptr.lock()->prim());
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
            auto test = boost::get<Method>(&state.ptr.lock()->prim());
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
        FunctionIndex index = state.cont.popFunction();
        state.mthd = Method(state.trns.top(), index);
    }
        break;
    case Instr::LOAD: {
        Reg ld = state.cont.popReg();
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
            cout << "* Method Length " << state.mthd.instructions().size() << endl;
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
            cout << "* Method Length " << state.mthdz.instructions().size() << endl;
#endif
            state.ptr.lock()->prim(state.mthdz);
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
        Reg dest = state.cont.popReg();
        Reg reg = state.cont.popReg();
        ObjectPtr mid;
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
        long val = state.cont.popLong();
#if DEBUG_INSTR > 0
        cout << "SYMN " << val << endl;
#endif
        state.sym = { val };
    }
        break;
    case Instr::CPP: {
        long val = state.cont.popLong();
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
        if (state.slf.expired()) {
            state.err0 = true;
        } else {
            state.slf.lock()->prim( statePtr(state) );
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
        auto before = boost::get<Method>(&state.slf.lock()->prim()),
             after  = boost::get<Method>(&state.ptr.lock()->prim());
        if (before && after) {
            WindPtr frame = WindPtr(new WindFrame());
            frame->before.code = *before;
            frame->before.lex = (*state.slf.lock())[ Symbols::get()["closure"] ].getPtr();
            frame->before.dyn = state.dyn.top();
            frame->after.code = *after;
            frame->after.lex = (*state.ptr.lock())[ Symbols::get()["closure"] ].getPtr();
            frame->after.dyn = state.dyn.top();
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
        long val = state.cont.popLong();
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
        string msg = state.cont.popString();
#if DEBUG_INSTR > 0
        cout << "THROA \"" << msg << "\"" << endl;
#endif
        if (state.err0)
            throwError(state, "TypeError", msg);
    }
        break;
    case Instr::LOCFN: {
        string msg = state.cont.popString();
#if DEBUG_INSTR > 0
        cout << "LOCFN \"" << msg << "\"" << endl;
#endif
        state.file = msg;
        /*
        InstrSeq seq = asmCode(makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                               makeAssemblerLine(Instr::GETL, Reg::PTR),
                               makeAssemblerLine(Instr::SYM, "meta"),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::RTRV),
                               makeAssemblerLine(Instr::SYM, "fileStorage"),
                               makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                               makeAssemblerLine(Instr::RTRV),
                               makeAssemblerLine(Instr::GETD, Reg::PTR),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                               makeAssemblerLine(Instr::EXPD, Reg::SYM),
                               makeAssemblerLine(Instr::POP, Pop::PTR, Reg::STO),
                               makeAssemblerLine(Instr::SETF));
        state.cont.insert(state.cont.begin(), seq.begin(), seq.end());
        garnishBegin(state, msg);
        */
    }
        break;
    case Instr::LOCLN: {
        long num = state.cont.popLong();
#if DEBUG_INSTR > 0
        cout << "LOCLN " << num << endl;
#endif
        state.line = num;
        /*
        InstrSeq seq = asmCode(makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO),
                               makeAssemblerLine(Instr::GETL, Reg::PTR),
                               makeAssemblerLine(Instr::SYM, "meta"),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::RTRV),
                               makeAssemblerLine(Instr::SYM, "lineStorage"),
                               makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                               makeAssemblerLine(Instr::RTRV),
                               makeAssemblerLine(Instr::GETD, Reg::PTR),
                               makeAssemblerLine(Instr::MOV, Reg::PTR, Reg::SLF),
                               makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                               makeAssemblerLine(Instr::EXPD, Reg::SYM),
                               makeAssemblerLine(Instr::POP, Reg::PTR, Reg::STO),
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
        InstrSeq total;
        function<void(stack<BacktraceFrame>&)> stackUnwind = [&total, &stackUnwind](stack<BacktraceFrame>& stck) {
            if (!stck.empty()) {
                long line;
                string file;
                auto elem = stck.top();
                tie(line, file) = elem;
                stck.pop();

                InstrSeq step0 = asmCode(makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                         makeAssemblerLine(Instr::CLONE),
                                         makeAssemblerLine(Instr::PUSH, Reg::RET, Reg::STO));
                InstrSeq step1 = garnishSeq(line);
                InstrSeq step2 = asmCode(makeAssemblerLine(Instr::PEEK, Reg::SLF, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::SYM, "line"),
                                         makeAssemblerLine(Instr::SETF));
                InstrSeq step3 = garnishSeq(file);
                InstrSeq step4 = asmCode(makeAssemblerLine(Instr::POP, Reg::SLF, Reg::STO),
                                         makeAssemblerLine(Instr::MOV, Reg::RET, Reg::PTR),
                                         makeAssemblerLine(Instr::SYM, "file"),
                                         makeAssemblerLine(Instr::SETF),
                                         makeAssemblerLine(Instr::MOV, Reg::SLF, Reg::RET));
                total.insert(total.begin(), step4.begin(), step4.end());
                total.insert(total.begin(), step3.begin(), step3.end());
                total.insert(total.begin(), step2.begin(), step2.end());
                total.insert(total.begin(), step1.begin(), step1.end());
                total.insert(total.begin(), step0.begin(), step0.end());

                stackUnwind(stck);

                stck.push(elem);
            }
        };
        stackUnwind(state.trace);
        InstrSeq intro = asmCode(makeAssemblerLine(Instr::GETL, Reg::SLF),
                                 makeAssemblerLine(Instr::SYM, "meta"),
                                 makeAssemblerLine(Instr::RTRV),
                                 makeAssemblerLine(Instr::SYM, "StackFrame"),
                                 makeAssemblerLine(Instr::MOV, Reg::RET, Reg::SLF),
                                 makeAssemblerLine(Instr::RTRV));
        total.insert(total.begin(), intro.begin(), intro.end());
        state.stack = pushNode(state.stack, state.cont);
        state.cont = CodeSeek(move(total));
    }
        break;
    case Instr::NRET: {
#if DEBUG_INSTR > 0
        cout << "NRET" << endl;
#endif
        state.trace.push( make_tuple(0, "") );
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
    }
}

void doOneStep(IntState& state) {
    if (state.cont.atEnd()) {
        // Pop off the stack
#if DEBUG_INSTR > 0
        cout << "<><><>" << endl;
#endif
        if (state.stack) {
            state.cont = state.stack->get();
            state.stack = popNode(state.stack);
            doOneStep(state);
        }
    } else {
        // Run one command
        Instr instr = state.cont.popInstr();
#if DEBUG_INSTR > 0
        cout << (long)instr << endl;
#endif
        executeInstr(instr, state);
    }
}

bool isIdling(IntState& state) {
    return (state.cont.atEnd() && !state.stack);
}

