//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef PROTO_HPP
#define PROTO_HPP

#include "Process.hpp"
#include "Stream.hpp"
#include "Symbol.hpp"
#include "Number.hpp"
#include "Instructions.hpp"
#include "Protection.hpp"
#include "HashMap.hpp"
#include <list>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <boost/variant.hpp>
#include <boost/blank.hpp>
#include <set>

/// \file
///
/// \brief The fundamental Latitude data structures.

/// \cond

// Yes, this is probably horrible from a design standpoint, but I needed some
// way to resolve the circular dependencies.
class Stmt; // From Reader.hpp
struct IntState; // From Bytecode.hpp
using StatePtr = std::shared_ptr<IntState>; // From Bytecode.hpp

/// \endcond

class Slot;
class Object;

/// A list of statements.
using LStmt = std::list< std::shared_ptr<Stmt> >;

/// A pointer to an Object. Specifically, an ObjectPtr is a pointer to
/// an object managed by the singleton Allocator.
using ObjectPtr = Object*;

/// This structure satisfies the C++ Compare trait, suitable for use
/// as a comparator in structures such as std::set and std::map.
struct PtrCompare {

    /// Compares two ObjectPtr instances. Returns whether the address
    /// of the first is strictly less than that of the second,
    /// according to the rules of address comparison.
    ///
    /// \param a the first object pointer
    /// \param b the second object pointer
    /// \return whether a < b
    bool operator()(const ObjectPtr& a, const ObjectPtr& b);

};

struct ObjectHierarchyIterator;

struct ObjectHierarchy {
    ObjectPtr curr;
    ObjectHierarchyIterator begin();
    ObjectHierarchyIterator end();
};

struct ObjectHierarchyIterator {

    typedef std::ptrdiff_t difference_type;
    typedef ObjectPtr value_type;
    typedef ObjectPtr* pointer;
    typedef ObjectPtr& reference;
    typedef std::input_iterator_tag iterator_category;

    ObjectPtr curr;
    bool operator==(ObjectHierarchyIterator);
    bool operator!=(ObjectHierarchyIterator);
    ObjectPtr& operator*();
    ObjectPtr* operator->();
    ObjectHierarchyIterator& operator++();
    ObjectHierarchyIterator operator++(int);
};

/// \brief A primitive field, which can be either empty or an element
/// of any number of types.
using Prim = boost::variant<boost::blank, Number, std::string,
                            StreamPtr, Symbolic, ProcessPtr,
                            Method, StatePtr>;

/// A Slot is either empty (INH) or has contents (PTR).
///
/// \deprecated This enum is now a glorified Boolean; use Booleans
/// directly instead
enum class [[deprecated]] SlotType { PTR, INH };

/// A slot of an object, which may or may not exist.
///
/// *Note:* It used to be necessary to interface directly with this
/// class, when accessing slots on Latitude objects. This class has
/// since become an internal detail not exposed in the public API of
/// the Object class, so it should seldom, if ever, be necessary for
/// external users to interface directly with this class. It is
/// provided here, rather than privately, for legacy reasons.
struct Slot {

    /// The object in the slot, possibly null.
    ObjectPtr obj;

    /// The slot's protection, undefined if obj is null.
    Protection protection;

    /// Constructs an empty slot with no protection.
    Slot() = default;

    /// Constructs a slot containing an object pointer and no
    /// protection. If the argument is null then an empty slot is
    /// constructed.
    ///
    /// \param ptr the object pointer
    Slot(ObjectPtr ptr) noexcept;

    /// Constructs a slot containing an object pointer and some
    /// protection mask. If the argument is null then an empty slot is
    /// constructed.
    ///
    /// \param ptr the object pointer
    /// \param protect the protection bitmask
    Slot(ObjectPtr ptr, Protection protect) noexcept;

    /// Returns whether the slot contains a non-null object.
    ///
    /// \return true if `obj` is not null
    bool hasObject() const noexcept;

};

bool operator==(const Slot& a, const Slot& b);
bool operator!=(const Slot& a, const Slot& b);

/// An object in the language. Objects have slots which are referenced
/// by symbols, as well as a special `prim` field which is not
/// directly accessible using the syntax of the language. The slots
/// are used to store information, like instance variables in C++. The
/// `prim` field can be used to store a C++ construct, such as a
/// `double` or an `std::string`, in an object in the language.
/// Internally, the core libraries use this `prim` field to implement
/// many of the built-in numerical, string, and symbol methods.
class Object {
private:
    HashMap<Symbolic, Slot> slots;
    Prim primitive;

