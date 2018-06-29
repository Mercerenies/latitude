//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "Bytecode.hpp"
#include "Reader.hpp"
#include "Garnish.hpp"
#include "Standard.hpp"
#include "Assembler.hpp"
#include "Parents.hpp"
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

Thunk::Thunk(Method code, ObjectPtr lex, ObjectPtr dyn)
    : code(code), lex(lex), dyn(dyn) {}

WindFrame::WindFrame(const Thunk& before, const Thunk& after)
    : before(before), after(after) {}

VMState::VMState(IntState& state, const ReadOnlyState& reader)
    : state(state), reader(reader) {}

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
    state.line = 0;
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

void hardKill(VMState& vm) {
    vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_EMPTY }));
    vm.state.stack = NodePtr<MethodSeek>();
}

void resolveThunks(VMState& vm, NodePtr<WindPtr> oldWind, NodePtr<WindPtr> newWind) {
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
    vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
    vm.state.stack = pushNode(vm.state.stack,
                             MethodSeek(Method(vm.reader.gtu, { Table::GTU_UNSTORED })));
    vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_STORED }));
    for (WindPtr ptr : exits) {
        vm.state.lex.push( clone(ptr->after.lex) );
        vm.state.dyn.push( clone(ptr->after.dyn) );
        vm.state.stack = pushNode(vm.state.stack, MethodSeek(ptr->after.code));
    }
    for (WindPtr ptr : enters) {
        vm.state.lex.push( clone(ptr->before.lex) );
        vm.state.dyn.push( clone(ptr->before.dyn) );
        vm.state.stack = pushNode(vm.state.stack, MethodSeek(ptr->before.code));
    }
}

void pushTrace(IntState& state) {
    state.trace = pushNode(state.trace, make_tuple(state.line, state.file));
}

void popTrace(IntState& state) {
    auto curr = state.trace->get();
    state.trace = popNode(state.trace);
    state.line = std::get<0>(curr);
    state.file = std::get<1>(curr);
}

