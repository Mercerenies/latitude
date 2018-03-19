
#include "Operator.h"
#include "Unicode.hpp"
#include <cstdlib>
#include <cstring>
#include <boost/optional.hpp>

static const char* ops = "!@#%^*-+/<=>?\\|~"; // Expand to Pd, Po, Sm, Sk (all of P*, S*?) /////

BOOL isOperator(char* id) {
    std::string str(id);
    boost::optional<long> pos { 0L };
    while (pos) {
        if (auto ch = charAt(str, *pos)) {
            if (!isOperatorChar(ch->codePoint())) {
                return false;
            }
        }
        pos = nextCharPos(str, *pos);
    }
    return true;
}

BOOL isOperatorChar(long cp) {
    char curr = (char)cp;
    return (strchr(ops, curr) != NULL);
}
