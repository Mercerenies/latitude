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

///// Make instruction sequences more "constant" for efficiency

// Yes, this is probably horrible from a design standpoint, but I needed some
// way to resolve the circular dependencies.
class Stmt; // From Reader.hpp
struct IntState; // From Bytecode.hpp
using StatePtr = std::shared_ptr<IntState>; // From Bytecode.hpp

class Slot;
class Object;

using LStmt = std::list< std::shared_ptr<Stmt> >;
using ObjectPtr = Object*;
using Prim = boost::variant<boost::blank, Number, std::string,
                            StreamPtr, Symbolic, ProcessPtr,
                            Method, StatePtr>;

enum class SlotType { PTR, INH };

// TODO A completely correct design would use these special constants internally but expose a class-based API that overrides the | and & operators, to avoid using the wrong type accidentally.
enum {
    NO_PROTECTION = 0,
    PROTECT_ASSIGN = 1,
    PROTECT_DELETE = 2
};

typedef char protection_t;

/*
 * A slot in an object, which may or may not exist. If it does not exist,
 * the type of the slot is INH, which indicates that it may exist in a parent
 * or it may not exist at all. If the type is PTR, the slot contains an object
 * pointer.
 */
class Slot {
private:
    ObjectPtr obj;
    protection_t protection;
public:
    Slot() noexcept;
    Slot(ObjectPtr ptr) noexcept;
    Slot(ObjectPtr ptr, protection_t protect) noexcept;
    SlotType getType() const noexcept;
    ObjectPtr getPtr() const;
    void putPtr(ObjectPtr ptr);
    void addProtection(protection_t p) noexcept;
    bool isProtected(protection_t p) const noexcept;
    bool hasAnyProtection() const noexcept;
};

/*
 * An object in the language. Objects have slots which are referenced by symbols,
 * as well as a special `prim` field which is not directly accessible using the
 * syntax of the language. The slots are used to store information, like instance
 * variables in C++. The `prim` field can be used to store a C++ construct, such
 * as a `double` or an `std::string`, in an object in the language. Internally,
 * the core libraries use this `prim` field to implement many of the built-in numerical,
 * string, and symbol methods.
 */
class Object {
private:
    std::unordered_map<Symbolic, Slot> slots;
    Prim primitive;
public:
    Slot operator [](Symbolic key) const;
    void put(Symbolic key, ObjectPtr ptr);
    void remove(Symbolic key);
    std::set<Symbolic> directKeys() const;
    bool isProtected(Symbolic key, protection_t p) const;
    bool hasAnyProtection(Symbolic key) const;
    void addProtection(Symbolic key, protection_t p);
    void protectAll(protection_t);
    template <typename T, typename... Ts>
    void protectAll(protection_t p, T key, Ts... keys);
    Prim& prim() noexcept;
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

/*
 * Clones the object, using the global garbage collector
 * to allocate the new object.
 */
ObjectPtr clone(ObjectPtr obj);
/*
 * Gets a set of all of the keys in the object. Recursively computes the set of keys
 * using an algorithm similar to `getInheritedSlot`. If the parent keys are not
 * desired, the instance method `directKeys` on `Object` will return only the directly
 * available keys without checking the "parent" slot.
 */
std::set<Symbolic> keys(ObjectPtr obj);
/*
 * Returns the object's inheritance hierarchy, starting with the object itself
 * and stopping when it encounters a loop in parents, usually at the top-level
 * `Object` object.
 */
std::list<ObjectPtr> hierarchy(ObjectPtr obj);

/*
 * Sets the $whereAmI variable. Should be used when the "execution environment" is changing,
 * for instance when loading a file. Note carefully that while several methods in the language
 * will implicitly set this before running, the Reader.hpp functions such as eval() will NOT
 * do so.
 */
void hereIAm(ObjectPtr dyn, ObjectPtr here);

#endif // PROTO_HPP
