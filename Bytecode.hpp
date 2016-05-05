#ifndef _BYTECODE_HPP_
#define _BYTECODE_HPP_

#include <map>
#include <vector>
#include <string>
#include <boost/variant.hpp>

using InstrSeq = std::vector<unsigned char>;

enum class Instr : unsigned char {
    MOV = 0x01, PUSH = 0x02, POP = 0x03, GETL = 0x04, GETD = 0x05, ESWAP = 0x06, ECLR = 0x07,
        ESET = 0x08, SYM = 0x09, NUM = 0x0A, INT = 0x0B, FLOAT = 0x0C, NSWAP = 0x0D, CALL = 0x0E,
        XCALL = 0x0F, XCALL0 = 0x10, RET = 0x11, CLONE = 0x12
        };

enum class Reg : unsigned char {
    PTR = 0x01, SLF = 0x02, RET = 0x03, LEX = 0x04, DYN = 0x05, ARG = 0x06, STO = 0x07,
        CONT = 0x08, STACK = 0x09, ERR0 = 0x0A, ERR1 = 0x0B, SYM = 0x0C, NUM0 = 0x0D,
        NUM1 = 0x0E
        };

using RegisterArg = boost::variant<Reg, long, std::string>;

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

class AssemblerError {
private:
    std::string message;
public:
    AssemblerError();
    AssemblerError(std::string message);
    std::string getMessage();
};

bool isObjectRegister(const RegisterArg& arg);
bool isStackRegister(const RegisterArg& arg);
bool isStringRegisterArg(const RegisterArg& arg);
bool isLongRegisterArg(const RegisterArg& arg);
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
    void appendOnto(InstrSeq&);
};

template <typename... Ts>
AssemblerLine makeAssemblerLine(Instr instr, Ts... args) {
    AssemblerLine line;
    line.setCommand(instr);
    std::array<RegisterArg&, sizeof...(args)> args0 = { args... };
    for (auto& arg : args0)
        line.addRegisterArg(arg);
    return line;
}

#endif // _BYTECODE_HPP_
