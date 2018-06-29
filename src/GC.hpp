//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef GC_HPP
#define GC_HPP

#include "Bytecode.hpp"
#include "Proto.hpp"
#include <vector>
#include <array>
#include <algorithm>

/// \file
///
/// \brief The garbage collector class

/// A singleton object representing the global garbage collector. All
/// language `Object` instances should be allocated through this
/// object so that they can be cleaned up through this object.
class GC {
private:
    static GC instance;
    constexpr static long TOTAL_COUNT = 65536L;
    std::set<Object*> alloc;
    long count;
    unsigned long limit;
    bool tracing;
    GC();
public:

    /// Returns the garbage collector singleton instance.
    ///
    /// \return the singleton GC instance
    static GC& get() noexcept;

    /// Creates an object and returns a pointer to it.
    ///
    /// \return a pointer to a new object
    ObjectPtr allocate();

    /// Frees the object and updates the garbage collector to the fact
    /// that the object has been freed.
    ///
    /// \param obj the object to free
    void free(Object* obj);

    /// Frees every object in the garbage collector's allocated
    /// set. This method is primarily useful for graceful termination
    /// at the end of execution.
    void freeAll();

    /// Cleans up objects. Any object references maintained by C++ or
    /// by the embedded code should be passed in as arguments, as the
    /// algorithm will assume anything that is unreachable from the
    /// arguments should be freed. As a consequence of this,
    /// garbageCollect called with no arguments will free every object
    /// that was created using the garbage collector.
    ///
    /// \param globals the global variables accessible to the program
    /// \return the number of objects freed
    long garbageCollect(std::vector<Object*> globals);

    /// This is a convenience function which calls
    /// #garbageCollect(std::vector<Object*>) with all of the arguments from
    /// the various registers in the VM.
    ///
    /// \param vm the virtual machine state
    /// \return the number of objects freed
    long garbageCollect(VMState& vm);
    /// A convenience function which garbage collects given an
    /// arbitrary iterable sequence of global values.
    ///
    /// \param begin the begin iterator
    /// \param end the end iterator
    /// \return the number of objects freed
    /// \see garbageCollect(std::vector<Object*>)
    template <typename InputIterator>
    long garbageCollect(InputIterator begin, InputIterator end);

    /// Activates or deactivates tracing, so that the garbage
    /// collector prints a line of text whenever it runs.
    void setTracing(bool);

    size_t getTotal() const;

    size_t getLimit() const;

    void tick(VMState& vm);

};

template <typename InputIterator>
long GC::garbageCollect(InputIterator begin, InputIterator end) {
    std::vector<Object*> globals;
    globals.insert(globals.end(), begin, end);
    return GC::garbageCollect(globals);
}

#endif // GC_HPP
