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

class AssemblerError {
private:
    std::string message;
public:
    AssemblerError();
    AssemblerError(std::string message);
    std::string getMessage();
};

void appendRegisterArg(const RegisterArg& arg, InstrSeq& seq);
void appendInstruction(const Instr& instr, InstrSeq& seq);

class AssemblerLine {
private:
    Instr command;
    std::vector<RegisterArg> args;
public:
    AssemblerLine() = default;
    void setCommand(Instr);
    void addRegisterArg(const RegisterArg&);
    void validate(); // Throws if invalid
    void appendOnto(InstrSeq&) const;
};

template <typename... Ts>
AssemblerLine makeAssemblerLine(Instr instr, Ts... args) {
    AssemblerLine line;
    line.setCommand(instr);
    std::array<RegisterArg, sizeof...(args)> args0 = { args... };
    for (auto& arg : args0)
        line.addRegisterArg(arg);
    line.validate();
    return line;
}

template <typename... Ts>
InstrSeq asmCode(Ts... args) {
    InstrSeq seq;
    std::array<AssemblerLine, sizeof...(args)> args0 = { args... };
    for (auto& arg : args0)
        arg.appendOnto(seq);
    return seq;
}

class StackNode;

using NodePtr = std::shared_ptr<StackNode>;

// A home-baked stack implementation which allows sharing of data between continuations for
// efficiency
class StackNode {
private:
    NodePtr next;
    InstrSeq data;
public:
    StackNode(const InstrSeq& data0);
    const InstrSeq& get();
    friend NodePtr pushNode(NodePtr node, const InstrSeq& data);
    friend NodePtr popNode(NodePtr node);
};

NodePtr pushNode(NodePtr node, const InstrSeq& data);
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
    InstrSeq cont;
    NodePtr stack;
    bool err0, err1;
    Symbolic sym;
    Number num0, num1;
    std::string str0, str1;
    InstrSeq mthd;
    std::map<long, CppFunction> cpp;
    StreamPtr strm;
    ProcessPtr prcs;
    InstrSeq mthdz;
    bool flag;
    std::stack<WindPtr> wind;
    std::stack<ObjectPtr> hand;
    long line;
    std::string file;
    std::stack<BacktraceFrame> trace;
};

struct Thunk {
    InstrSeq code;
    ObjectPtr lex;
    ObjectPtr dyn;
};

struct WindFrame {
    Thunk before;
    Thunk after;
};

IntState intState();
StatePtr statePtr(const IntState& state);

void resolveThunks(IntState& state, std::stack<WindPtr> oldWind, std::stack<WindPtr> newWind);

unsigned char popChar(InstrSeq& state);
long popLong(InstrSeq& state);
std::string popString(InstrSeq& state);
Reg popReg(InstrSeq& state);
Instr popInstr(InstrSeq& state);
InstrSeq popLine(InstrSeq& state);

void executeInstr(Instr instr, IntState& state);

void doOneStep(IntState& state);

bool isIdling(IntState& state);

#endif // _BYTECODE_HPP_
