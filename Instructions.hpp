#ifndef _INSTRUCTIONS_HPP_
#define _INSTRUCTIONS_HPP_
#include <deque>
#include <vector>
#include <map>
#include <boost/variant.hpp>

class AssemblerLine;

enum class Instr : unsigned char {
    MOV = 0x01, PUSH = 0x02, POP = 0x03, GETL = 0x04, GETD = 0x05, ESWAP = 0x06, ECLR = 0x07,
        ESET = 0x08, SYM = 0x09, NUM = 0x0A, INT = 0x0B, FLOAT = 0x0C, NSWAP = 0x0D, CALL = 0x0E,
        XCALL = 0x0F, XCALL0 = 0x10, RET = 0x11, CLONE = 0x12, RTRV = 0x13, RTRVD = 0x14, STR = 0x15,
        SSWAP = 0x16, EXPD = 0x17, MTHD = 0x18, LOAD = 0x19, SETF = 0x1A, PEEK = 0x1B, SYMN = 0x1C,
        CPP = 0x1D, BOL = 0x1E, TEST = 0x1F, BRANCH = 0x20, CCALL = 0x21, CGOTO = 0x22, CRET = 0x23,
        WND = 0x24, UNWND = 0x25, THROW = 0x26, THROQ = 0x27, ADDS = 0x28, ARITH = 0x29, THROA = 0x2A,
        LOCFN = 0x2B, LOCLN = 0x2C, LOCRT = 0x2D, NRET = 0x2E, UNTR = 0x2F, CMPLX = 0x30
        };

enum class Reg : unsigned char {
    PTR = 0x01, SLF = 0x02, RET = 0x03, LEX = 0x04, DYN = 0x05, ARG = 0x06, STO = 0x07,
        CONT = 0x08, STACK = 0x09, ERR0 = 0x0A, ERR1 = 0x0B, SYM = 0x0C, NUM0 = 0x0D,
        NUM1 = 0x0E, STR0 = 0x0F, STR1 = 0x10, MTHD = 0x11, CPP = 0x12, STRM = 0x13,
        PRCS = 0x14, MTHDZ = 0x15, FLAG = 0x16, WIND = 0x17, HAND = 0x18, LINE = 0x19,
        FILE = 0x1A, TRACE = 0x1B, TRNS = 0x1C
        };

struct FunctionIndex {
    int index;
};

using InstrSeq = std::deque<unsigned char>;
using RegisterArg = boost::variant<Reg, long, std::string, FunctionIndex>;

class InstructionSet {
public:
    typedef std::function<bool(const RegisterArg&)> Validator;
    typedef std::vector<Validator> ValidList;
private:
    bool initialized;
    std::map<Instr, ValidList> props;
    static InstructionSet iset;
    void initialize();
    InstructionSet() = default;
public:
    static InstructionSet& getInstance();
    bool hasInstruction(Instr instr);
    ValidList getParams(Instr instr);
};

bool isRegister(const RegisterArg& arg);
bool isObjectRegister(const RegisterArg& arg);
bool isStackRegister(const RegisterArg& arg);
bool isStringRegisterArg(const RegisterArg& arg);
bool isLongRegisterArg(const RegisterArg& arg);
bool isAsmRegisterArg(const RegisterArg& arg);

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

class TranslationUnit {
private:
    InstrSeq seq;
    std::vector<InstrSeq> methods;
public:
    TranslationUnit();
    TranslationUnit(const InstrSeq& seq);
    InstrSeq& instructions();
    InstrSeq& method(int);
    int methodCount();
    FunctionIndex pushMethod(const InstrSeq&);
    FunctionIndex pushMethod(InstrSeq&&);
};

using TranslationUnitPtr = std::shared_ptr<TranslationUnit>;

class Method {
private:
    TranslationUnitPtr unit;
    FunctionIndex ind;
public:
    Method();
    Method(const TranslationUnitPtr&, FunctionIndex);
    InstrSeq& instructions();
    size_t size();
    TranslationUnitPtr translationUnit();
    FunctionIndex index();
};

class InstrSeek {
private:
    unsigned long pos;
    bool _size_set;
    unsigned long _size;
public:
    InstrSeek();
    virtual ~InstrSeek() = default;
    virtual std::unique_ptr<InstrSeek> copy() = 0;
    virtual InstrSeq& instructions() = 0;
    unsigned long position();
    void advancePosition(unsigned long);
    unsigned long size();
    bool atEnd();
    unsigned char popChar();
    long popLong();
    std::string popString();
    Reg popReg();
    Instr popInstr();
    FunctionIndex popFunction();
};

class CodeSeek : public InstrSeek {
private:
    std::shared_ptr<InstrSeq> seq;
public:
    CodeSeek();
    CodeSeek(const InstrSeq&);
    CodeSeek(InstrSeq&&);
    virtual std::unique_ptr<InstrSeek> copy();
    virtual InstrSeq& instructions();
};

class MethodSeek : public InstrSeek {
private:
    Method method;
public:
    MethodSeek(Method);
    virtual std::unique_ptr<InstrSeek> copy();
    virtual InstrSeq& instructions();
};

class SeekHolder {
private:
    std::unique_ptr<InstrSeek> internal;
    // Treat this as a reference; it should not be freed
    // and belongs to the internal variable above.
    InstrSeq* instr;
public:
    SeekHolder();
    template <typename T>
    SeekHolder(const T&);
    SeekHolder(const SeekHolder&);
    template <typename T>
    SeekHolder& operator=(const T&);
    SeekHolder& operator=(const SeekHolder&);
    InstrSeq& instructions();
    unsigned long position();
    void advancePosition(unsigned long);
    unsigned char popChar();
    long popLong();
    std::string popString();
    Reg popReg();
    Instr popInstr();
    FunctionIndex popFunction();
    bool atEnd();
};

template <typename T>
SeekHolder::SeekHolder(const T& prev)
    : internal(std::unique_ptr<InstrSeek>(new T(prev)))
    , instr(&internal->instructions()) {}

template <typename T>
SeekHolder& SeekHolder::operator=(const T& other) {
    internal = std::unique_ptr<InstrSeek>(new T(other));
    instr = &internal->instructions();
    return *this;
}

#endif