void executeInstr(Instr instr, VMState& vm) {
    switch (instr) {
    case Instr::MOV: {
        Reg src = vm.state.cont.readReg(0);
        Reg dest = vm.state.cont.readReg(1);
#if DEBUG_INSTR > 0
        cout << "MOV " << (long)src << " " << (long)dest << endl;
#endif
        ObjectPtr mid = nullptr;
        switch (src) {
        case Reg::PTR:
            mid = vm.state.ptr;
            break;
        case Reg::SLF:
            mid = vm.state.slf;
            break;
        case Reg::RET:
            mid = vm.state.ret;
            break;
        default:
            mid = nullptr;
            vm.state.err0 = true;
            break;
        }
        switch (dest) {
        case Reg::PTR:
            vm.state.ptr = mid;
            break;
        case Reg::SLF:
            vm.state.slf = mid;
            break;
        case Reg::RET:
            vm.state.ret = mid;
            break;
        default:
            vm.state.err0 = true;
            break;
        }
    }
        break;
    case Instr::PUSH: {
        Reg src = vm.state.cont.readReg(0);
        Reg stack = vm.state.cont.readReg(1);
#if DEBUG_INSTR > 0
        cout << "PUSH " << (long)src << " " << (long)stack << endl;
#endif
        ObjectPtr mid = nullptr;
        switch (src) {
        case Reg::PTR:
            mid = vm.state.ptr;
            break;
        case Reg::SLF:
            mid = vm.state.slf;
            break;
        case Reg::RET:
            mid = vm.state.ret;
            break;
        default:
            mid = nullptr;
            vm.state.err0 = true;
            break;
        }
        switch (stack) {
        case Reg::LEX:
            vm.state.lex.push(mid);
            break;
        case Reg::DYN:
            vm.state.dyn.push(mid);
            break;
        case Reg::ARG:
            vm.state.arg.push(mid);
            break;
        case Reg::STO:
            vm.state.sto.push(mid);
            break;
        case Reg::HAND:
            vm.state.hand.push(mid);
            break;
        default:
            vm.state.err0 = true;
            break;
        }
    }
        break;
    case Instr::POP: {
        stack<ObjectPtr>* stack;
        Reg dest = vm.state.cont.readReg(0);
        Reg reg = vm.state.cont.readReg(1);
        ObjectPtr mid = nullptr;
#if DEBUG_INSTR > 0
        cout << "POP " << (long)reg << endl;
#endif
        switch (reg) {
        case Reg::LEX:
            stack = &vm.state.lex;
            break;
        case Reg::DYN:
            stack = &vm.state.dyn;
            break;
        case Reg::ARG:
            stack = &vm.state.arg;
            break;
        case Reg::STO:
            stack = &vm.state.sto;
            break;
        case Reg::HAND:
            stack = &vm.state.hand;
            break;
        default:
            stack = nullptr;
            vm.state.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                mid = stack->top();
                stack->pop();
            } else {
                vm.state.err0 = true;
            }
            switch (dest) {
            case Reg::PTR:
                vm.state.ptr = mid;
                break;
            case Reg::SLF:
                vm.state.slf = mid;
                break;
            case Reg::RET:
                vm.state.ret = mid;
                break;
            default:
                vm.state.err0 = true;
                break;
            }
        }
    }
        break;
    case Instr::GETL: {
#if DEBUG_INSTR > 0
        cout << "GETL" << endl;
#endif
        Reg dest = vm.state.cont.readReg(0);
        if (vm.state.lex.empty()) {
            vm.state.err0 = true;
        } else {
            switch (dest) {
            case Reg::PTR:
                vm.state.ptr = vm.state.lex.top();
                break;
            case Reg::SLF:
                vm.state.slf = vm.state.lex.top();
                break;
            case Reg::RET:
                vm.state.ret = vm.state.lex.top();
                break;
            default:
                vm.state.err0 = true;
                break;
            }
        }
    }
        break;
    case Instr::GETD: {
#if DEBUG_INSTR > 0
        cout << "GETD" << endl;
#endif
        Reg dest = vm.state.cont.readReg(0);
        if (vm.state.dyn.empty()) {
            vm.state.err0 = true;
        } else {
            switch (dest) {
            case Reg::PTR:
                vm.state.ptr = vm.state.dyn.top();
                break;
            case Reg::SLF:
                vm.state.slf = vm.state.dyn.top();
                break;
            case Reg::RET:
                vm.state.ret = vm.state.dyn.top();
                break;
            default:
                vm.state.err0 = true;
                break;
            }
        }
    }
        break;
    case Instr::ESWAP: {
#if DEBUG_INSTR > 0
        cout << "ESWAP" << endl;
#endif
        swap(vm.state.err0, vm.state.err1);
    }
        break;
    case Instr::ECLR: {
#if DEBUG_INSTR > 0
        cout << "ECLR" << endl;
#endif
        vm.state.err0 = false;
    }
        break;
    case Instr::ESET: {
#if DEBUG_INSTR > 0
        cout << "ESET" << endl;
#endif
        vm.state.err0 = true;
    }
        break;
    case Instr::SYM: {
        string str = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "SYM \"" << str << "\"" << endl;
#endif
        vm.state.sym = Symbols::get()[str];
    }
        break;
    case Instr::NUM: {
        string str = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "NUM \"" << str << "\"" << endl;
#endif
        auto temp = parseInteger(str.c_str());
        assert(temp);
        vm.state.num0 = *temp;
    }
        break;
    case Instr::INT: {
        long val = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "INT " << val << endl;
#endif
        vm.state.num0 = Number(val);
    }
        break;
    case Instr::FLOAT: {
        string str = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "FLOAT \"" << str << "\"" << endl;
#endif
        double dd = strtod(str.c_str(), NULL);
        vm.state.num0 = Number(dd);
    }
        break;
    case Instr::NSWAP: {
#if DEBUG_INSTR > 0
        cout << "NSWAP" << endl;
#endif
        swap(vm.state.num0, vm.state.num1);
    }
        break;
    case Instr::CALL: {
        long args = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "CALL " << args << " (" << Symbols::get()[vm.state.sym] << ")" << endl;
#if DEBUG_INSTR > 2
        cout << "* Method Properties " << vm.state.ptr << endl;
#endif
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&vm.state.ptr->prim());
        ObjectPtr closure = (*vm.state.ptr)[ Symbols::get()["closure"] ];
#if DEBUG_INSTR > 2
        cout << "* Method Properties " <<
            (closure != nullptr) << " " <<
            (stmt ? stmt->index().index : -1) << " " <<
            (stmt ? stmt->translationUnit() : nullptr) << endl;
#endif
        if ((closure != nullptr) && stmt) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!vm.state.dyn.empty())
                vm.state.dyn.push( clone(vm.state.dyn.top()) );
            else
                vm.state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            auto lex = vm.state.lex.top();
            vm.state.lex.push( clone(closure) );
            // (4) Bind all the local variables
            vm.state.lex.top()->put(Symbols::get()["self"], vm.state.slf);
            vm.state.lex.top()->put(Symbols::get()["again"], vm.state.ptr);
            vm.state.lex.top()->put(Symbols::get()["caller"], lex);
            vm.state.lex.top()->protectAll(Protection::PROTECT_ASSIGN | Protection::PROTECT_DELETE,
                                        Symbols::get()["self"], Symbols::get()["again"]);
            // (5) Push the trace information
            pushTrace(vm.state);
            // (6) Bind all of the arguments
            if (!vm.state.dyn.empty()) {
                int index = args;
                for (long n = 0; n < args; n++) {
                    ObjectPtr arg = vm.state.arg.top();
                    vm.state.arg.pop();
                    vm.state.dyn.top()->put(Symbols::get()[ "$" + to_string(index) ], arg);
                    index--;
                }
            }
            // (7) Push %cont onto %stack
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.trns.push(stmt->translationUnit());
            // (8) Make a new %cont
            if (stmt) {
#if DEBUG_INSTR > 3
                cout << "* (cont) " << stmt->size() << endl;
                if (stmt->size() == 0) {
                    // What are we looking at...?
                    cout << "* * Where ptr has" << endl;
                    for (auto& x : keys(vm.state.ptr))
                        cout << "  " << Symbols::get()[x];
                    cout << endl;
                    cout << "* * Directly" << endl;
                    for (auto& x : (vm.state.ptr)->directKeys())
                        cout << "  " << Symbols::get()[x];
                    cout << endl;
                    cout << "* * Parents of ptr" << endl;
                    for (auto& x : hierarchy(vm.state.ptr))
                        cout << "  " << x;
                    cout << endl;
                    cout << "* * Following the prims of ptr" << endl;
                    for (auto& x : hierarchy(vm.state.ptr))
                        cout << "  " << x->prim().which();
                    cout << endl;
                }
#endif
                vm.state.cont = MethodSeek(*stmt);
            }
        } else {
            // It's not a method; just return it
            for (long n = 0; n < args; n++) {
                vm.state.arg.pop(); // For consistency, we must pop and discard these anyway
            }
            vm.state.ret = vm.state.ptr;
        }
    }
        break;
    case Instr::XCALL: {
#if DEBUG_INSTR > 0
        cout << "XCALL (" << Symbols::get()[vm.state.sym] << ")" << endl;
#endif
        auto stmt = boost::get<Method>(&vm.state.ptr->prim());
        if (stmt) {
            // (6) Push %cont onto %stack
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.trns.push(stmt->translationUnit());
            // (7) Make a new %cont
            if (stmt)
                vm.state.cont = MethodSeek(*stmt);
        }
    }
        break;
    case Instr::XCALL0: {
        long args = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "XCALL0 " << args << " (" << Symbols::get()[vm.state.sym] << ")" << endl;
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&vm.state.ptr->prim());
        ObjectPtr closure = (*vm.state.ptr)[ Symbols::get()["closure"] ];
        if ((closure != nullptr) && stmt) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!vm.state.dyn.empty())
                vm.state.dyn.push( clone(vm.state.dyn.top()) );
            else
                vm.state.err0 = true;
            // (3) Push a clone of the closure onto %lex
            auto lex = vm.state.lex.top();
            vm.state.lex.push( clone(closure) );
            // (4) Bind all the local variables
            vm.state.lex.top()->put(Symbols::get()["self"], vm.state.slf);
            vm.state.lex.top()->put(Symbols::get()["again"], vm.state.ptr);
            vm.state.lex.top()->put(Symbols::get()["caller"], lex);
            vm.state.lex.top()->protectAll(Protection::PROTECT_ASSIGN | Protection::PROTECT_DELETE,
                                        Symbols::get()["self"], Symbols::get()["again"]);
            // (5) Push the trace information
            pushTrace(vm.state);
            // (6) Bind all of the arguments
            if (!vm.state.dyn.empty()) {
                int index = args;
                for (long n = 0; n < args; n++) {
                    ObjectPtr arg = vm.state.arg.top();
                    vm.state.arg.pop();
                    vm.state.dyn.top()->put(Symbols::get()[ "$" + to_string(index) ], arg);
                    index--;
                }
            }
        } else {
            // It's not a method; just return it
            for (long n = 0; n < args; n++) {
                vm.state.arg.pop(); // For consistency, we must pop and discard these anyway
            }
            vm.state.ret = vm.state.ptr;
        }
    }
        break;
    case Instr::RET: {
#if DEBUG_INSTR > 0
        cout << "RET" << endl;
#endif
        if (vm.state.lex.empty())
            vm.state.err0 = true;
        else
            vm.state.lex.pop();
        if (vm.state.dyn.empty())
            vm.state.err0 = true;
        else
            vm.state.dyn.pop();
        if (!vm.state.trace)
            vm.state.err0 = true;
        else
            popTrace(vm.state);
        if (vm.state.trns.empty())
            vm.state.err0 = true;
        else
            vm.state.trns.pop();
    }
        break;
    case Instr::CLONE: {
#if DEBUG_INSTR > 0
        cout << "CLONE" << endl;
#endif
        vm.state.ret = clone(vm.state.slf);
    }
        break;
    case Instr::RTRV: {
#if DEBUG_INSTR > 0
        cout << "RTRV (" << Symbols::get()[vm.state.sym] << ")" << endl;
#endif
        // Try to find the value itsel
        ObjectPtr value = objectGet(vm.state.slf, vm.state.sym);
        if (value == nullptr) {
#if DEBUG_INSTR > 2
            cout << "* Looking for missing" << endl;
#if DEBUG_INSTR > 3
            cout << "* Information:" << endl;
            cout << "* * Lex: " << vm.state.lex.top() << endl;
            cout << "* * Dyn: " << vm.state.dyn.top() << endl;
            cout << "* * Slf: " << vm.state.slf << endl;
#endif
#endif
            // Now try for missing
            value = objectGet(vm.state.slf, Symbols::get()["missing"]);
#if DEBUG_INSTR > 1
            if (value == nullptr)
                cout << "* Found no missing" << endl;
            else
                cout << "* Found missing" << endl;
#endif
            if (value == nullptr) {
                ObjectPtr meta = nullptr;
                // If there is no `missing` either, fall back to the last resort
                if (!vm.state.lex.empty()) {
                    value = objectGet(vm.state.lex.top(), Symbols::get()["meta"]);
                    meta = value;
                }
                if (value != nullptr)
                    value = objectGet(value, Symbols::get()["missed"]);
#if DEBUG_INSTR > 1
                if (value == nullptr)
                    cout << "* Found no missed" << endl;
                else
                    cout << "* Found missed" << endl;
#endif
                if (value == nullptr) {
                    // Abandon ship!
                    vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
                    vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_TERMINATE }));
                } else {
                    vm.state.slf = meta;
                    vm.state.ptr = value;
                    vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
                    vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_CALL_ZERO }));
                }
            } else {
                //vm.state.sym = backup;
                vm.state.ret = value;
                //vm.state.slf = vm.state.slf;
                vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
                vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_MISSING }));
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
            vm.state.ret = value;
        }
    }
        break;
    case Instr::RTRVD: {
#if DEBUG_INSTR > 0
        cout << "RTRVD (" << Symbols::get()[vm.state.sym] << ")" << endl;
#endif
        ObjectPtr slot = (*vm.state.slf)[vm.state.sym];
        if (slot != nullptr)
            vm.state.ret = slot;
        else
            vm.state.err0 = true;
    }
        break;
    case Instr::STR: {
        string str = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "STR \"" << str << "\"" << endl;
#endif
        vm.state.str0 = str;
    }
        break;
    case Instr::SSWAP: {
#if DEBUG_INSTR > 0
        cout << "SSWAP" << endl;
#endif
        swap(vm.state.str0, vm.state.str1);
    }
        break;
    case Instr::EXPD: {
        Reg expd = vm.state.cont.readReg(0);
#if DEBUG_INSTR > 0
        cout << "EXPD " << (long)expd << endl;
#endif
        switch (expd) {
        case Reg::SYM: {
            auto test = boost::get<Symbolic>(&vm.state.ptr->prim());
            if (test)
                vm.state.sym = *test;
            else
                vm.state.err0 = true;
        }
            break;
        case Reg::NUM0: {
            auto test = boost::get<Number>(&vm.state.ptr->prim());
            if (test)
                vm.state.num0 = *test;
            else
                vm.state.err0 = true;
        }
            break;
        case Reg::NUM1: {
            auto test = boost::get<Number>(&vm.state.ptr->prim());
            if (test)
                vm.state.num1 = *test;
            else
                vm.state.err0 = true;
        }
            break;
        case Reg::STR0: {
            auto test = boost::get<string>(&vm.state.ptr->prim());
            if (test)
                vm.state.str0 = *test;
            else
                vm.state.err0 = true;
        }
            break;
        case Reg::STR1: {
            auto test = boost::get<string>(&vm.state.ptr->prim());
            if (test)
                vm.state.str1 = *test;
            else
                vm.state.err0 = true;
        }
            break;
        case Reg::MTHD: {
            auto test = boost::get<Method>(&vm.state.ptr->prim());
            if (test)
                vm.state.mthd = *test;
            else
                vm.state.err0 = true;
        }
            break;
        case Reg::STRM: {
            auto test = boost::get<StreamPtr>(&vm.state.ptr->prim());
            if (test)
                vm.state.strm = *test;
            else
                vm.state.err0 = true;
        }
            break;
        case Reg::PRCS: {
            auto test = boost::get<ProcessPtr>(&vm.state.ptr->prim());
            if (test)
                vm.state.prcs = *test;
            else
                vm.state.err0 = true;
        }
            break;
        case Reg::MTHDZ: {
            auto test = boost::get<Method>(&vm.state.ptr->prim());
            if (test)
                vm.state.mthdz = *test;
            else
                vm.state.err0 = true;
        }
            break;
        default:
            vm.state.err0 = true;
            break;
        }
    }
        break;
    case Instr::MTHD: {
#if DEBUG_INSTR > 0
        cout << "MTHD ..." << endl;
#endif
        FunctionIndex index = vm.state.cont.readFunction(0);
        vm.state.mthd = Method(vm.state.trns.top(), index);
    }
        break;
    case Instr::LOAD: {
        Reg ld = vm.state.cont.readReg(0);
#if DEBUG_INSTR > 0
        cout << "LOAD " << (long)ld << endl;
#endif
        switch (ld) {
        case Reg::SYM: {
            vm.state.ptr->prim(vm.state.sym);
        }
            break;
        case Reg::NUM0: {
            vm.state.ptr->prim(vm.state.num0);
        }
            break;
        case Reg::NUM1: {
            vm.state.ptr->prim(vm.state.num1);
        }
            break;
        case Reg::STR0: {
            vm.state.ptr->prim(vm.state.str0);
        }
            break;
        case Reg::STR1: {
            vm.state.ptr->prim(vm.state.str1);
        }
            break;
        case Reg::MTHD: {
#if DEBUG_INSTR > 1
            cout << "* Method Length " << vm.state.mthd.instructions().size() << endl;
#endif
            vm.state.ptr->prim(vm.state.mthd);
        }
            break;
        case Reg::STRM: {
            vm.state.ptr->prim(vm.state.strm);
        }
            break;
        case Reg::PRCS: {
            vm.state.ptr->prim(vm.state.prcs);
        }
            break;
        case Reg::MTHDZ: {
#if DEBUG_INSTR > 1
            cout << "* Method Length " << vm.state.mthdz.instructions().size() << endl;
#endif
            vm.state.ptr->prim(vm.state.mthdz);
        }
            break;
        default:
            vm.state.err0 = true;
            break;
        }
    }
        break;
    case Instr::SETF: {
#if DEBUG_INSTR > 0
        cout << "SETF (" << Symbols::get()[vm.state.sym] << ")" << endl;
#if DEBUG_INSTR > 2
        cout << "* Information:" << endl;
        cout << "* * Lex: " << vm.state.lex.top() << endl;
        cout << "* * Dyn: " << vm.state.dyn.top() << endl;
        cout << "* * Slf: " << vm.state.slf << endl;
#endif
#endif
        if (vm.state.slf == nullptr)
            vm.state.err0 = true;
        else if (vm.state.slf->isProtected(vm.state.sym, Protection::PROTECT_ASSIGN))
            throwError(vm, "ProtectedError");
        else
            vm.state.slf->put(vm.state.sym, vm.state.ptr);
    }
        break;
    case Instr::PEEK: {
        stack<ObjectPtr>* stack;
        Reg dest = vm.state.cont.readReg(0);
        Reg reg = vm.state.cont.readReg(1);
        ObjectPtr mid = nullptr;
#if DEBUG_INSTR > 0
        cout << "PEEK " << (long)reg << endl;
#endif
        switch (reg) {
        case Reg::LEX:
            stack = &vm.state.lex;
            break;
        case Reg::DYN:
            stack = &vm.state.dyn;
            break;
        case Reg::ARG:
            stack = &vm.state.arg;
            break;
        case Reg::STO:
            stack = &vm.state.sto;
            break;
        case Reg::HAND:
            stack = &vm.state.hand;
            break;
        default:
            stack = nullptr;
            vm.state.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                mid = stack->top();
            } else {
                vm.state.err0 = true;
            }
            switch (dest) {
            case Reg::PTR:
                vm.state.ptr = mid;
                break;
            case Reg::SLF:
                vm.state.slf = mid;
                break;
            case Reg::RET:
                vm.state.ret = mid;
                break;
            default:
                vm.state.err0 = true;
                break;
            }
        }
    }
        break;
    case Instr::SYMN: {
        long val = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "SYMN " << val << " (" << Symbols::get()[Symbolic{val}] << ")" << endl;
#endif
        vm.state.sym = { val };
    }
        break;
    case Instr::CPP: {
        long val = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "CPP " << val << endl;
#endif
        auto func = vm.reader.cpp.at(val);
        if (func)
            func(vm);
        else
            vm.state.err0 = true;
    }
        break;
    case Instr::BOL: {
#if DEBUG_INSTR > 0
        cout << "BOL (" << vm.state.flag << ")" << endl;
#endif
        vm.state.ret = garnishObject(vm.reader, vm.state.flag);
    }
        break;
    case Instr::TEST: {
#if DEBUG_INSTR > 0
        cout << "TEST" << endl;
#endif
        if (vm.state.slf == nullptr || vm.state.ptr == nullptr)
            vm.state.flag = false;
        else
            vm.state.flag = (vm.state.slf == vm.state.ptr);
    }
        break;
    case Instr::BRANCH: {
#if DEBUG_INSTR > 0
        cout << "BRANCH (" << vm.state.flag << ")" << endl;
#endif
        vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
        if (vm.state.flag) {
#if DEBUG_INSTR > 2
            cout << "* Method ( ) Length " << vm.state.mthd.instructions().size() << endl;
#endif
            vm.state.cont = MethodSeek(vm.state.mthd);
        } else {
#if DEBUG_INSTR > 2
            cout << "* Method (Z) Length " << vm.state.mthdz.instructions().size() << endl;
#endif
            vm.state.cont = MethodSeek(vm.state.mthdz);
        }
    }
        break;
    case Instr::CCALL: {
#if DEBUG_INSTR > 0
        cout << "CCALL" << endl;
#endif
        if (vm.state.slf == nullptr) {
            vm.state.err0 = true;
        } else {
            vm.state.slf->prim( statePtr(vm.state) );
            vm.state.arg.push(vm.state.slf);
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_CALL_ONE }));
        }
    }
        break;
    case Instr::CGOTO: {
#if DEBUG_INSTR > 0
        cout << "CGOTO" << endl;
#endif
        auto cont = boost::get<StatePtr>( vm.state.ptr->prim() );
        if (cont) {
            auto oldWind = vm.state.wind;
            auto newWind = cont->wind;
            vm.state = *cont;
            resolveThunks(vm, oldWind, newWind);
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
        auto cont = boost::get<StatePtr>( vm.state.ptr->prim() );
        auto ret = vm.state.ret;
        if (cont) {
            auto oldWind = vm.state.wind;
            auto newWind = cont->wind;
            vm.state = *cont;
            vm.state.ret = ret;
            resolveThunks(vm, oldWind, newWind);
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
        auto beforeMthd = boost::get<Method>(&vm.state.slf->prim()),
             afterMthd  = boost::get<Method>(&vm.state.ptr->prim());
        if (beforeMthd && afterMthd) {
            Thunk before {
                *beforeMthd,
                (*vm.state.slf)[ Symbols::get()["closure"] ],
                vm.state.dyn.top()
            };
            Thunk after {
                *afterMthd,
                (*vm.state.ptr)[ Symbols::get()["closure"] ],
                vm.state.dyn.top()
            };
            WindPtr frame = WindPtr(new WindFrame(before, after));
            vm.state.wind = pushNode(vm.state.wind, frame);
        } else {
#if DEBUG_INSTR > 0
        cout << "* Not methods" << endl;
#endif
            vm.state.err0 = true;
        }
    }
        break;
    case Instr::UNWND: {
#if DEBUG_INSTR > 0
        cout << "UNWND" << endl;
#endif
        vm.state.wind = popNode(vm.state.wind);
    }
        break;
    case Instr::THROW: {
#if DEBUG_INSTR > 0
        cout << "THROW" << endl;
#endif
        ObjectPtr exc = vm.state.slf;
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
        recurse(vm.state.hand);
#if DEBUG_INSTR > 1
        cout << "* Got handlers: " << handlers.size() << endl;
#endif
        vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
        vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_PANIC }));
        for (ObjectPtr handler : handlers) {
            vm.state.arg.push(exc);
            vm.state.sto.push(handler);
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_HANDLER }));
        }
    }
        break;
    case Instr::THROQ: {
#if DEBUG_INSTR > 0
        cout << "THROQ (" << vm.state.err0 << ")" << endl;
#endif
        if (vm.state.err0) {
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_THROW }));
        }
    }
        break;
    case Instr::ADDS: {
#if DEBUG_INSTR > 0
        cout << "ADDS" << endl;
#endif
        vm.state.str0 += vm.state.str1;
    }
        break;
    case Instr::ARITH: {
        long val = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "ARITH " << val << endl;
#endif
        switch (val) {
        case 1L:
            vm.state.num0 += vm.state.num1;
            break;
        case 2L:
            vm.state.num0 -= vm.state.num1;
            break;
        case 3L:
            vm.state.num0 *= vm.state.num1;
            break;
        case 4L:
            vm.state.num0 /= vm.state.num1;
            break;
        case 5L:
            vm.state.num0 %= vm.state.num1;
            break;
        case 6L:
            vm.state.num0 = vm.state.num0.pow(vm.state.num1);
            break;
        case 7L:
            vm.state.num0 &= vm.state.num1;
            break;
        case 8L:
            vm.state.num0 |= vm.state.num1;
            break;
        case 9L:
            vm.state.num0 ^= vm.state.num1;
            break;
        default:
            vm.state.err0 = true;
            break;
        }
    }
        break;
    case Instr::THROA: {
        string msg = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "THROA \"" << msg << "\"" << endl;
#endif
        if (vm.state.err0)
            throwError(vm, "TypeError", msg);
    }
        break;
    case Instr::LOCFN: {
        string msg = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "LOCFN \"" << msg << "\"" << endl;
#endif
        vm.state.file = msg;
    }
        break;
    case Instr::LOCLN: {
        long num = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "LOCLN " << num << endl;
#endif
        vm.state.line = num;
    }
        break;
    case Instr::LOCRT: {
#if DEBUG_INSTR > 0
        cout << "LOCRT" << endl;
#endif
        auto stck = vm.state.trace;
        if (stck) {
            ObjectPtr sframe = vm.reader.lit.at(Lit::SFRAME);
            ObjectPtr frame = nullptr;
            ObjectPtr top = nullptr;
            while (stck) {
                ObjectPtr temp;
                long line;
                string file;
                auto elem = stck->get();
                tie(line, file) = elem;
                temp = clone(sframe);
                if (file != "") {
                    if (frame == nullptr) {
                        top = temp;
                    } else {
                        frame->put(Symbols::parent(), temp);
                    }
                    frame = temp;
                    frame->put(Symbols::get()["line"], garnishObject(vm.reader, line));
                    frame->put(Symbols::get()["file"], garnishObject(vm.reader, file));
                }
                stck = popNode(stck);
            }
            assert(top != nullptr); // Should always be non-null since the loop must run once
            vm.state.ret = top;
        } else {
            // The %trace stack was empty; this should not happen...
            // Honestly, this should probably be an assert failure,
            // but %trace is so weird right now that I'm hesitant to
            // rely on it.
            vm.state.ret = vm.reader.lit.at(Lit::NIL);
            // TODO This *should* be an assertion failure, once %trace is reliable
        }
    }
        break;
    case Instr::NRET: {
#if DEBUG_INSTR > 0
        cout << "NRET" << endl;
#endif
        vm.state.trace = pushNode(vm.state.trace, make_tuple(0L, string("")));
        vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
        vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_RETURN }));
    }
        break;
    case Instr::UNTR: {
#if DEBUG_INSTR > 0
        cout << "UNTR" << endl;
#endif
        if (vm.state.trns.empty())
            vm.state.err0 = true;
        else
            vm.state.trns.pop();
    }
        break;
    case Instr::CMPLX: {
        string str0 = vm.state.cont.readString(0);
        string str1 = vm.state.cont.readString(1);
#if DEBUG_INSTR > 0
        cout << "CMPLX" << endl;
#endif
        double rl = strtod(str0.c_str(), NULL);
        double im = strtod(str1.c_str(), NULL);
        vm.state.num0 = Number(Number::complex(rl, im));
    }
        break;
    case Instr::YLD: {
        long val = vm.state.cont.readLong(0);
        Reg reg = vm.state.cont.readReg(1);
#if DEBUG_INSTR > 0
        cout << "YLD " << val << " " << (long)reg << endl;
#endif
        auto obj = vm.reader.lit.at(val);
        if (obj != nullptr) {
            switch (reg) {
            case Reg::PTR:
                vm.state.ptr = obj;
                break;
            case Reg::SLF:
                vm.state.slf = obj;
                break;
            case Reg::RET:
                vm.state.ret = obj;
                break;
            default:
                vm.state.err0 = true;
                break;
            }
        } else {
            vm.state.err0 = true;
        }
    }
        break;
    case Instr::YLDC: {
        long val = vm.state.cont.readLong(0);
        Reg reg = vm.state.cont.readReg(1);
#if DEBUG_INSTR > 0
        cout << "YLDC " << val << " " << (long)reg << endl;
#endif
        auto obj = vm.reader.lit.at(val);
        if (obj != nullptr) {
            obj = clone(obj);
            switch (reg) {
            case Reg::PTR:
                vm.state.ptr = obj;
                break;
            case Reg::SLF:
                vm.state.slf = obj;
                break;
            case Reg::RET:
                vm.state.ret = obj;
                break;
            default:
                vm.state.err0 = true;
                break;
            }
        } else {
            vm.state.err0 = true;
        }
    }
        break;
    case Instr::DEL: {
#if DEBUG_INSTR > 0
        cout << "DEL" << endl;
#endif
        if (vm.state.slf == nullptr) {
            vm.state.err0 = true;
        } else if (vm.state.slf->isProtected(vm.state.sym, Protection::PROTECT_DELETE)) {
            throwError(vm, "ProtectedError", "Delete-protected variable");
        } else {
            vm.state.slf->remove(vm.state.sym);
        }
    }
        break;
    case Instr::ARR: {
        long val = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "ARR " << val << endl;
#endif
        ObjectPtr arr = clone(vm.reader.lit.at(Lit::ARRAY));
        int j = 2 * (int)val - 1;
        for (long i = 0; i < val; i++, j -= 2) {
            arr->put(Symbols::natural(j), vm.state.arg.top());
            vm.state.arg.pop();
        }
        arr->put(Symbols::get()["lowerBound"], garnishObject(vm.reader, 0));
        arr->put(Symbols::get()["upperBound"], garnishObject(vm.reader, val));
        vm.state.ret = arr;
    }
        break;
    case Instr::DICT: {
        long val = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "DICT " << val << endl;
#endif
        ObjectPtr dict = vm.reader.lit.at(Lit::DICT);
        ObjectPtr impl0 = (*dict)[Symbols::get()["&impl"]];
        ObjectPtr impl = clone(impl0);
        (*impl) = (*impl0);
        dict = clone(dict);
        dict->put(Symbols::get()["&impl"], impl);
        for (long i = 0; i < val; i++) {
            ObjectPtr value = vm.state.arg.top();
            vm.state.arg.pop();
            ObjectPtr key = vm.state.arg.top();
            vm.state.arg.pop();
            auto key0 = boost::get<Symbolic>(&key->prim());
            if (key0) {
                impl->put(*key0, value);
            } else {
                throwError(vm, "TypeError", "Symbol expected");
                return;
            }
        }
        vm.state.ret = dict;
    }
        break;
    case Instr::XXX: {
        long val = vm.state.cont.readLong(0);
        val = val; // Ignore unused variable warning
#if DEBUG_INSTR > 0
        cout << "XXX " << val << endl;
#endif
        static std::stack< NodePtr<BacktraceFrame> > trace_marker {};
        switch (val) {
        case 0:
            // Store
            trace_marker.push(vm.state.trace);
            break;
        case 1: {
            // Compare
            auto temp = vm.state.trace;
            auto curr = trace_marker.top();
            while ((curr != nullptr) && (temp != nullptr)) {
                if (curr->get() != temp->get()) {
                    // Welp
                    cerr << "Trace assertion failure!" << endl;
                    hardKill(vm);
                }
                curr = popNode(curr);
                temp = popNode(temp);
            }
            if (curr != temp) {
                // Welp
                cerr << "Trace assertion failure!" << endl;
                hardKill(vm);
            }
            trace_marker.pop();
        }
            break;
        }
    }
        break;
    case Instr::GOTO: {
#if DEBUG_INSTR > 0
        cout << "GOTO" << endl;
#endif
        vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
        vm.state.cont = vm.state.mthd;
    }
        break;
    case Instr::MSWAP: {
#if DEBUG_INSTR > 0
        cout << "MSWAP" << endl;
#endif
        swap(vm.state.mthd, vm.state.mthdz);
    }
        break;
    }
}

void doOneStep(VMState& vm) {
    vm.state.cont.advancePosition(1);
    if (vm.state.cont.atEnd()) {
        // Pop off the stack
#if DEBUG_INSTR > 1
        cout << "<><><>" << endl;
#endif
        if (vm.state.stack) {
#ifdef PROFILE_INSTR
            Profiling::get().etcBegin();
#endif
            vm.state.cont = vm.state.stack->get();
            vm.state.stack = popNode(vm.state.stack);
#ifdef PROFILE_INSTR
            Profiling::get().etcEnd();
#endif
            doOneStep(vm);
        }
    } else {
        // Run one command
        Instr instr = vm.state.cont.readInstr();
#if DEBUG_INSTR > 1
        cout << "<" << (long)instr << ">" << endl;
#endif
#ifdef PROFILE_INSTR
        Profiling::get().instructionBegin(instr);
#endif
        executeInstr(instr, vm);
#ifdef PROFILE_INSTR
        Profiling::get().instructionEnd(instr);
#endif
        GC::get().tick(vm);
    }
}

bool isIdling(IntState& state) {
    return (state.cont.atEnd() && !state.stack);
}
