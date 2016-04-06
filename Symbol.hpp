#ifndef _SYMBOL_HPP_
#define _SYMBOL_HPP_

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <string>
#include <functional>

struct Symbolic;

/*
 * A singleton which keeps a list of language symbols and internal numerical
 * identifiers to allow constant-time comparison.
 */
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
static Symbolic gensym(std::string prefix);
    static Symbols& get();
    static bool isUninterned(const std::string& str);
    Symbolic operator[](const std::string& str);
    std::string operator[](const Symbolic& str);
};

/*
 * A thin wrapper for symbols in index form, to avoid
 * accidental casting to and from integer types.
 */
struct Symbolic {
    Symbols::index_t index;
};

/*
 * Symbols are comparable for equality in constant time.
 */
bool operator ==(const Symbolic& a, const Symbolic& b);
/*
 * Symbols follow an arbitrary but consistent total ordering
 * to allow their admission into tree-like structures such
 * as `std::map` and `std::set`.
 */
bool operator <(const Symbolic& a, const Symbolic& b);

namespace std {
    /*
     * `Symbolic` instances are hashable by their index, for use
     * in hash structures such as `std::unordered_set` and
     * `std::unordered_map`.
     */
    template <>
    class hash<Symbolic> {
    public:
        size_t operator()(const Symbolic& val) const noexcept {
            return hash<Symbols::index_t>()(val.index);
        }
    };
}

#endif
