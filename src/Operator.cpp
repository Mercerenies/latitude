
#include "Operator.h"
#include "Unicode.hpp"
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <boost/optional.hpp>

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
    static constexpr uni_class_t classes[] = {
        UNICLS_PC,
        UNICLS_PD,
        UNICLS_PS,
        UNICLS_PE,
        UNICLS_PI,
        UNICLS_PF,
        UNICLS_PO,
        UNICLS_SM,
        UNICLS_SC,
        UNICLS_SK,
        UNICLS_SO
    };
    uni_class_t cls = get_class((int)cp);
    if (cp == (long)'$')
        return false;
    return (std::find(std::begin(classes), std::end(classes), cls) != std::end(classes));
}
