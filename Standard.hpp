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
        CPP_UNI_CHR = 44;
    constexpr long
        GTU_LOOP_DO = 1;

}

ObjectPtr spawnObjects(IntState& state, ReadOnlyState& reader, int argc, char** argv);

void throwError(IntState& state, std::string name, std::string msg);
void throwError(IntState& state, std::string name);

#endif // STANDARD_HPP
