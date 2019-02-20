//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef SYMBOL_HPP
#define SYMBOL_HPP

#include <unordered_map>
#include <deque>
#include <string>
#include <functional>

/// \file
///
/// \brief The symbol table management singleton and associated functions.

struct Symbolic;

/// There are three types of symbols in the Latitude language. This
/// enumeration distinguishes between them.
enum class SymbolType {
    /// A symbol which is interned by both name and index.
    STANDARD,
    /// A symbol which is interned only by its index.
    GENERATED,
    /// A symbol which remains uninterned.
    NATURAL
};

/// \brief A singleton containing the global symbol table.
///
/// A singleton which maintains an association between language
/// symbols and internal numerical identifiers to allow constant-time
/// comparison.
class Symbols {
public:
    /// The internal type used for the comparisons of Latitude
    /// symbols.
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

    /// Produces a new generated symbol with a default prefix.
    ///
    /// \return the new symbol
    static Symbolic gensym();

    /// Produces a new generated symbol with the given prefix.
    ///
    /// \return the new symbol
    static Symbolic gensym(std::string prefix);

    /// Returns the natural symbol associated with the value.
    ///
    /// \param n A nonnegative integer
    /// \return the natural symbol
    static Symbolic natural(int n);

    /// Returns the singleton Symbols instance.
    ///
    /// \return the singleton
    static Symbols& get() noexcept;

    /// Returns what type of symbol the argument represents.
    ///
    /// \param sym a symbol
    /// \return the type of symbol
    static SymbolType symbolType(Symbolic sym);

    /// Returns whether or not a symbol with the given name contains
    /// special characters which will require escaping. If this method
    /// returns true, then the name should be printed with the `'()`
    /// syntax, taking care to escape special characters.
    ///
    /// A name will require escaping if it contains any of the
    /// following characters: `.,:()[]{}"'`, space, tab, newline, or
    /// the null character.
    ///
    /// \param str the name
    /// \return whether the name will require escaping when printed
    static bool requiresEscape(const std::string& str);

    /// Returns the index of the standard symbol whose name is
    /// `parent`. This particular symbol is cached as a special place
    /// in memory, so accessing it through this method is
    /// significantly faster than the usual hash lookup performed when
    /// calling `operator[]`.
    ///
    /// \return the `parent` symbol
    static Symbolic parent();

    /// Returns the symbol associated with the given name. This method
    /// may modify the symbol table if it is necessary to construct a
    /// new symbol.
    ///
    /// If the name begins with a tilde (`~`), then a new generated
    /// symbol will be constructed and returned. The symbol table will
    /// be modified so that the new symbol's index (but *not* its
    /// name) is stored as a key.
    ///
    /// If the name is nonempty and consists only of ASCII digits (`0`
    /// through `9`), then the appropriate natural symbol will be
    /// returned. In this case, the symbol table will be left
    /// unmodified.
    ///
    /// Otherwise, the name will be looked up in the existing symbol
    /// table. If the name exists as a key, then the pre-existing
    /// symbol will be returned. Otherwise, a new standard symbol is
    /// created and returned. In the latter case, the symbol table
    /// will be modified so that the new symbol's name and index are
    /// stored as keys.
    ///
    /// \param str the name
    /// \return the appropriate symbol
    Symbolic operator[](const std::string& str);

    /// Returns the name of the given symbol. If the symbol is a
    /// natural symbol, then its name is simply a string
    /// representation of the nonnegative integer used to construct
    /// the natural symbol. Otherwise, the name will be looked up in
    /// the symbol table itself.
    ///
    /// Looking up a name from the symbol's index is a left inverse
    /// but *not* a right inverse to looking up an index from a name.
    /// That is, taking a symbol's name and converting it to an index
    /// and back will always produce the same name, but taking an
    /// index and converting it to a name and back will not
    /// necessarily produce the same index, for instance if the symbol
    /// was generated.
    ///
    /// \param str the symbol
    /// \return the name of the symbol
    std::string operator[](const Symbolic& str);

};

/// \brief A thin wrapper for symbols in index form.
///
/// A thin wrapper for symbols in index form, to avoid accidental
/// casting to and from integer types. The index of the symbol can be
/// accessed explicitly via the `index` field of this structure, and
/// Symbolic instances can be constructed from integers by aggregate
/// initialization.
struct Symbolic {
    /// Initializes to an unused "null" symbolic value.
    Symbolic() = default;
    /// The index of the symbol.
    Symbols::index_t index;
};

/// Compares the two symbols for equality. Two symbols are equal if
/// and only if their indices are the same. This test is no slower
/// than a simple integer comparison.
///
/// \param a the first symbol
/// \param b the second symbol
/// \return whether the two are equal
bool operator ==(const Symbolic& a, const Symbolic& b) noexcept;

bool operator !=(const Symbolic& a, const Symbolic& b) noexcept;


/// Symbols follow an arbitrary but consistent total ordering to allow
/// their admission into tree-like structures such as `std::map` and
/// `std::set`. This comparison can be computed in constant time.
///
/// \param a the first symbol
/// \param b the second symbol
/// \return whether the first symbol is strictly less than the
///         second, according to the arbitrary ordering
bool operator <(const Symbolic& a, const Symbolic& b) noexcept;

namespace std {
    /// Symbolic instances are hashable by their index, for use in
    /// hash structures such as `std::unordered_set` and
    /// `std::unordered_map`.
    template <>
    class hash<Symbolic> {
    public:
        /// Returns the hash value for the symbol.
        ///
        /// \param val the symbol
        /// \return the symbol's hash value
        size_t operator()(const Symbolic& val) const noexcept {
            return hash<Symbols::index_t>()(val.index);
        }
    };
}

#endif // SYMBOL_HPP
