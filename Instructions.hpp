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
        LOCFN = 0x2B, LOCLN = 0x2C, LOCRT = 0x2D, NRET = 0x2E
        };

enum class Reg : unsigned char {
    PTR = 0x01, SLF = 0x02, RET = 0x03, LEX = 0x04, DYN = 0x05, ARG = 0x06, STO = 0x07,
        CONT = 0x08, STACK = 0x09, ERR0 = 0x0A, ERR1 = 0x0B, SYM = 0x0C, NUM0 = 0x0D,
        NUM1 = 0x0E, STR0 = 0x0F, STR1 = 0x10, MTHD = 0x11, CPP = 0x12, STRM = 0x13,
        PRCS = 0x14, MTHDZ = 0x15, FLAG = 0x16, WIND = 0x17, HAND = 0x18, LINE = 0x19,
        FILE = 0x1A, TRACE = 0x1B
        };

using InstrSeq = std::deque<unsigned char>;
using RegisterArg = boost::variant<Reg, long, std::string, InstrSeq>;

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

#endif
