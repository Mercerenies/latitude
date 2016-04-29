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

bool areExceptionsEnabled(Scope scope);
[[ noreturn ]] void gracefulTerminate(Scope scope, std::string category, std::string message);

// These are convenience functions for various language errors.
ProtoError doSystemArgError(Scope scope, std::string name, int expected, int got);
ProtoError doSlotError(Scope scope, ObjectPtr problem, Symbolic slotName);
ProtoError doParseError(Scope scope);
ProtoError doParseError(Scope scope, std::string message);
ProtoError doEtcError(Scope scope, std::string errorName, std::string msg);

#endif // _STANDARD_HPP_
