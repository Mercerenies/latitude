#ifndef _GC_HPP_
#define _GC_HPP_

#include "Bytecode.hpp"
#include "Proto.hpp"
#include <vector>
#include <array>
#include <algorithm>

/*
 * A singleton object representing the global garbage collector. All
 * language `Object` instances should be allocated through this object
 * so that they can be cleaned up through this object.
 */
class GC {
private:
    static GC instance;
    std::set<ObjectSPtr> alloc;
    GC() = default;
public:
    static GC& get() noexcept;
    /*
     * Creates an object and returns a pointer to it.
     */
    ObjectPtr allocate();
    /*
     * Cleans up objects. Any object references which C++ or the embedded
     * code maintains should be passed in as arguments, as the algorithm
     * will assume anything that is unreachable from the arguments should
     * be freed. As an aside to this, `garbageCollect` called with no
     * arguments will free every object that was created using the garbage
     * collector.
     */
    long garbageCollect(std::vector<ObjectPtr>);
    long garbageCollect(IntState&);
    template <typename InputIterator>
    long garbageCollect(InputIterator begin, InputIterator end);
};

template <typename InputIterator>
long GC::garbageCollect(InputIterator begin, InputIterator end) {
    std::vector<ObjectPtr> globals;
    globals.insert(globals.end(), begin, end);
    return GC::garbageCollect(globals);
}

#endif // _GC_HPP_
