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

class StackNode;

using NodePtr = std::shared_ptr<StackNode>;

// A home-baked stack implementation which allows sharing of data between continuations for
// efficiency
class StackNode {
private:
    NodePtr next;
    SeekHolder data;
public:
    StackNode(const SeekHolder& data0);
    const SeekHolder& get();
    friend NodePtr pushNode(NodePtr node, const SeekHolder& data);
    friend NodePtr popNode(NodePtr node);
};

NodePtr pushNode(NodePtr node, const SeekHolder& data);
NodePtr popNode(NodePtr node);

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
    NodePtr stack;
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
    std::stack<WindPtr> wind;
    std::stack<ObjectPtr> hand;
    long line;
    std::string file;
    std::stack<BacktraceFrame> trace;
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

// hardKill() leaves the interpreter state in a valid but unspecified state which is guaranteed
// to return true for isIdling(state)
void hardKill(IntState&);

void resolveThunks(IntState& state, std::stack<WindPtr> oldWind, std::stack<WindPtr> newWind);

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
