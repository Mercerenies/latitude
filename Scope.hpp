#ifndef _SCOPE_HPP_
#define _SCOPE_HPP_
#include "Proto.hpp"

struct Scope {
    ObjectPtr lex;
    ObjectPtr dyn;
    Scope(ObjectPtr, ObjectPtr);
};

#endif
