//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef PARENTS_HPP
#define PARENTS_HPP

#include <list>
#include "Proto.hpp"
#include "Symbol.hpp"

/// Returns the most direct parent of `obj` which has a slot named
/// `name`. If no such slot exists, the null pointer is returned.
///
/// Note that this method does not take `missing` methods into account
/// when performing the lookup, so only slots which actually exist
/// will be seen.
///
/// \param obj the object to check
/// \param name the slot's name
/// \return the origin, or null if the slot does not exist
ObjectPtr origin(ObjectPtr obj, Symbolic name);

/// Performs a lookup for a slot with the given name on the object and
/// its inheritance hierarchy. If such a slot is found, the value of
/// the slot is returned. Otherwise, the null pointer is returned.
///
/// Note that this method does not take `missing` methods into account
/// when performing the lookup, so only slots which actually exist
/// will be seen.
///
/// \param obj the object to check
/// \param name the slot's name
/// \return the slot's value, or null if the slot does not exist
ObjectPtr objectGet(ObjectPtr obj, Symbolic name);

#endif // PARENTS_HPP