    Slot* getSlot(Symbolic key);
    const Slot* getSlot(Symbolic key) const;

public:

    /// Returns a the object in the specified slot. If no such
    /// slot exists, the null pointer is returned.
    ///
    /// \param key the key
    /// \return the slot
    ObjectPtr operator [](Symbolic key) const;

    /// Stores a particular object, which shall be non-null, at the
    /// given key. If no slot at that key exists, one is created with
    /// the particular object as its contents. If a slot already
    /// exists, its contents are replaced. To delete a slot, the
    /// remove() method should be used, as passing a null pointer to
    /// this method has undefined consequences.
    ///
    /// \param key the key at which to store the object
    /// \param ptr the object to store
    void put(Symbolic key, ObjectPtr ptr);

    /// Removes the slot with the given key from the object. If no
    /// such slot exists, this method has no effect. Note that this
    /// class does not implement prototypical parenting semantics, so
    /// this method is only designed to remove slots that exist on the
    /// object itself, not on parent objects.
    ///
    /// \param key the key to remove
    void remove(Symbolic key);

    /// Returns a set containing all of the symbols for which this
    /// object has slots.
    ///
    /// \return a set of keys
    std::set<Symbolic> directKeys() const;

    /// Returns whether the slot with the given name has the given
    /// protection.
    ///
    /// \param key the key
    /// \param p the protection bitmask
    /// \return whether the protection exists
    bool isProtected(Symbolic key, Protection p) const;

    /// Returns whether the slot has any protection.
    ///
    /// \param key the key for the slot
    /// \return whether any protection exists
    bool hasAnyProtection(Symbolic key) const;

    /// Adds protection to the given slot.
    ///
    /// \param key the key for the slot
    /// \param p the protection bitmask to add
    /// \return false if the slot did not exist, true otherwise
    bool addProtection(Symbolic key, Protection p);

    /// Adds protection to all of the given slots.
    ///
    /// \param p the protection to add
    void protectAll(Protection p);

    /// Adds protection to all of the given slots.
    ///
    /// \param p the protection to add
    /// \param key the first key
    /// \param keys the remaining keys
    template <typename T, typename... Ts>
    void protectAll(Protection p, T key, Ts... keys);

    /// Returns a reference to the `prim` field of the object.
    ///
    /// \return the `prim` field
    Prim& prim() noexcept;

    /// Sets the `prim` field of the object to the specified value,
    /// which must be assignable to the type Prim. The old value of
    /// the `prim` field is returned.
    ///
    /// \param prim0 the new value
    /// \return the old value
    template <typename T>
    Prim prim(const T& prim0);

};

template <typename T, typename... Ts>
void Object::protectAll(Protection p, T key, Ts... keys) {
    addProtection(key, p);
    protectAll(p, keys...);
}

template <typename T>
Prim Object::prim(const T& prim0) {
    Prim old = primitive;
    primitive = prim0;
    return old;
}

/// Clones the object, using the global garbage collector to allocate
/// the new object.
///
/// \param obj the object to clone
/// \return a newly cloned object
ObjectPtr clone(ObjectPtr obj);

/// Gets a set of all of the keys in the object by recursive
/// computation. If the parent keys are not desired, the instance
/// method `directKeys` on `Object` will return only the directly
/// available keys without checking the "parent" slot.
///
/// \param obj the object
/// \return a set of keys
std::set<Symbolic> keys(ObjectPtr obj);

/// Returns the object's inheritance hierarchy, starting with the
/// object itself and stopping when it encounters a loop in parents,
/// usually at the top-level `Object` object.
///
/// \param obj the object
/// \return the hierarchy of parents
ObjectHierarchy hierarchy(ObjectPtr obj);

/// Sets the $whereAmI variable. Should be used when the "execution
/// environment" is changing, for instance when loading a file. Note
/// carefully that while several methods in the language will
/// implicitly set this before running, the Reader.hpp functions such
/// as eval() will NOT do so.
///
/// \param dyn the dynamic scope
/// \param here the location to the set the variable to
///
/// \deprecated Assign the variable directly instead
[[deprecated]]
void hereIAm(ObjectPtr dyn, ObjectPtr here);

#endif // PROTO_HPP
