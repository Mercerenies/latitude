#ifndef _PROTO_HPP_
#define _PROTO_HPP_

#include "Stream.hpp"
#include "Symbol.hpp"
#include "Number.hpp"
#include <list>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <boost/variant.hpp>
#include <boost/blank.hpp>
#include <set>

struct SignalValidator; // From Cont.hpp
class Stmt; // From Reader.hpp

class Slot;
class Object;

using LStmt = std::list< std::shared_ptr<Stmt> >;
using ObjectPtr = std::weak_ptr<Object>;
using ObjectSPtr = std::shared_ptr<Object>;
using SystemCall = std::function<ObjectPtr(ObjectPtr, ObjectPtr, std::list<ObjectPtr>)>;
using Method = LStmt;
using Prim = boost::variant<boost::blank, Number, std::string,
                            Method, SystemCall, StreamPtr, Symbolic,
                            std::weak_ptr<SignalValidator> >;

enum class SlotType { PTR, INH };

/*
 * A slot in an object, which may or may not exist. If it does not exist,
 * the type of the slot is INH, which indicates that it may exist in a parent
 * or it may not exist at all. If the type is PTR, the slot contains an object
 * pointer.
 */
class Slot {
private:
    ObjectPtr obj;
public:
    Slot();
    Slot(ObjectPtr ptr);
    SlotType getType();
    ObjectPtr getPtr();
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
    virtual ~Object() = default;
    virtual Slot operator [](Symbolic key);
    virtual void put(Symbolic key, ObjectPtr ptr);
    virtual std::set<Symbolic> directKeys();
    Prim& prim();
    template <typename T>
    Prim prim(const T& prim0);
};

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
 * Accesses the object's meta field. This is purely a convenience function.
 *   meta(obj) === getInheritedSlot(obj, Symbols::get()["meta"]);
 */
ObjectPtr meta(ObjectPtr dyn, ObjectPtr obj);
/*
 * Accesses the object's slot, recursively checking the "parent" object if the slot
 * is not found. Returns a null pointer if the slot does not exist. Note that this
 * function is designed to handle loops in parents and will stop looking when it
 * encounters one. In fact, such a loop always exists by default, as `Object` is
 * its own parent in the standard library.
 */
ObjectPtr getInheritedSlot(ObjectPtr dyn, ObjectPtr obj, Symbolic name);
/*
 * Checks whether a given slot exists, using the same algorithm as `getInheritedSlot`.
 */
bool hasInheritedSlot(ObjectPtr dyn, ObjectPtr obj, Symbolic name);
/*
 * Returns the object who actually owns the slot which would be referenced in a
 * call to getInheritedSlot.
 */
ObjectPtr getInheritedOrigin(ObjectPtr dyn, ObjectPtr obj, Symbolic name);
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

#endif // _PROTO_HPP_
