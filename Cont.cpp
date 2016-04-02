#include "Cont.hpp"

Signal::Signal(Symbolic id, ObjectPtr obj)
    : identifier(id) , object(obj) {}

bool Signal::match(Symbolic other) {
    return (other == identifier);
}

ObjectPtr Signal::getObject() {
    return object;
}
