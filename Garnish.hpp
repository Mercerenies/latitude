#ifndef _GARNISH_HPP_
#define _GARNISH_HPP_

#include <string>
#include <boost/blank.hpp>
#include "Proto.hpp"

/*
 * Garnishes a Boolean value, returning True or False.
 */
ObjectPtr garnish(ObjectPtr global, bool value);
/*
 * Garnishes a blank, returning Nil.
 */
ObjectPtr garnish(ObjectPtr global, boost::blank value);
/*
 * Garnishes a string, returning a String.
 */
ObjectPtr garnish(ObjectPtr global, std::string value);
/*
 * Garnishes a double, returning a Number.
 */
ObjectPtr garnish(ObjectPtr global, double value);
/*
 * Garnishes an integer, returning a Number.
 */
ObjectPtr garnish(ObjectPtr global, int value);

/*
 * Converts the object's `prim` field to a string, using the
 * appropriate conversion technique.
 */
std::string primToString(ObjectPtr obj);
/*
 * Compares two `prim` fields for equality. This will never fail
 * outright and will always return `false` for two different types.
 */
bool primEquals(ObjectPtr obj1, ObjectPtr obj2);

/*
 * Using the current lexical and dynamic contexts, prints all of the slots
 * of the object to the stream specified. This is mainly designed as a debug
 * function and should seldom be used in production code.
 */
void dumpObject(ObjectPtr lex, ObjectPtr dyn, Stream& stream, ObjectPtr obj);
/*
 * Calls `toString` on the object and prints it to the stream specified.
 */
void simplePrintObject(ObjectPtr lex, ObjectPtr dyn, Stream& stream, ObjectPtr obj);

#endif // _GARNISH_HPP_
