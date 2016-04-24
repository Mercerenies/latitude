#include "GC.hpp"

GC GC::instance = GC();

GC& GC::get() {
    return instance;
}

ObjectPtr GC::allocate() {
    ObjectSPtr ptr = std::make_shared<Object>();
#if GC_PRINT > 2
    std::cout << "<<Allocating " << ptr << ">>" << std::endl;
#endif
    alloc.insert(ptr);
    return ptr;
}
