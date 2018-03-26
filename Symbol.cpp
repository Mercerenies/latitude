#include "Symbol.hpp"
#include <sstream>
#include <algorithm>
#include <iterator>

using namespace std;

Symbols Symbols::instance = Symbols();
long Symbols::gensymIndex = 100L;

Symbols::Symbols()
    : syms(), index(0), parentIndex(0) {
    parentIndex = (*this)["parent"].index;
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
    if (n <= 0)
        return get()[""];
    Symbolic sym;
    sym.index = - n;
    return sym;
}

Symbols& Symbols::get() noexcept {
    return instance;
}

bool Symbols::isUninterned(const std::string& str) {
    if (str == "")
        return false;
    return (str[0] == '~');
}

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
    if (str.index < 0) { // "Natural" symbol
        ostringstream str0;
        str0 << "~NAT" << abs(str.index);
        return str0.str();
    }
    if (data.find(str.index) == data.end())
        return "";
    return (*data.find(str.index)).second;
}

bool operator ==(const Symbolic& a, const Symbolic& b) noexcept {
    return a.index == b.index;
}

bool operator <(const Symbolic& a, const Symbolic& b) noexcept {
    return a.index < b.index;
}
