#ifndef STANDARD_HPP
#define STANDARD_HPP

#include <string>
#include "Proto.hpp"
#include "Bytecode.hpp"

namespace Table {

    static constexpr long
        CPP_TERMINATE = 0,
        CPP_KERNEL_LOAD = 1,
        CPP_STREAM_DIR = 2,
        CPP_STREAM_PUT = 3,
        CPP_TO_STRING = 4,
        CPP_GENSYM = 5,
        CPP_INSTANCE_OF = 6,
        CPP_STREAM_READ = 7,
        CPP_EVAL = 8,
        CPP_SYM_NAME = 9,
        CPP_SYM_NUM = 10,
        CPP_SYM_INTERN = 11,
        CPP_SIMPLE_CMP = 12,
        CPP_NUM_LEVEL = 13,
        CPP_ORIGIN = 14,
        CPP_PROCESS_TASK = 15,
        CPP_OBJECT_KEYS = 16,
        CPP_FILE_DOOPEN = 17,
        CPP_FILE_DOCLOSE = 18,
        CPP_FILE_EOF = 19,
        CPP_STRING_LENGTH = 20,
        CPP_STRING_SUB = 21,
        CPP_STRING_FIND = 22,
        CPP_GC_RUN = 23,
        CPP_FILE_HEADER = 24,
        CPP_STR_ORD = 25,
        CPP_STR_CHR = 26,
        CPP_GC_TOTAL = 27,
        CPP_TIME_SPAWN = 28,
        CPP_ENV_GET = 29,
        CPP_ENV_SET = 30,
        CPP_EXE_PATH = 31,
        CPP_PATH_OP = 32,
        CPP_FILE_EXISTS = 33,
        CPP_TRIG_OP = 34,
        CPP_MATH_FLOOR = 35,
        CPP_NUM_CONST = 36,
        CPP_PROT_VAR = 37,
        CPP_PROT_IS = 38,
        CPP_STR_NEXT = 39,
        CPP_COMPLEX = 40,
        CPP_PRIM_METHOD = 41,
        CPP_LOOP_DO = 42,
        CPP_UNI_ORD = 43,
        CPP_UNI_CHR = 44,
        CPP_OSINFO = 45,
        CPP_COMPLPARTS = 46,
        CPP_OBJID = 47,
        CPP_STREAM_FLUSH = 48,
        CPP_UNI_CAT = 49,
        CPP_GC_TRACE = 50,
        CPP_PARSE_DOUBLE = 51,
        CPP_DUPLICATE = 52,
        CPP_UNI_CASE = 53,
        CPP_RANDOM = 54,
        CPP_PANIC = 55,
        CPP_HAS_SLOT = 56,
        CPP_WHILE_REGS = 57,
        CPP_WHILE_REGS_ZERO = 58,
        CPP_FRESH = 59;
    constexpr long
        GTU_EMPTY = 0,
        GTU_LOOP_DO = 1,
        GTU_RETURN = 2,
        GTU_THROW = 3,
        GTU_TERMINATE = 4,
        GTU_HANDLER = 5,
        GTU_CALL_ONE = 6,
        GTU_CALL_ZERO = 7,
        GTU_MISSING = 8,
        GTU_TRUE = 9,
        GTU_FALSE = 10,
        GTU_NIL = 11,
        GTU_ERROR_MESSAGE = 12,
        GTU_ERROR = 13,
        GTU_KEYS = 14,
        GTU_KEY_TERM = 15,
        GTU_THROW_OBJ = 16,
        GTU_PANIC = 17,
        GTU_WHILE_DO = 18,
        GTU_POP_TWO = 19,
        GTU_WHILE_AGAIN = 20;

}

ObjectPtr spawnObjects(IntState& state, ReadOnlyState& reader, int argc, char** argv);

void throwError(IntState& state, const ReadOnlyState& reader, std::string name, std::string msg);
void throwError(IntState& state, const ReadOnlyState& reader, std::string name);
void throwError(IntState& state, const ReadOnlyState& reader, ObjectPtr obj);

#endif // STANDARD_HPP
