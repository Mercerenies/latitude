//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "Symbol.hpp"
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <cctype>

using namespace std;

Symbols Symbols::instance = Symbols();
long Symbols::gensymIndex = 100L;

Symbols::Symbols()
    : syms(), names(), index(0), parentIndex(0) {
    // Okay, so this calls for some explanation. In order to use the
    // HashMap.hpp implementation, Symbols need to be default
    // constructible to a meaningful empty value. So the first symbol
    // interned (index 0) needs to have a value that will never be
    // accessed from inside Latitude. Thus, we generate a symbol and
    // immediately discard it, hence making index 0 unreachable.
    (*this)["~"];
    parentIndex = (*this)["parent"].index;
}

bool Symbols::hasGeneratedName(const std::string& str) {
    if (str == "")
        return false;
    return (str[0] == '~');
}

bool Symbols::hasNumericalName(const std::string& str) {
    if (str == "")
        return false;
    for (char ch : str) {
        if (!isdigit(ch))
            return false;
    }
    return true;
}

Symbolic Symbols::gensym() {
    return gensym("G");
}

Symbolic Symbols::gensym(std::string prefix) {
    ++gensymIndex;
    ostringstream oss;
    oss << "~" << prefix << gensymIndex;
    return get()[oss.str()];
}

Symbolic Symbols::natural(int n) {
    if (n < 0)
        return get()[""];
    Symbolic sym;
    sym.index = - n - 1;
    return sym;
}

Symbols& Symbols::get() noexcept {
    return instance;
}

SymbolType Symbols::symbolType(Symbolic sym) {
    if (sym.index < 0)
        return SymbolType::NATURAL;
    std::string name = get()[sym];
    if (get().hasGeneratedName(name))
        return SymbolType::GENERATED;
    return SymbolType::STANDARD;
};

bool Symbols::requiresEscape(const std::string& str){
    string str0(str); // Need a copy
    if ((str0 == "") || (str0 == "~"))
        return true;
    sort(str0.begin(), str0.end());
    string special(".,:()[]{}\"\' \t\n");
    special += '\0';
    sort(special.begin(), special.end());
    string str1;
    set_intersection(str0.begin(), str0.end(), special.begin(), special.end(),
                     inserter(str1, str1.begin()));
    return (str1.begin() != str1.end());
}

Symbolic Symbols::parent() {
    return { get().parentIndex };
}

Symbolic Symbols::operator[](const std::string& str) {
    if (hasGeneratedName(str)) {
        syms.emplace_back(str);
        ++index;
        assert((size_t)index == syms.size());
        return { index - 1 };
    } else if (hasNumericalName(str)) {
        return { - stoi(str) - 1 };
    } else {
        if (names.find(str) == names.end()) {
            syms.emplace_back(str);
            names.emplace(str, index);
            ++index;
            assert((size_t)index == syms.size());
        }
        Symbolic sym = { names.find(str)->second };
        return sym;
    }
}

std::string Symbols::operator[](const Symbolic& str) {
    if (str.index < 0) { // "Natural" symbol
        return to_string(- str.index - 1);
    }
    if ((size_t)str.index >= syms.size())
        return "";
    return syms[str.index];
}

bool operator ==(const Symbolic& a, const Symbolic& b) noexcept {
    return a.index == b.index;
}

bool operator !=(const Symbolic& a, const Symbolic& b) noexcept {
    return !(a == b);
}

bool operator <(const Symbolic& a, const Symbolic& b) noexcept {
    return a.index < b.index;
}
