#ifndef _BYTECODE_HPP_
#define _BYTECODE_HPP_

#include <map>
#include <vector>
#include <string>
#include <stack>
#include <type_traits>
#include "Symbol.hpp"
#include "Number.hpp"
#include "Proto.hpp"
#include "Instructions.hpp"
#include "Stack.hpp"

struct Thunk;
struct WindFrame;
struct IntState;

using CppFunction = std::function<void(IntState&)>;
using StatePtr = std::shared_ptr<IntState>;
using WindPtr = std::shared_ptr<WindFrame>;
using BacktraceFrame = std::tuple<long, std::string>;

struct IntState {
    ObjectPtr ptr, slf, ret;
    std::stack<ObjectPtr> lex, dyn, arg, sto;
    SeekHolder cont;
    NodePtr<SeekHolder> stack;
    bool err0, err1;
    Symbolic sym;
    Number num0, num1;
    std::string str0, str1;
    Method mthd;
    std::map<long, CppFunction> cpp;
    StreamPtr strm;
    ProcessPtr prcs;
    Method mthdz;
    bool flag;
    NodePtr<WindPtr> wind;
    std::stack<ObjectPtr> hand;
    long line;
    std::string file;
    NodePtr<BacktraceFrame> trace;
    std::stack<TranslationUnitPtr> trns;
    std::map<long, ObjectPtr> lit;
};

struct Thunk {
    Method code;
    ObjectPtr lex;
    ObjectPtr dyn;
};

struct WindFrame {
    Thunk before;
    Thunk after;
};

IntState intState();
StatePtr statePtr(const IntState& state);
StatePtr statePtr(IntState&& state);

// hardKill() leaves the interpreter state in a valid but unspecified state which is guaranteed
// to return true for isIdling(state)
void hardKill(IntState&);

void resolveThunks(IntState& state, NodePtr<WindPtr> oldWind, NodePtr<WindPtr> newWind);

unsigned char popChar(InstrSeq& state);
long popLong(InstrSeq& state);
std::string popString(InstrSeq& state);
Reg popReg(InstrSeq& state);
Instr popInstr(InstrSeq& state);
FunctionIndex popFunction(InstrSeq& state);

void executeInstr(Instr instr, IntState& state);

void doOneStep(IntState& state);

bool isIdling(IntState& state);

#endif // _BYTECODE_HPP_
