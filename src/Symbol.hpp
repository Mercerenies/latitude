#ifndef SYMBOL_HPP
#define SYMBOL_HPP

#include <unordered_map>
#include <deque>
#include <string>
#include <functional>

struct Symbolic;

enum class SymbolType {
    STANDARD, GENERATED, NATURAL
};

/*
 * A singleton which keeps a list of language symbols and internal numerical
 * identifiers to allow constant-time comparison.
 */
class Symbols {
public:
    using index_t = long;
private:
    static long gensymIndex;
    static Symbols instance;
    std::deque<std::string> syms;
    std::unordered_map<std::string, index_t> names;
    index_t index;
    index_t parentIndex;
    Symbols();
    bool hasGeneratedName(const std::string& str);
    bool hasNumericalName(const std::string& str);
public:
    static Symbolic gensym();
    static Symbolic gensym(std::string prefix);
    static Symbolic natural(int n);
    static Symbols& get() noexcept;
    static SymbolType symbolType(Symbolic sym);
    static bool requiresEscape(const std::string& str);
    static Symbolic parent();
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
bool operator ==(const Symbolic& a, const Symbolic& b) noexcept;
/*
 * Symbols follow an arbitrary but consistent total ordering
 * to allow their admission into tree-like structures such
 * as `std::map` and `std::set`.
 */
bool operator <(const Symbolic& a, const Symbolic& b) noexcept;

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

#endif // SYMBOL_HPP
