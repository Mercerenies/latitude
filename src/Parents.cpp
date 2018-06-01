
#include "Parents.hpp"
#include "Proto.hpp"

ObjectPtr origin(ObjectPtr obj, Symbolic name) {
    for (ObjectPtr curr : hierarchy(obj)) {
        if ((*curr)[name] != nullptr)
            return curr;
    }
    return nullptr;
}

ObjectPtr objectGet(ObjectPtr obj, Symbolic name) {
    ObjectPtr curr = origin(obj, name);
    if (curr == nullptr)
        return nullptr;
    else
        return (*curr)[name];
}
