//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <map>
#include <vector>
#include <string>
#include <stack>
#include <chrono>
#include <type_traits>
#include "Symbol.hpp"
#include "Number.hpp"
#include "Proto.hpp"
#include "Instructions.hpp"
#include "Stack.hpp"

//#define PROFILE_INSTR

/// \file
///
/// \brief VM information and types necessary for running Latitude bytecode.

struct Thunk;
struct WindFrame;
struct VMState;
struct IntState;
struct ReadOnlyState;

#ifdef PROFILE_INSTR

class Profiling {
private:
    static Profiling instance;
    std::unordered_map<unsigned char, int> counts;
    std::unordered_map<unsigned char, std::chrono::duration<double>> times;
    unsigned char current;
    std::chrono::time_point<std::chrono::high_resolution_clock> tick;
    Profiling() = default;
    void _begin(unsigned char x);
    void _end(unsigned char x);
public:
    static Profiling& get() noexcept;
    void etcBegin();
    void etcEnd();
    void instructionBegin(Instr x);
    void instructionEnd(Instr x);
    void dumpData();
};

#endif // PROFILE_INSTR

/// \brief A CppFunction represents a C++ function that is callable
/// from within the Latitude VM.
using CppFunction = std::function<void(VMState&)>;

/// \brief A StatePtr is a smart pointer to an IntState instance.
using StatePtr = std::shared_ptr<IntState>;

/// \brief A WindPtr is a smart pointer to a WindFrame instance.
using WindPtr = std::shared_ptr<WindFrame>;

/// \brief A single frame of a backtrace.
using BacktraceFrame = std::tuple<long, std::string>;

/// The interpreter state consists of several mutable registers of
/// various types.
struct IntState {
    std::stack<ObjectPtr> lex, dyn, arg, sto;
    MethodSeek cont;
    NodePtr<MethodSeek> stack;
    NodePtr<WindPtr> wind;
    std::stack<ObjectPtr> hand;
    long line;
    std::string file;
    NodePtr<BacktraceFrame> trace;
    std::stack<TranslationUnitPtr> trns;

    IntState();

};

/// TransientState is a middle ground between IntState (always
/// preserved) and ReadOnlyState (constant after initialization).
/// TransientState can be changed freely by the VM but will be lost
/// when a continuation jump is performed.
struct TransientState {
    ObjectPtr ptr, slf, ret;
    bool err0, err1;
    Symbolic sym;
    Number num0, num1;
    std::string str0, str1;
    Method mthd;
    StreamPtr strm;
    ProcessPtr prcs;
    Method mthdz;
    bool flag;

    TransientState();

};

/// In addition to an IntState instance, the VM retains a
/// ReadOnlyState instance, which contains registers that, once
/// initialized, will never change.
struct ReadOnlyState {
    std::vector<CppFunction> cpp;
    std::vector<ObjectPtr> lit;
    TranslationUnitPtr gtu;

    ReadOnlyState();

};

/// The composite virtual machine register space. A VMState consists
/// of an IntState, a TransientState, and a ReadOnlyState. These
/// objects can be explicitly constructed but are usually constructed
/// implicitly via VMState::createAndInit().
struct VMState {
    IntState state;
    TransientState trans;
    const ReadOnlyState reader;

    /// Constructs a VMState.
    ///
    /// \param state the interpreter state
    /// \param reader the read-only state
    VMState(IntState& state, const ReadOnlyState& reader);

    /// Copy-constructs a VMState.
    ///
    /// Virtual machine states can be copied, but (for efficiency
    /// reasons) copying must be done explicitly.
    ///
    /// \param arg the VMState
    explicit VMState(const VMState& arg) = default;

    /// Move-constructs a VMState.
    ///
    /// Virtual machine states can be moved, but (for efficiency
    /// reasons) moving must be done explicitly.
    ///
    /// \param arg the VMState
    explicit VMState(VMState&& arg) = default;

