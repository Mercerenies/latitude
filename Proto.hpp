#ifndef PROTO_HPP
#define PROTO_HPP

#include "Process.hpp"
#include "Stream.hpp"
#include "Symbol.hpp"
#include "Number.hpp"
#include "Instructions.hpp"
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

/// A pointer to an Object.
class ObjectPtr {
private:
    Object* impl;
    void up();
    void down();
public:
    ObjectPtr();
    ObjectPtr(nullptr_t);
    explicit ObjectPtr(Object* ptr);
    ObjectPtr(const ObjectPtr& ptr);
    ObjectPtr(ObjectPtr&& ptr);
    ~ObjectPtr();
    ObjectPtr& operator=(const ObjectPtr& ptr);
    ObjectPtr& operator=(ObjectPtr&& ptr);
    Object& operator*() const;
    Object* operator->() const;
    Object* get() const;
    bool operator==(const ObjectPtr& ptr) const;
    bool operator!=(const ObjectPtr& ptr) const;
    bool operator<(const ObjectPtr& ptr) const; // TODO Make this a separate class for std::set
};

/// \brief A primitive field, which can be either empty or an element
/// of any number of types.
using Prim = boost::variant<boost::blank, Number, std::string,
                            StreamPtr, Symbolic, ProcessPtr,
                            Method, StatePtr>;

/// A Slot is either empty (INH) or has contents (PTR).
enum class SlotType { PTR, INH };

/// \brief Global constants for protection types. These values are
/// designed to be used in a bitmask.
enum {
    // TODO A completely correct design would use these special
    // constants internally but expose a class-based API that
    // overrides the | and & operators, to avoid using the wrong type
    // accidentally.
    NO_PROTECTION = 0,
    PROTECT_ASSIGN = 1,
    PROTECT_DELETE = 2
};

/// A bitmask containing protection data.
typedef char protection_t;

/// A slot of an object, which may or may not exist. If it does not
/// exist, the type of the slot is INH, which indicates that it may
/// exist in a parent or that it may not exist at all. If the type is
/// PTR, the slot contains an object pointer directly.
class Slot {
private:
    ObjectPtr obj;
    protection_t protection;
public:

    /// Constructs an empty slot with no protection.
    Slot() noexcept;

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
    Slot(ObjectPtr ptr, protection_t protect) noexcept;

    /// Returns the slot's type. If the type is SlotType::INH then the
    /// slot is empty (that is, the slot's correct value may be
    /// "inherited"). If the type is SlotType::PTR then the slot has a
    /// non-null value (that is, the "pointer" is valid).
    ///
    /// \return the slot's type
    SlotType getType() const noexcept;

    /// Returns the slot's object pointer, or null if the slot is
    /// empty.
    ///
    /// \return the pointer
    ObjectPtr getPtr() const;

    /// Stores an object in the slot. If the argument is null, the
    /// slot is instead emptied.
    ///
    /// \param ptr the object pointer
    void putPtr(ObjectPtr ptr);

    /// Adds new protection bits to the slot's protection mask.
    ///
    /// \param p the bitmask to add
    void addProtection(protection_t p) noexcept;

    /// Returns whether the object has all of the given protection
    /// bits set.
    ///
    /// \param p the bits to check
    /// \return whether the bits are set
    bool isProtected(protection_t p) const noexcept;

    /// Returns whether any of the object's protection bits are set.
    ///
    /// \return whether any bits are set
    bool hasAnyProtection() const noexcept;

};

/// An object in the language. Objects have slots which are referenced
/// by symbols, as well as a special `prim` field which is not
/// directly accessible using the syntax of the language. The slots
/// are used to store information, like instance variables in C++. The
/// `prim` field can be used to store a C++ construct, such as a
/// `double` or an `std::string`, in an object in the
/// language. Internally, the core libraries use this `prim` field to
/// implement many of the built-in numerical, string, and symbol
/// methods.
class Object {
private:
    std::unordered_map<Symbolic, Slot> slots;
    Prim primitive;

    Slot getSlot(Symbolic key) const;

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
    /// protection, as if through Slot::isProtected().
    ///
    /// \param key the key
    /// \param p the protection bitmask
    /// \return whether the protection exists
    bool isProtected(Symbolic key, protection_t p) const;

    /// Returns whether the slot has any protection, as if through
    /// Slot::hasAnyProtection().
    ///
    /// \param key the key for the slot
    /// \return whether any protection exists
    bool hasAnyProtection(Symbolic key) const;

    /// Adds protection to the given slot, as if through
    /// Slot::addProtection().
    ///
    /// \param key the key for the slot
    /// \param p the protection bitmask to add
    /// \return false if the slot did not exist, true otherwise
    bool addProtection(Symbolic key, protection_t p);

    /// Adds protection to all of the given slots.
    ///
    /// \param p the protection to add
    void protectAll(protection_t p);

    /// Adds protection to all of the given slots.
    ///
    /// \param p the protection to add
    /// \param key the first key
    /// \param keys the remaining keys
    template <typename T, typename... Ts>
    void protectAll(protection_t p, T key, Ts... keys);

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
void Object::protectAll(protection_t p, T key, Ts... keys) {
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
std::list<ObjectPtr> hierarchy(ObjectPtr obj);

/// Sets the $whereAmI variable. Should be used when the "execution
/// environment" is changing, for instance when loading a file. Note
/// carefully that while several methods in the language will
/// implicitly set this before running, the Reader.hpp functions such
/// as eval() will NOT do so.
///
/// \param dyn the dynamic scope
/// \param here the location to the set the variable to
void hereIAm(ObjectPtr dyn, ObjectPtr here);

#endif // PROTO_HPP
