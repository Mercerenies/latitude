#ifndef _SYMBOL_HPP_
#define _SYMBOL_HPP_

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <string>
#include <functional>

struct Symbolic;

class Symbols {
public:
    using index_t = long;
    using bimap_t = boost::bimap< boost::bimaps::unordered_set_of<index_t>,
                                  boost::bimaps::unordered_multiset_of<std::string> >;
private:
    static long gensymIndex;
    static Symbols instance;
    bimap_t syms;
    index_t index;
    Symbols() = default;
public:
    static Symbolic gensym();
    static Symbols& get();
    static bool isUninterned(const std::string& str);
    Symbolic operator[](const std::string& str);
    std::string operator[](const Symbolic& str);
};

struct Symbolic {
    Symbols::index_t index;
};

bool operator ==(const Symbolic& a, const Symbolic& b);

namespace std {
    template <>
    class hash<Symbolic> {
    public:
        size_t operator()(const Symbolic& val) const noexcept {
            return hash<Symbols::index_t>()(val.index);
        }
    };
}

#endif
