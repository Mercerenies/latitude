#ifndef _ALLOCATOR_HPP_
#define _ALLOCATOR_HPP_

#include "Proto.hpp"

/*
 * A singleton object which manages allocation and deallocation of objects. Note that the GC object
 * manages the garbage collection algorithm to remove old memory. This object manages the lower-level
 * task of actually performing the memory claiming and freeing.
 */
class Allocator {
private:
    static Allocator instance;
    Allocator() = default;
public:
    static Allocator& get() noexcept;
    ObjectPtr allocate();
    void free(ObjectPtr);
};

#endif // _ALLOCATOR_HPP_