    /// A static factory method which initializes the standard library
    /// and virtual machine state, constructing all of the necessary
    /// built-in Latitude objects. The constructed VMState is
    /// returned.
    ///
    /// \param[out] global the global scope object
    /// \param[in] argc the command line argument count
    /// \param[in] argv the command line arguments
    /// \return the constructed VMState
    static VMState createAndInit(ObjectPtr* global, int argc, char** argv);

};

/// A thunk contains a method and dynamic and lexical scoping
/// information in which to execute the method.
struct Thunk {
    const Method code;
    const ObjectPtr lex;
    const ObjectPtr dyn;

    /// Constructs a thunk.
    ///
    /// \param code the body of the thunk
    /// \param lex the lexical scope
    /// \param dyn the dynamic scope
    Thunk(Method code, ObjectPtr lex, ObjectPtr dyn);

};

/// A wind frame protects the call stack by executing certain code
/// when a scope is entered and when it is exited. WindFrame instances
/// will always be respected when the VM performs a continuation jump.
struct WindFrame {
    const Thunk before;
    const Thunk after;

    /// Constructs a wind frame.
    ///
    /// \param before the thunk to run before entering the scope
    /// \param after the thunk to run after exiting the scope
    WindFrame(const Thunk& before, const Thunk& after);

};

/// Copies the interpreter state and returns a smart pointer to the copy.
///
/// \return a smart pointer
StatePtr statePtr(const IntState& state);

/// Moves the interpreter state and returns a smart pointer to the new data.
///
/// \return a smart pointer
StatePtr statePtr(IntState&& state);

/// This function leaves the interpreter in a valid but unspecified
/// state which is guaranteed to be idling (according to #isIdling).
///
/// \param vm the virtual machine state
void hardKill(VMState& vm);

/// This function, which is called during continuation jumps, compares
/// two wind frame stacks. Once it finds the first element that the
/// two stacks share, it unwinds the call stack to that point,
/// executing all of the wind frame exit thunks. Then, it winds the
/// call stack down to the new continuation position, executing all of
/// the wind frame enter thunks.
///
/// \param vm the virtual machine state
/// \param oldWind the wind stack that the continuation is jumping \e from
/// \param newWind the wind stack that the continuation is jumping \e to
void resolveThunks(VMState& vm, NodePtr<WindPtr> oldWind, NodePtr<WindPtr> newWind);

/// Pushes the current stack trace data (%line and %file) onto the
/// %trace register stack.
///
/// \param state the interpreter state
void pushTrace(IntState& state);

/// Pushes the current stack trace data (from %trace) into the
/// %line and %file.
///
/// \param state the interpreter state
void popTrace(IntState& state);

/// Given an instruction and an interpreter state whose `%%cont`
/// register contains arguments for the instruction, this function
/// executes the instruction, modifying the interpreter's registers as
/// necessary.
///
/// \param instr the instruction
/// \param vm the virtual machine state
void executeInstr(Instr instr, VMState& vm);

/// This function performs one VM instruction from the front of the
/// `%%cont` register. If `%%cont` is empty, the function will pop
/// continuations off the `%%stack` register until a non-empty one is
/// acquired. If `%%cont` is empty and `%%stack` contains only empty
/// continuations, this function empties the stack and performs no
/// further operations.
///
/// \param vm the virtual machine state
void doOneStep(VMState& vm);

/// This function returns whether or not the interpreter has more work
/// to do. An interpreter is idling if there are no instructions in
/// its `%%cont` register and its `%%stack` register is empty. Note
/// that it is possible for #isIdling to return `false` when there are
/// no more instructions available, if `%%stack` contains some empty
/// continuations. In this case, calling #doOneStep will eliminate the
/// empty continuations, so that future #isIdling calls return `true`.
///
/// \param state the interpreter state
/// \return whether the interpreter is idling
bool isIdling(IntState& state);

#endif // BYTECODE_HPP
