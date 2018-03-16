#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP
#include <deque>
#include <vector>
#include <map>
#include <boost/variant.hpp>

/// \file
///
/// \brief Data types for managing VM instructions.

class AssemblerLine;

/// The instruction enumeration, containing all of the opcodes for
/// every instruction supported by the Latitude VM. Any value not
/// listed here is not a valid opcode.
enum class Instr : unsigned char {
    MOV = 0x01, PUSH = 0x02, POP = 0x03, GETL = 0x04, GETD = 0x05, ESWAP = 0x06, ECLR = 0x07,
    ESET = 0x08, SYM = 0x09, NUM = 0x0A, INT = 0x0B, FLOAT = 0x0C, NSWAP = 0x0D, CALL = 0x0E,
    XCALL = 0x0F, XCALL0 = 0x10, RET = 0x11, CLONE = 0x12, RTRV = 0x13, RTRVD = 0x14, STR = 0x15,
    SSWAP = 0x16, EXPD = 0x17, MTHD = 0x18, LOAD = 0x19, SETF = 0x1A, PEEK = 0x1B, SYMN = 0x1C,
    CPP = 0x1D, BOL = 0x1E, TEST = 0x1F, BRANCH = 0x20, CCALL = 0x21, CGOTO = 0x22, CRET = 0x23,
    WND = 0x24, UNWND = 0x25, THROW = 0x26, THROQ = 0x27, ADDS = 0x28, ARITH = 0x29, THROA = 0x2A,
    LOCFN = 0x2B, LOCLN = 0x2C, LOCRT = 0x2D, NRET = 0x2E, UNTR = 0x2F, CMPLX = 0x30, YLD = 0x31,
    YLDC = 0x32, DEL = 0x33
};

/// The register enumeration, containing numerical values for
/// referencing every register in an instance of the Latitude VM.
enum class Reg : unsigned char {
    PTR = 0x01, SLF = 0x02, RET = 0x03, LEX = 0x04, DYN = 0x05, ARG = 0x06, STO = 0x07,
    CONT = 0x08, STACK = 0x09, ERR0 = 0x0A, ERR1 = 0x0B, SYM = 0x0C, NUM0 = 0x0D,
    NUM1 = 0x0E, STR0 = 0x0F, STR1 = 0x10, MTHD = 0x11, CPP = 0x12, STRM = 0x13,
    PRCS = 0x14, MTHDZ = 0x15, FLAG = 0x16, WIND = 0x17, HAND = 0x18, LINE = 0x19,
    FILE = 0x1A, TRACE = 0x1B, TRNS = 0x1C, LIT = 0x1D, GTU = 0x1E
};

/// \brief A namespace containing `%%lit` table references.
///
/// This namespace contains constants for referencing the positions in
/// the read-only `%%lit` register, which contains a table of special
/// objects.
namespace Lit {

    /// \brief The index of the Nil object.
    constexpr long NIL    =  0L;

    /// \brief The index of the False object.
    constexpr long FALSE  =  1L;

    /// \brief The index of the True object.
    constexpr long TRUE   =  2L;

    /// \brief The index of the Boolean object.
    constexpr long BOOL   =  3L;

    /// \brief The index of the String object.
    constexpr long STRING =  4L;

    /// \brief The index of the Number object.
    constexpr long NUMBER =  5L;

    /// \brief The index of the Symbol object.
    constexpr long SYMBOL =  6L;

    /// \brief The index of the StackFrame object.
    constexpr long SFRAME =  7L;

    /// \brief The index of the Method object.
    constexpr long METHOD =  8L;

    /// \brief The index of the FileHeader object.
    constexpr long FHEAD  =  9L;

    /// \brief The index of the err object.
    constexpr long ERR    = 10L;

}

/// A function index is fundamentally just an integral value. This
/// type is provided to avoid accidentally treating function indices
/// as ordinary integers and vice versa.
struct FunctionIndex {
    int index;
};

struct Instruction;

/// A serial instruction sequence is a sequence of characters.
using SerialInstrSeq = std::deque<unsigned char>;

/// A (non-serial) instruction sequence is a sequence of instructions.
using InstrSeq = std::deque<Instruction>;

/// \brief An argument to an instruction.
///
/// Arguments to instructions can be references to registers, integer
/// values, string values, or indices of functions in the VM.
using RegisterArg = boost::variant<Reg, long, std::string, FunctionIndex>;

/// A single instruction consists of the instruction's opcode,
/// followed by zero or more arguments.
struct Instruction {
    Instr instr;
    std::vector<RegisterArg> args;

    Instruction(Instr, const std::vector<RegisterArg>);

};

