#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <vector>
#include "Proto.hpp"

struct ObjectEntry;
struct CountedArray;

/*
 * An object entry contains an Object (in the Proto.cpp sense), a flag indicating whether it is in use or
 * free, and an index referring back to its location in the array, to allow for constant-time freeing of
 * object memory.
 */
struct ObjectEntry {
    Object object;
    bool in_use;
    unsigned int index;
};

/*
 * This array keeps a count of the number of elements in the array which are being used right now, to
 * determine which array, or "bucket", to use when a new object is required.
 */
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

#endif // ALLOCATOR_HPP
