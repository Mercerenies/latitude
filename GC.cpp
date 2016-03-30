#include "GC.hpp"

GC GC::instance = GC();

GC& GC::get() {
    return instance;
}

ObjectPtr GC::allocate() {
    ObjectSPtr ptr(new Object());
    alloc.insert(ptr);
    return ptr;
}
