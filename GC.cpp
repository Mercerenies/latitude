#include "GC.hpp"

GC GC::instance = GC();

GC& GC::get() {
    return instance;
}

ObjectPtr GC::allocate() {
    ObjectSPtr ptr = std::make_shared<Object>();
    alloc.insert(ptr);
    return ptr;
}
