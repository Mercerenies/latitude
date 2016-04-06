#ifndef _GARNISH_HPP_
#define _GARNISH_HPP_

#include <string>
#include <boost/blank.hpp>
#include "Proto.hpp"

ObjectPtr garnish(ObjectPtr global, bool value);
ObjectPtr garnish(ObjectPtr global, boost::blank value);
ObjectPtr garnish(ObjectPtr global, std::string value);
ObjectPtr garnish(ObjectPtr global, double value);
ObjectPtr garnish(ObjectPtr global, int value);

std::string primToString(ObjectPtr obj);
bool primEquals(ObjectPtr obj1, ObjectPtr obj2);

void dumpObject(ObjectPtr lex, ObjectPtr dyn, Stream& stream, ObjectPtr obj);
void simplePrintObject(ObjectPtr lex, ObjectPtr dyn, Stream& stream, ObjectPtr obj);

#endif // _GARNISH_HPP_