/// The singleton InstructionSet instance manages the collection of
/// valid instruction opcodes. It is used by makeRuntimeAssemblerLine
/// to validate the arguments to an instruction and can be used to
/// query the expected argument types.
class InstructionSet {
public:
    /// \brief A validator for determining argument correctness.
    ///
    /// A validator contains criteria for determining whether or not
    /// an argument is correct. It is a 1-ary predicate which takes a
    /// RegisterArg.
    typedef std::function<bool(const RegisterArg&)> Validator;
    /// A ValidList value represents a sequence of Validator values.
    typedef std::vector<Validator> ValidList;
private:
    bool initialized;
    std::map<Instr, ValidList> props;
    static InstructionSet iset;
    void initialize();
    InstructionSet() = default;
public:

    /// Returns the singleton instruction set.
    ///
    /// \return the singleton value
    static InstructionSet& getInstance();

    /// Returns whether or not the instruction set considers the given
    /// opcode to be valid.
    ///
    /// \return whether the opcode is valid or not
    bool hasInstruction(Instr instr);

    /// Returns a sequence of Validator predicates describing all of
    /// the arguments to a given opcode.
    ///
    /// \return the appropriate ValidList value
    ValidList getParams(Instr instr);

};

/// This function is an InstructionSet::Validator representing
/// arguments which are register references.
///
/// \param arg the instruction argument
/// \return whether the argument is a register reference
bool isRegister(const RegisterArg& arg);

/// This function is an InstructionSet::Validator representing
/// arguments which are references to object registers.
///
/// \param arg the instruction arguments
/// \return whether the argument is an object register reference
bool isObjectRegister(const RegisterArg& arg);

/// This function is an InstructionSet::Validator representing
/// arguments which are references to stack registers.
///
/// \param arg the instruction arguments
/// \return whether the argument is an stack register reference
bool isStackRegister(const RegisterArg& arg);

/// This function is an InstructionSet::Validator representing string
/// arguments.
///
/// \param arg the instruction arguments
/// \return whether the argument is a string
bool isStringRegisterArg(const RegisterArg& arg);

/// This function is an InstructionSet::Validator representing integer
/// arguments.
///
/// \param arg the instruction arguments
/// \return whether the argument is an integer
bool isLongRegisterArg(const RegisterArg& arg);

/// This function is an InstructionSet::Validator representing
/// function arguments.
///
/// \param arg the instruction arguments
/// \return whether the argument is a function
bool isAsmRegisterArg(const RegisterArg& arg);

/// This is the general error class for exceptions in the VM system,
/// usually with the integrity of a VM instruction.
class AssemblerError {
private:
    std::string message;
public:
    /// \brief Constructs an assembler error with a generic message.
    AssemblerError();
    /// \brief Constructs an assembler error with a specific message.
    AssemblerError(std::string message);
    /// \brief Returns the error's message.
    ///
    /// \return the message
    std::string getMessage();
};

/// Appends an argument to a serialized instruction sequence, as a
/// sequence of characters.
///
/// \param arg the instruction argument
/// \param seq the serialized instruction sequence
void appendRegisterArg(const RegisterArg& arg, SerialInstrSeq& seq);

/// Appends an opcode to a serialized instruction sequence, as a
/// sequence of characters.
///
/// \param instr the instruction
/// \param seq the serialized instruction sequence
void appendInstruction(const Instr& instr, SerialInstrSeq& seq);

// TODO When we're done, it's possible that AssemblerLine could end up
// serving the purpose of the Instruction struct as well.

/// An AssemblerLine is a single instruction as it is being built
/// up. Assembler lines consist of a single opcode and zero or more
/// arguments.
class AssemblerLine {
private:
    Instr command;
    std::vector<RegisterArg> args;
public:

    /// \brief Constructs an empty assembler line.
    AssemblerLine() = default;

    /// \brief Sets the opcode for the assembler line.
    ///
    /// \param str the opcode
    void setCommand(Instr str);

    /// \brief Adds an argument to the instruction.
    ///
    /// \param arg the argument
    void addRegisterArg(const RegisterArg& arg);

    /// \brief Verifies the integrity of the instruction and its arguments.
    ///
    /// If the instruction is valid and correct, no operations
    /// occur. Otherwise, an exception is thrown.
    ///
    /// \throw AssemblerError in the case of an integrity error
    void validate();

    /// \brief Appends the assembler line onto an instruction sequence.
    ///
    /// \param seq the sequence of instructions
    void appendOnto(InstrSeq& seq) const;

    /// \brief Appends the assembler line onto a serialized
    /// instruction sequence.
    ///
    /// \param seq the serialized instruction sequence
    void appendOntoSerial(SerialInstrSeq& seq) const;
};

/// For efficiency reasons, it is undesirable to pass around methods
/// as sequences of instructions. Thus, all methods in Latitude code
/// are stored in localized translation units, where they can be
/// referenced by a pointer and an index. A translation unit consists
/// of a global sequence of instructions (usually the "top-level" code
/// in a file or module) and an ordered sequence of instruction
/// sequences, which are often (but not necessarily) referenced as
/// methods.
class TranslationUnit {
private:
    std::vector<InstrSeq> methods;
public:

