#ifndef _STANDARD_HPP_
#define _STANDARD_HPP_

#include <string>
#include "Proto.hpp"
#include "Cont.hpp"

/*
 * Using the global garbage collector, spawns the standard library set of objects
 * and returns the default global scope.
 */
ObjectPtr spawnObjects();

// These are convenience functions for various language errors.
ProtoError doSlotError(Scope scope, ObjectPtr problem, Symbolic slotName);
ProtoError doParseError(Scope scope);
ProtoError doParseError(Scope scope, std::string message);

#endif // _STANDARD_HPP_
