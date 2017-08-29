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

struct IntState { // TODO Should we make all std::stack be NodePtr here? More heap allocations but cheap copies.
    ObjectPtr ptr, slf, ret;
    std::stack<ObjectPtr> lex, dyn, arg, sto;
    SeekHolder cont;
    NodePtr<SeekHolder> stack;
    bool err0, err1;
    Symbolic sym;
    Number num0, num1;
    std::string str0, str1;
    Method mthd;
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
};

struct ReadOnlyState {
    std::map<long, CppFunction> cpp;
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

ReadOnlyState readOnlyState();

// hardKill() leaves the interpreter state in a valid but unspecified state which is guaranteed
// to return true for isIdling(state)
void hardKill(IntState&);

void resolveThunks(IntState& state, NodePtr<WindPtr> oldWind, NodePtr<WindPtr> newWind);

unsigned char popChar(SerialInstrSeq& state);
long popLong(SerialInstrSeq& state);
std::string popString(SerialInstrSeq& state);
Reg popReg(SerialInstrSeq& state);
Instr popInstr(SerialInstrSeq& state);
FunctionIndex popFunction(SerialInstrSeq& state);

void executeInstr(Instr instr, IntState& state, const ReadOnlyState& reader);

void doOneStep(IntState& state, const ReadOnlyState& reader);

bool isIdling(IntState& state);

#endif // _BYTECODE_HPP_
