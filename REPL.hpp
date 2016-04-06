#ifndef _REPL_HPP_
#define _REPL_HPP_

#include "Proto.hpp"

/*
 * Creates a `REPL` object and gives it basic methods for manipulating the
 * top-level loop.
 */
ObjectPtr spawnREPLObjects(ObjectPtr& global);
/*
 * Creates a `REPL` object and gives it basic methods for manipulating the
 * top-level loop. The continuation passed in should be a continuation that
 * will exit the REPL loop and is used to implement a `quit` method.
 */
ObjectPtr spawnREPLObjects(ObjectPtr& global, ObjectPtr& cont);

/*
 * Runs the REPL. Internally, this creates a continuation, calls the two-argument
 * form of `spawnREPLObjects`, and then loops.
 */
void runREPL(ObjectPtr& global);

#endif // _REPL_HPP_
