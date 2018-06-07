
#include "Serialize.hpp"
#include <string>

struct AppendVisitor {
    SerialInstrSeq* instructions;

    void operator()(const Reg& reg) {
        instructions->push_back((unsigned char)reg);
    }

    void operator()(const std::string& str) {
        for (char ch : str) {
            if (ch == 0) {
                instructions->push_back('\0');
                instructions->push_back('.');
            } else {
                instructions->push_back(ch);
            }
        }
        instructions->push_back('\0');
        instructions->push_back('\0');
    }

    void operator()(const long& val) {
        long val1 = val;
        if (val1 < 0)
            instructions->push_back(0xFF);
        else
            instructions->push_back(0x00);
        val1 = abs(val1);
        for (int i = 0; i < 4; i++) {
            instructions->push_back((unsigned char)(val1 % 256));
            val1 /= 256;
        }
    }

    void operator()(const FunctionIndex& ind) {
        // No need for a sign bit; this is an index so it's always nonnegative
        int val1 = ind.index;
        for (int i = 0; i < 4; i++) {
            instructions->push_back((unsigned char)(val1 % 256));
            val1 /= 256;
        }
    }

};

void appendRegisterArg(const RegisterArg& arg, SerialInstrSeq& seq) {
    AppendVisitor visitor { &seq };
    boost::apply_visitor(visitor, arg);
}

void appendInstruction(const Instr& instr, SerialInstrSeq& seq) {
    seq.push_back((unsigned char)instr);
}

unsigned char popChar(SerialInstrSeq& state) {
    if (state.empty())
        return 0;
    char val = state.front();
    state.pop_front();
    return val;
}

long popLong(SerialInstrSeq& state) {
    int sign = 1;
    if (popChar(state) > 0)
        sign *= -1;
    long value = 0;
    long pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)popChar(state);
        pow <<= 8;
    }
    return sign * value;
}

std::string popString(SerialInstrSeq& state) {
    std::string str;
    unsigned char ch;
    while (true) {
        ch = popChar(state);
        if (ch == '\0') {
            ch = popChar(state);
            if (ch == '.')
                str += '\0';
            else if (ch == '\0')
                break;
        } else {
            str += ch;
        }
    }
    return str;
}

Reg popReg(SerialInstrSeq& state) {
    unsigned char ch = popChar(state);
    return (Reg)ch;
}

Instr popInstr(SerialInstrSeq& state) {
    unsigned char ch = popChar(state);
    return (Instr)ch;
}

FunctionIndex popFunction(SerialInstrSeq& state) {
    int value = 0;
    int pow = 1;
    for (int i = 0; i < 4; i++) {
        value += pow * (long)popChar(state);
        pow <<= 8;
    }
    return { value };
}
