#!/usr/bin/python3

# Note: Depends on Alakazam (pip install alakazam)

import sys
from contextlib import closing
from enum import Enum
from itertools import count

import alakazam as zz
from alakazam import _1, _2, _3, _4, _5

ArgType = Enum('ArgType', "LONG STRING REG ASM")

REGISTER_TABLE = {
    0x01: "%ptr",
    0x02: "%slf",
    0x03: "%ret",
    0x04: "%lex",
    0x05: "%dyn",
    0x06: "%arg",
    0x07: "%sto",
    0x08: "%cont",
    0x09: "%stack",
    0x0A: "%err0",
    0x0B: "%err1",
    0x0C: "%sym",
    0x0D: "%num0",
    0x0E: "%num1",
    0x0F: "%str0",
    0x10: "%str1",
    0x11: "%mthd",
    0x12: "%cpp",
    0x13: "%strm",
    0x14: "%prcs",
    0x15: "%mthdz",
    0x16: "%flag",
    0x17: "%wind",
    0x18: "%hand",
    0x19: "%line",
    0x1A: "%file",
    0x1B: "%trace",
    0x1C: "%trns",
    0x1D: "%lit",
    0x1E: "%gtu",
}

INSTRUCTION_TABLE = {
    0x01: ("mov", [ArgType.REG, ArgType.REG]),
    0x02: ("push", [ArgType.REG, ArgType.REG]),
    0x03: ("pop", [ArgType.REG, ArgType.REG]),
    0x04: ("getl", [ArgType.REG]),
    0x05: ("getd", [ArgType.REG]),
    0x06: ("eswap", []),
    0x07: ("eclr", []),
    0x08: ("eset", []),
    0x09: ("sym", [ArgType.STRING]),
    0x0A: ("num", [ArgType.STRING]),
    0x0B: ("int", [ArgType.LONG]),
    0x0C: ("float", [ArgType.STRING]),
    0x0D: ("nswap", []),
    0x0E: ("call", [ArgType.LONG]),
    0x0F: ("xcall", []),
    0x10: ("xcall0", [ArgType.LONG]),
    0x11: ("ret", []),
    0x12: ("clone", []),
    0x13: ("rtrv", []),
    0x14: ("rtrvd", []),
    0x15: ("str", [ArgType.STRING]),
    0x16: ("sswap", []),
    0x17: ("expd", [ArgType.REG]),
    0x18: ("mthd", [ArgType.ASM]),
    0x19: ("load", [ArgType.REG]),
    0x1A: ("setf", []),
    0x1B: ("peek", [ArgType.REG, ArgType.REG]),
    0x1C: ("symn", [ArgType.STRING]),
    0x1D: ("cpp", [ArgType.LONG]),
    0x1E: ("bol", []),
    0x1F: ("test", []),
    0x20: ("branch", []),
    0x21: ("ccall", []),
    0x22: ("cgoto", []),
    0x23: ("cret", []),
    0x24: ("wnd", []),
    0x25: ("unwnd", []),
    0x26: ("throw", []),
    0x27: ("throq", []),
    0x28: ("adds", []),
    0x29: ("arith", [ArgType.LONG]),
    0x2A: ("throa", [ArgType.STRING]),
    0x2B: ("locfn", [ArgType.STRING]),
    0x2C: ("locln", [ArgType.LONG]),
    0x2D: ("locrt", []),
    0x2E: ("nret", []),
    0x2F: ("untr", []),
    0x30: ("cmplx", [ArgType.STRING, ArgType.STRING]),
    0x31: ("yld", [ArgType.LONG, ArgType.REG]),
    0x32: ("yldc", [ArgType.LONG, ArgType.REG]),
    0x33: ("del", []),
    0x34: ("arr", [ArgType.LONG]),
    0x35: ("dict", [ArgType.LONG]),
    0x36: ("xxx", [ArgType.LONG]),
}

class Register:

    def __init__(self, n):
        self._index = n

    def __str__(self):
        return REGISTER_TABLE[self._index]

