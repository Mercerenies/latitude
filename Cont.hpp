#ifndef _CONT_HPP_
#define _CONT_HPP_

#include "Symbol.hpp"
#include "Proto.hpp"

/*
 * A basic type whose existence is used to verify that a throw of a
 * `Signal` is safe.
 */
struct SignalValidator {};

/*
 * An object that is thrown to escape a `callCC` block in the language
 * itself.
 */
class Signal {
private:
    Symbolic identifier;
    ObjectPtr object;
public:
    Signal(Symbolic id, ObjectPtr obj);
    bool match(Symbolic other);
    ObjectPtr getObject();
};

/*
 * An exception thrown within the language or by a language system call.
 */
class ProtoError {
private:
    ObjectPtr object;
public:
    ProtoError(ObjectPtr obj);
    ObjectPtr getObject();
};

/*
 * Throws the object as a `ProtoError` object.
 */
[[ noreturn ]] void throwProtoError(const ObjectPtr&);

#endif // _CONT_HPP_
