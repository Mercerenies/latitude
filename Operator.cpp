
#include "Operator.h"
#include "Unicode.hpp"
#include <cstdlib>
#include <cstring>

static const char* ops = "!@#%^*-+/<=>?\\|~"; // Expand to Pd, Po, Sm, Sk (all of P*, S*?) /////

BOOL is_operator(char* id) {
    // TODO Use the Unicode navigation functions rather than just going by bytes (/////)
    while (*id != 0) {
        if (!is_operator_char((long)id[0]))
            return false;
        ++id;
    }
    return true;
}

BOOL is_operator_char(long cp) {
    char curr = (char)cp;
    return (strchr(ops, curr) != NULL);
}
