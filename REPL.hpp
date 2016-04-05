#ifndef _REPL_HPP_
#define _REPL_HPP_

#include "Proto.hpp"

ObjectPtr spawnREPLObjects(ObjectPtr& global);
ObjectPtr spawnREPLObjects(ObjectPtr& global, ObjectPtr& cont);

void runREPL(ObjectPtr& global);

#endif // _REPL_HPP_