class OpCode:

    def __init__(self, n):
        self._index = n

    def __str__(self):
        return INSTRUCTION_TABLE[self._index][0]

    def parameters(self):
        return INSTRUCTION_TABLE[self._index][1]

class Method:

    def __init__(self, n):
        self._index = n

    def __str__(self):
        return "<<{}>>".format(self._index)

class Instruction:

    def __init__(self, op, args=None):
        self.op_code = op
        self.args = args or []

    def __str__(self):
        if self.args:
            return "{} {}".format(self.op_code, ', '.join(map(str, self.args)))
        else:
            return str(self.op_code)

class WrappedStr(str):

    def __str__(self):
        a = ""
        for x in self:
            if x == '"':
                a += '\\"'
            else:
                a += x
        return '"' + a + '"'

class Header:

    def __init__(self, version):
        self.version = version
        self.module_name = None
        self.package_name = None


class BytecodeFile:

    def __init__(self, filename):
        self._file = open(filename, 'rb')

    def close(self):
        self._file.close()

    def _read_number(self, length, signed=True):
        if signed:
            sign = -1 if self.read_byte() > 0 else 1
        else:
            sign = 1
        n = 0
        power = 1
        for i in range(length):
            n += self.read_byte() * power
            power <<= 8
        return sign * n

    def read_byte(self):
        value = self._file.read(1)
        if value:
            return value[0]
        else:
            raise EOFError()

    def read_long(self):
        return self._read_number(4)

    def read_string(self):
        s = bytearray()
        while True:
            x = self.read_byte()
            if x == 0:
                y = self.read_byte()
                if y == 0:
                    break
                elif y == ord('.'):
                    s.append(0)
            else:
                s.append(x)
        return s.decode(encoding='utf-8')

    def read_register(self):
        return Register(self.read_byte())

    def read_opcode(self):
        return OpCode(self.read_byte())

    def read_method(self):
        return Method(self._read_number(4, signed=False))

    def read(self, parm):
        return {
            ArgType.LONG: self.read_long,
            ArgType.STRING: lambda: WrappedStr(self.read_string()),
            ArgType.REG: self.read_register,
            ArgType.ASM: self.read_method,
        }[parm]()

    def read_instruction(self):
        op = self.read_opcode()
        args = []
        for parm in op.parameters():
            args.append(self.read(parm))
        return Instruction(op, args)

    def read_block(self):
        length = self._read_number(8, signed=False)
        block = []
        pos = self._file.tell()
        while self._file.tell() < pos + length:
            block.append(self.read_instruction())
        return block

    def read_checked(self):
        nature = self.read_byte()
        return {
            ord('#'): self.read_long,
            ord('$'): self.read_string,
        }[nature]()

def read_header(file):
    version = file.read_long()
    header = Header(version)

    curr = file.read_byte()
    while curr != ord('.'):
        value = file.read_checked()
        {
            ord('M'): zz.set(_1.module_name, _2),
            ord('P'): zz.set(_1.package_name, _2),
        }[curr](header, value)
        curr = file.read_byte()

    # Additional sentinel bit; ignore
    file.read_byte()

    return header

def print_header_info(filename, header):
    print("File:", sys.argv[1])
    print("Version:", header.version)
    if header.module_name:
        print("Module:", header.module_name)
    if header.package_name:
        print("Package:", header.package_name)

if len(sys.argv) <= 1:
    print("Usage: ./bytecode.py <filename>", file=sys.stderr)
    exit()

with closing(BytecodeFile(sys.argv[1])) as file:
    try:
        header = read_header(file)
        if header.version != 1000:
            print("Unknown file version!", file=stderr)
            exit(1)
        print_header_info(filename=sys.argv[1], header=header)
        for i in count(0):
            curr = file.read_block()
            print("<<{}>>".format(i))
            for instr in curr:
                print("  {}".format(instr))
    except EOFError:
        pass
