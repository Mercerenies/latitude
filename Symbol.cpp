#include "Symbol.hpp"
#include <sstream>

using namespace std;

Symbols Symbols::instance = Symbols();
long Symbols::gensymIndex = 100L;

Symbolic Symbols::gensym() {
    auto& data = get().syms.right;
    index_t index = 0;
    bool done = false;
    while (!done) {
        ostringstream oss;
        oss << "G" << gensymIndex;
        auto str = oss.str();
        if (data.find(str) == data.end()) {
            done = true;
            index = get()[str];
        }
        ++gensymIndex;
    }
    return { index };
}

Symbols& Symbols::get() {
    return instance;
}

auto Symbols::operator[](const std::string& str)
    -> index_t {
    auto& data = syms.right;
    if (data.find(str) == data.end())
        syms.insert(bimap_t::value_type(index++, str));
    return (*data.find(str)).second;
}

auto Symbols::operator[](const index_t& str)
    -> std::string {
    auto& data = syms.left;
    if (data.find(str) == data.end())
        return ""; // TODO Is this what we really want?
    return (*data.find(str)).second;
}
