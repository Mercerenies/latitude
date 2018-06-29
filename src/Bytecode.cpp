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

VMState VMState::createAndInit(ObjectPtr* global, int argc, char** argv) {
    IntState state;
    ReadOnlyState reader;

    state.cont = MethodSeek(Method(reader.gtu, { Table::GTU_EMPTY }));

    *global = spawnObjects(state, reader, argc, argv);

    return { state, reader };

}

IntState::IntState()
    : // %cont?
      line(0) {}

TransientState::TransientState()
    : sym(Symbols::get()[""]) {}

ReadOnlyState::ReadOnlyState()
    : gtu(std::make_shared<TranslationUnit>()) {}

StatePtr statePtr(const IntState& state) {
    return make_shared<IntState>(state);
}

StatePtr statePtr(IntState&& state) {
    return make_shared<IntState>(forward<IntState&&>(state));
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
            mid = vm.trans.ptr;
            break;
        case Reg::SLF:
            mid = vm.trans.slf;
            break;
        case Reg::RET:
            mid = vm.trans.ret;
            break;
        default:
            mid = nullptr;
            vm.trans.err0 = true;
            break;
        }
        switch (dest) {
        case Reg::PTR:
            vm.trans.ptr = mid;
            break;
        case Reg::SLF:
            vm.trans.slf = mid;
            break;
        case Reg::RET:
            vm.trans.ret = mid;
            break;
        default:
            vm.trans.err0 = true;
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
            mid = vm.trans.ptr;
            break;
        case Reg::SLF:
            mid = vm.trans.slf;
            break;
        case Reg::RET:
            mid = vm.trans.ret;
            break;
        default:
            mid = nullptr;
            vm.trans.err0 = true;
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
            vm.trans.err0 = true;
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
            vm.trans.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                mid = stack->top();
                stack->pop();
            } else {
                vm.trans.err0 = true;
            }
            switch (dest) {
            case Reg::PTR:
                vm.trans.ptr = mid;
                break;
            case Reg::SLF:
                vm.trans.slf = mid;
                break;
            case Reg::RET:
                vm.trans.ret = mid;
                break;
            default:
                vm.trans.err0 = true;
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
            vm.trans.err0 = true;
        } else {
            switch (dest) {
            case Reg::PTR:
                vm.trans.ptr = vm.state.lex.top();
                break;
            case Reg::SLF:
                vm.trans.slf = vm.state.lex.top();
                break;
            case Reg::RET:
                vm.trans.ret = vm.state.lex.top();
                break;
            default:
                vm.trans.err0 = true;
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
            vm.trans.err0 = true;
        } else {
            switch (dest) {
            case Reg::PTR:
                vm.trans.ptr = vm.state.dyn.top();
                break;
            case Reg::SLF:
                vm.trans.slf = vm.state.dyn.top();
                break;
            case Reg::RET:
                vm.trans.ret = vm.state.dyn.top();
                break;
            default:
                vm.trans.err0 = true;
                break;
            }
        }
    }
        break;
    case Instr::ESWAP: {
#if DEBUG_INSTR > 0
        cout << "ESWAP" << endl;
#endif
        swap(vm.trans.err0, vm.trans.err1);
    }
        break;
    case Instr::ECLR: {
#if DEBUG_INSTR > 0
        cout << "ECLR" << endl;
#endif
        vm.trans.err0 = false;
    }
        break;
    case Instr::ESET: {
#if DEBUG_INSTR > 0
        cout << "ESET" << endl;
#endif
        vm.trans.err0 = true;
    }
        break;
    case Instr::SYM: {
        string str = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "SYM \"" << str << "\"" << endl;
#endif
        vm.trans.sym = Symbols::get()[str];
    }
        break;
    case Instr::NUM: {
        string str = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "NUM \"" << str << "\"" << endl;
#endif
        auto temp = parseInteger(str.c_str());
        assert(temp);
        vm.trans.num0 = *temp;
    }
        break;
    case Instr::INT: {
        long val = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "INT " << val << endl;
#endif
        vm.trans.num0 = Number(val);
    }
        break;
    case Instr::FLOAT: {
        string str = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "FLOAT \"" << str << "\"" << endl;
#endif
        double dd = strtod(str.c_str(), NULL);
        vm.trans.num0 = Number(dd);
    }
        break;
    case Instr::NSWAP: {
#if DEBUG_INSTR > 0
        cout << "NSWAP" << endl;
#endif
        swap(vm.trans.num0, vm.trans.num1);
    }
        break;
    case Instr::CALL: {
        long args = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "CALL " << args << " (" << Symbols::get()[vm.trans.sym] << ")" << endl;
#if DEBUG_INSTR > 2
        cout << "* Method Properties " << vm.trans.ptr << endl;
#endif
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&vm.trans.ptr->prim());
        ObjectPtr closure = (*vm.trans.ptr)[ Symbols::get()["closure"] ];
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
                vm.trans.err0 = true;
            // (3) Push a clone of the closure onto %lex
            auto lex = vm.state.lex.top();
            vm.state.lex.push( clone(closure) );
            // (4) Bind all the local variables
            vm.state.lex.top()->put(Symbols::get()["self"], vm.trans.slf);
            vm.state.lex.top()->put(Symbols::get()["again"], vm.trans.ptr);
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
                    for (auto& x : keys(vm.trans.ptr))
                        cout << "  " << Symbols::get()[x];
                    cout << endl;
                    cout << "* * Directly" << endl;
                    for (auto& x : (vm.trans.ptr)->directKeys())
                        cout << "  " << Symbols::get()[x];
                    cout << endl;
                    cout << "* * Parents of ptr" << endl;
                    for (auto& x : hierarchy(vm.trans.ptr))
                        cout << "  " << x;
                    cout << endl;
                    cout << "* * Following the prims of ptr" << endl;
                    for (auto& x : hierarchy(vm.trans.ptr))
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
            vm.trans.ret = vm.trans.ptr;
        }
    }
        break;
    case Instr::XCALL: {
#if DEBUG_INSTR > 0
        cout << "XCALL (" << Symbols::get()[vm.trans.sym] << ")" << endl;
#endif
        auto stmt = boost::get<Method>(&vm.trans.ptr->prim());
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
        cout << "XCALL0 " << args << " (" << Symbols::get()[vm.trans.sym] << ")" << endl;
#endif
        // (1) Perform a hard check for `closure`
        auto stmt = boost::get<Method>(&vm.trans.ptr->prim());
        ObjectPtr closure = (*vm.trans.ptr)[ Symbols::get()["closure"] ];
        if ((closure != nullptr) && stmt) {
            // It's a method; get ready to call it
            // (2) Try to clone the top of %dyn
            if (!vm.state.dyn.empty())
                vm.state.dyn.push( clone(vm.state.dyn.top()) );
            else
                vm.trans.err0 = true;
            // (3) Push a clone of the closure onto %lex
            auto lex = vm.state.lex.top();
            vm.state.lex.push( clone(closure) );
            // (4) Bind all the local variables
            vm.state.lex.top()->put(Symbols::get()["self"], vm.trans.slf);
            vm.state.lex.top()->put(Symbols::get()["again"], vm.trans.ptr);
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
            vm.trans.ret = vm.trans.ptr;
        }
    }
        break;
    case Instr::RET: {
#if DEBUG_INSTR > 0
        cout << "RET" << endl;
#endif
        if (vm.state.lex.empty())
            vm.trans.err0 = true;
        else
            vm.state.lex.pop();
        if (vm.state.dyn.empty())
            vm.trans.err0 = true;
        else
            vm.state.dyn.pop();
        if (!vm.state.trace)
            vm.trans.err0 = true;
        else
            popTrace(vm.state);
        if (vm.state.trns.empty())
            vm.trans.err0 = true;
        else
            vm.state.trns.pop();
    }
        break;
    case Instr::CLONE: {
#if DEBUG_INSTR > 0
        cout << "CLONE" << endl;
#endif
        vm.trans.ret = clone(vm.trans.slf);
    }
        break;
    case Instr::RTRV: {
#if DEBUG_INSTR > 0
        cout << "RTRV (" << Symbols::get()[vm.trans.sym] << ")" << endl;
#endif
        // Try to find the value itsel
        ObjectPtr value = objectGet(vm.trans.slf, vm.trans.sym);
        if (value == nullptr) {
#if DEBUG_INSTR > 2
            cout << "* Looking for missing" << endl;
#if DEBUG_INSTR > 3
            cout << "* Information:" << endl;
            cout << "* * Lex: " << vm.state.lex.top() << endl;
            cout << "* * Dyn: " << vm.state.dyn.top() << endl;
            cout << "* * Slf: " << vm.trans.slf << endl;
#endif
#endif
            // Now try for missing
            value = objectGet(vm.trans.slf, Symbols::get()["missing"]);
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
                    vm.trans.slf = meta;
                    vm.trans.ptr = value;
                    vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
                    vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_CALL_ZERO }));
                }
            } else {
                //vm.trans.sym = backup;
                vm.trans.ret = value;
                //vm.trans.slf = vm.trans.slf;
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
            vm.trans.ret = value;
        }
    }
        break;
    case Instr::RTRVD: {
#if DEBUG_INSTR > 0
        cout << "RTRVD (" << Symbols::get()[vm.trans.sym] << ")" << endl;
#endif
        ObjectPtr slot = (*vm.trans.slf)[vm.trans.sym];
        if (slot != nullptr)
            vm.trans.ret = slot;
        else
            vm.trans.err0 = true;
    }
        break;
    case Instr::STR: {
        string str = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "STR \"" << str << "\"" << endl;
#endif
        vm.trans.str0 = str;
    }
        break;
    case Instr::SSWAP: {
#if DEBUG_INSTR > 0
        cout << "SSWAP" << endl;
#endif
        swap(vm.trans.str0, vm.trans.str1);
    }
        break;
    case Instr::EXPD: {
        Reg expd = vm.state.cont.readReg(0);
#if DEBUG_INSTR > 0
        cout << "EXPD " << (long)expd << endl;
#endif
        switch (expd) {
        case Reg::SYM: {
            auto test = boost::get<Symbolic>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.sym = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        case Reg::NUM0: {
            auto test = boost::get<Number>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.num0 = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        case Reg::NUM1: {
            auto test = boost::get<Number>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.num1 = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        case Reg::STR0: {
            auto test = boost::get<string>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.str0 = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        case Reg::STR1: {
            auto test = boost::get<string>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.str1 = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        case Reg::MTHD: {
            auto test = boost::get<Method>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.mthd = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        case Reg::STRM: {
            auto test = boost::get<StreamPtr>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.strm = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        case Reg::PRCS: {
            auto test = boost::get<ProcessPtr>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.prcs = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        case Reg::MTHDZ: {
            auto test = boost::get<Method>(&vm.trans.ptr->prim());
            if (test)
                vm.trans.mthdz = *test;
            else
                vm.trans.err0 = true;
        }
            break;
        default:
            vm.trans.err0 = true;
            break;
        }
    }
        break;
    case Instr::MTHD: {
#if DEBUG_INSTR > 0
        cout << "MTHD ..." << endl;
#endif
        FunctionIndex index = vm.state.cont.readFunction(0);
        vm.trans.mthd = Method(vm.state.trns.top(), index);
    }
        break;
    case Instr::LOAD: {
        Reg ld = vm.state.cont.readReg(0);
#if DEBUG_INSTR > 0
        cout << "LOAD " << (long)ld << endl;
#endif
        switch (ld) {
        case Reg::SYM: {
            vm.trans.ptr->prim(vm.trans.sym);
        }
            break;
        case Reg::NUM0: {
            vm.trans.ptr->prim(vm.trans.num0);
        }
            break;
        case Reg::NUM1: {
            vm.trans.ptr->prim(vm.trans.num1);
        }
            break;
        case Reg::STR0: {
            vm.trans.ptr->prim(vm.trans.str0);
        }
            break;
        case Reg::STR1: {
            vm.trans.ptr->prim(vm.trans.str1);
        }
            break;
        case Reg::MTHD: {
#if DEBUG_INSTR > 1
            cout << "* Method Length " << vm.trans.mthd.instructions().size() << endl;
#endif
            vm.trans.ptr->prim(vm.trans.mthd);
        }
            break;
        case Reg::STRM: {
            vm.trans.ptr->prim(vm.trans.strm);
        }
            break;
        case Reg::PRCS: {
            vm.trans.ptr->prim(vm.trans.prcs);
        }
            break;
        case Reg::MTHDZ: {
#if DEBUG_INSTR > 1
            cout << "* Method Length " << vm.trans.mthdz.instructions().size() << endl;
#endif
            vm.trans.ptr->prim(vm.trans.mthdz);
        }
            break;
        default:
            vm.trans.err0 = true;
            break;
        }
    }
        break;
    case Instr::SETF: {
#if DEBUG_INSTR > 0
        cout << "SETF (" << Symbols::get()[vm.trans.sym] << ")" << endl;
#if DEBUG_INSTR > 2
        cout << "* Information:" << endl;
        cout << "* * Lex: " << vm.state.lex.top() << endl;
        cout << "* * Dyn: " << vm.state.dyn.top() << endl;
        cout << "* * Slf: " << vm.trans.slf << endl;
#endif
#endif
        if (vm.trans.slf == nullptr)
            vm.trans.err0 = true;
        else if (vm.trans.slf->isProtected(vm.trans.sym, Protection::PROTECT_ASSIGN))
            throwError(vm, "ProtectedError");
        else
            vm.trans.slf->put(vm.trans.sym, vm.trans.ptr);
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
            vm.trans.err0 = true;
        }
        if (stack != nullptr) {
            if (!stack->empty()) {
                mid = stack->top();
            } else {
                vm.trans.err0 = true;
            }
            switch (dest) {
            case Reg::PTR:
                vm.trans.ptr = mid;
                break;
            case Reg::SLF:
                vm.trans.slf = mid;
                break;
            case Reg::RET:
                vm.trans.ret = mid;
                break;
            default:
                vm.trans.err0 = true;
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
        vm.trans.sym = { val };
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
            vm.trans.err0 = true;
    }
        break;
    case Instr::BOL: {
#if DEBUG_INSTR > 0
        cout << "BOL (" << vm.trans.flag << ")" << endl;
#endif
        vm.trans.ret = garnishObject(vm.reader, vm.trans.flag);
    }
        break;
    case Instr::TEST: {
#if DEBUG_INSTR > 0
        cout << "TEST" << endl;
#endif
        if (vm.trans.slf == nullptr || vm.trans.ptr == nullptr)
            vm.trans.flag = false;
        else
            vm.trans.flag = (vm.trans.slf == vm.trans.ptr);
    }
        break;
    case Instr::BRANCH: {
#if DEBUG_INSTR > 0
        cout << "BRANCH (" << vm.state.flag << ")" << endl;
#endif
        vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
        if (vm.trans.flag) {
#if DEBUG_INSTR > 2
            cout << "* Method ( ) Length " << vm.trans.mthd.instructions().size() << endl;
#endif
            vm.state.cont = MethodSeek(vm.trans.mthd);
        } else {
#if DEBUG_INSTR > 2
            cout << "* Method (Z) Length " << vm.trans.mthdz.instructions().size() << endl;
#endif
            vm.state.cont = MethodSeek(vm.trans.mthdz);
        }
    }
        break;
    case Instr::CCALL: {
#if DEBUG_INSTR > 0
        cout << "CCALL" << endl;
#endif
        if (vm.trans.slf == nullptr) {
            vm.trans.err0 = true;
        } else {
            vm.trans.slf->prim( statePtr(vm.state) );
            vm.state.arg.push(vm.trans.slf);
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_CALL_ONE }));
        }
    }
        break;
    case Instr::CGOTO: {
#if DEBUG_INSTR > 0
        cout << "CGOTO" << endl;
#endif
        auto cont = boost::get<StatePtr>( vm.trans.ptr->prim() );
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
        auto cont = boost::get<StatePtr>( vm.trans.ptr->prim() );
        auto ret = vm.trans.ret;
        if (cont) {
            auto oldWind = vm.state.wind;
            auto newWind = cont->wind;
            vm.state = *cont;
            vm.trans.ret = ret;
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
        auto beforeMthd = boost::get<Method>(&vm.trans.slf->prim()),
             afterMthd  = boost::get<Method>(&vm.trans.ptr->prim());
        if (beforeMthd && afterMthd) {
            Thunk before {
                *beforeMthd,
                (*vm.trans.slf)[ Symbols::get()["closure"] ],
                vm.state.dyn.top()
            };
            Thunk after {
                *afterMthd,
                (*vm.trans.ptr)[ Symbols::get()["closure"] ],
                vm.state.dyn.top()
            };
            WindPtr frame = WindPtr(new WindFrame(before, after));
            vm.state.wind = pushNode(vm.state.wind, frame);
        } else {
#if DEBUG_INSTR > 0
        cout << "* Not methods" << endl;
#endif
            vm.trans.err0 = true;
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
        ObjectPtr exc = vm.trans.slf;
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
        cout << "THROQ (" << vm.trans.err0 << ")" << endl;
#endif
        if (vm.trans.err0) {
            vm.state.stack = pushNode(vm.state.stack, vm.state.cont);
            vm.state.cont = MethodSeek(Method(vm.reader.gtu, { Table::GTU_THROW }));
        }
    }
        break;
    case Instr::ADDS: {
#if DEBUG_INSTR > 0
        cout << "ADDS" << endl;
#endif
        vm.trans.str0 += vm.trans.str1;
    }
        break;
    case Instr::ARITH: {
        long val = vm.state.cont.readLong(0);
#if DEBUG_INSTR > 0
        cout << "ARITH " << val << endl;
#endif
        switch (val) {
        case 1L:
            vm.trans.num0 += vm.trans.num1;
            break;
        case 2L:
            vm.trans.num0 -= vm.trans.num1;
            break;
        case 3L:
            vm.trans.num0 *= vm.trans.num1;
            break;
        case 4L:
            vm.trans.num0 /= vm.trans.num1;
            break;
        case 5L:
            vm.trans.num0 %= vm.trans.num1;
            break;
        case 6L:
            vm.trans.num0 = vm.trans.num0.pow(vm.trans.num1);
            break;
        case 7L:
            vm.trans.num0 &= vm.trans.num1;
            break;
        case 8L:
            vm.trans.num0 |= vm.trans.num1;
            break;
        case 9L:
            vm.trans.num0 ^= vm.trans.num1;
            break;
        default:
            vm.trans.err0 = true;
            break;
        }
    }
        break;
    case Instr::THROA: {
        string msg = vm.state.cont.readString(0);
#if DEBUG_INSTR > 0
        cout << "THROA \"" << msg << "\"" << endl;
#endif
        if (vm.trans.err0)
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
            vm.trans.ret = top;
        } else {
            // The %trace stack was empty; this should not happen...
            // Honestly, this should probably be an assert failure,
            // but %trace is so weird right now that I'm hesitant to
            // rely on it.
            vm.trans.ret = vm.reader.lit.at(Lit::NIL);
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
            vm.trans.err0 = true;
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
        vm.trans.num0 = Number(Number::complex(rl, im));
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
                vm.trans.ptr = obj;
                break;
            case Reg::SLF:
                vm.trans.slf = obj;
                break;
            case Reg::RET:
                vm.trans.ret = obj;
                break;
            default:
                vm.trans.err0 = true;
                break;
            }
        } else {
            vm.trans.err0 = true;
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
                vm.trans.ptr = obj;
                break;
            case Reg::SLF:
                vm.trans.slf = obj;
                break;
            case Reg::RET:
                vm.trans.ret = obj;
                break;
            default:
                vm.trans.err0 = true;
                break;
            }
        } else {
            vm.trans.err0 = true;
        }
    }
        break;
    case Instr::DEL: {
#if DEBUG_INSTR > 0
        cout << "DEL" << endl;
#endif
        if (vm.trans.slf == nullptr) {
            vm.trans.err0 = true;
        } else if (vm.trans.slf->isProtected(vm.trans.sym, Protection::PROTECT_DELETE)) {
            throwError(vm, "ProtectedError", "Delete-protected variable");
        } else {
            vm.trans.slf->remove(vm.trans.sym);
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
        vm.trans.ret = arr;
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
        vm.trans.ret = dict;
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
        vm.state.cont = vm.trans.mthd;
    }
        break;
    case Instr::MSWAP: {
#if DEBUG_INSTR > 0
        cout << "MSWAP" << endl;
#endif
        swap(vm.trans.mthd, vm.trans.mthdz);
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
