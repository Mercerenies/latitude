#ifndef _SYMBOL_HPP_
#define _SYMBOL_HPP_

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <string>

struct Symbolic;

class Symbols {
public:
    using index_t = long;
    using bimap_t = boost::bimap< boost::bimaps::unordered_set_of<index_t>,
                                  boost::bimaps::unordered_set_of<std::string> >;
private:
    static long gensymIndex;
    static Symbols instance;
    bimap_t syms;
    index_t index;
    Symbols() = default;
public:
    static Symbolic gensym();
    static Symbols& get();
    index_t operator[](const std::string& str);
    std::string operator[](const index_t& str);
};

struct Symbolic {
    Symbols::index_t index;
};

#endif
