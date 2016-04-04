#ifndef _CONT_HPP_
#define _CONT_HPP_

#include "Symbol.hpp"
#include "Proto.hpp"

struct SignalValidator {};

class Signal {
private:
    Symbolic identifier;
    ObjectPtr object;
public:
    Signal(Symbolic id, ObjectPtr obj);
    bool match(Symbolic other);
    ObjectPtr getObject();
};

class ProtoError {
private:
    ObjectPtr object;
public:
    ProtoError(ObjectPtr obj);
    ObjectPtr getObject();
};

#endif // _CONT_HPP_