    /// Constructs a default translation unit consisting of no code
    /// and no methods.
    TranslationUnit();

    /// Constructs a translation unit consisting of the given
    /// top-level code and no methods.
    TranslationUnit(const InstrSeq& seq);

    /// Returns the translation unit's top-level instruction sequnce.
    ///
    /// \return the top-level sequence of instructions
    InstrSeq& instructions();

    /// Returns the nth method in the translation unit. <em>No bounds
    /// checking is performed.</em> It is the caller's responsibility
    /// to ensure that the argument to this method is between 0 and
    /// methodCount().
    ///
    /// \param index the index of the method
    /// \return the method itself
    InstrSeq& method(int index);

    /// Returns the number of methods in the translation unit.
    ///
    /// \return the method count
    int methodCount();

    /// Adds a method to the translation unit. The argument will be
    /// copied into the translation unit.
    ///
    /// \param mthd the method to add
    /// \return the index of the method
    FunctionIndex pushMethod(const InstrSeq& mthd);

    /// Adds a method to the translation unit, using move semantics.
    ///
    /// \param mthd the method to add
    /// \return the index of the method
    FunctionIndex pushMethod(InstrSeq&& mthd);

};

/// A smart pointer to a translation unit.
using TranslationUnitPtr = std::shared_ptr<TranslationUnit>;

/// A method is referenced by a Method object, which consists of a
/// pointer to a translation unit and an index within that unit.
class Method {
private:
    TranslationUnitPtr unit;
    FunctionIndex ind;
public:

    /// Default-constructs a method. This constructor should be used
    /// sparingly, as it creates a "null" method object which cannot
    /// be called.
    Method();

    /// Constructs a callable method object.
    ///
    /// \param ptr the translation unit in which the method code resides
    /// \param index the position in the translation unit of the code
    Method(const TranslationUnitPtr& ptr, FunctionIndex index);

    /// Retrieves the sequence of instructions belonging to this method.
    ///
    /// \return the method's instruction sequence
    InstrSeq& instructions();

    /// Returns the size of the method instruction sequence.
    ///
    /// \return the size of the instruction sequence
    size_t size();

    /// Retrieves a pointer to the translation unit to which this
    /// method belongs.
    ///
    /// \return the translation unit to which the method belongs
    TranslationUnitPtr translationUnit();

    /// Returns the index within translationUnit() where this method
    /// can be found.
    ///
    /// \return the function index within the translation unit
    FunctionIndex index();

};

/// For efficiency reasons, it is undesirable to store methods
/// directly. Therefore, a MethodSeek instance stores a sequence of
/// instructions, not directly, but by reference to a translation
/// unit.
class MethodSeek {
private:
    unsigned long pos;
    unsigned long _size;
    Method method;
public:

    /// Constructs a nullary MethodSeek object. It is not safe to
    /// access the fields of a nullary MethodSeek object.
    MethodSeek();

    /// Constructs a MethodSeek which points to the specified method.
    ///
    /// \param m a method
    MethodSeek(Method m);

    /// Returns the position of the MethodSeek object within the
    /// InstrSeq data.
    ///
    /// \return the position
    unsigned long position();

    /// Advances the MethodSeek's position by an integer amount, which
    /// must be positive.
    ///
    /// \param val the number by which to advance the position
    void advancePosition(unsigned long val);

    /// Returns the size of the instructions() sequence.
    ///
    /// \return the size
    unsigned long size();

    /// Returns whether the sequence is at its end. Specifically, this
    /// checks whether or not size() is equal to position().
    ///
    /// \return whether or not the sequence is at its end
    bool atEnd();

    /// Reads a single long argument from the instruction
    /// sequence. The behavior is undefined if the given argment is
    /// not a long integer.
    ///
    /// \param n the position of the argument within the current instruction
    /// \return the long argument
    long readLong(int n);

    /// Reads a single string argument from the instruction
    /// sequence. The behavior is undefined if the given argment is
    /// not a string.
    ///
    /// \param n the position of the argument within the current instruction
    /// \return the string argument
    std::string readString(int n);

    /// Reads a single register argument from the instruction
    /// sequence. The behavior is undefined if the given argment is
    /// not a register.
    ///
    /// \param n the position of the argument within the current instruction
    /// \return the register argument
    Reg readReg(int n);

    /// Retrieves the opcode of the current instruction in the
    /// sequence.
    ///
    /// \return the instruction opcode
    Instr readInstr();

    /// Reads a single function argument from the instruction
    /// sequence. The behavior is undefined if the given argment is
    /// not a function index.
    ///
    /// \param n the position of the argument within the current instruction
    /// \return the function argument
    FunctionIndex readFunction(int n);

    InstrSeq& instructions();

};

#endif // INSTRUCTIONS_HPP
