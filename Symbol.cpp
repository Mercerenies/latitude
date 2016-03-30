#include "Symbol.hpp"

Symbols Symbols::instance = Symbols();

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
