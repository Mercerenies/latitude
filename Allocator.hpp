#ifndef _ALLOCATOR_HPP_
#define _ALLOCATOR_HPP_

#include <vector>
#include "Proto.hpp"

struct ObjectEntry {
    Object object;
    bool in_use;
};

struct CountedArray {
    size_t used;
    std::vector<ObjectEntry> array;
    CountedArray();
};

/*
 * A singleton object which manages allocation and deallocation of objects. Note that the GC object
 * manages the garbage collection algorithm to remove old memory. This object manages the lower-level
 * task of actually performing the memory claiming and freeing.
 */
class Allocator {
private:
    static Allocator instance;
    std::vector<CountedArray> vec;
    Allocator();
public:
    static Allocator& get() noexcept;
    ObjectPtr allocate();
    void free(ObjectPtr);
    void DEBUG();
};

#endif // _ALLOCATOR_HPP_
