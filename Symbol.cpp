#include "Symbol.hpp"
#include <sstream>

using namespace std;

Symbols Symbols::instance = Symbols();
long Symbols::gensymIndex = 100L;

Symbolic Symbols::gensym() {
    return gensym("G");
}

Symbolic Symbols::gensym(std::string prefix) {
    ++gensymIndex;
    ostringstream oss;
    oss << "~" << prefix << gensymIndex;
    return get()[oss.str()];
}

Symbols& Symbols::get() {
    return instance;
}

bool Symbols::isUninterned(const std::string& str) {
    if (str == "")
        return false;
    return (str[0] == '~');
}

Symbolic Symbols::operator[](const std::string& str) {
    if (isUninterned(str)) {
        index_t curr = index++;
        syms.insert(bimap_t::value_type(curr, str));
        return { curr };
    } else {
        auto& data = syms.right;
        if (data.find(str) == data.end())
            syms.insert(bimap_t::value_type(index++, str));
        Symbolic sym = { (*data.find(str)).second };
        return sym;
    }
}

std::string Symbols::operator[](const Symbolic& str) {
    auto& data = syms.left;
    if (data.find(str.index) == data.end())
        return ""; // TODO Is this what we really want?
    return (*data.find(str.index)).second;
}

bool operator ==(const Symbolic& a, const Symbolic& b) {
    return a.index == b.index;
}
