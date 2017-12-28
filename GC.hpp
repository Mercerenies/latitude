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
    std::set<Object*> alloc;
    GC() = default;
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
    /// \param state the interpreter state
    /// \param reader the read-only interpreter state
    /// \return the number of objects freed
    long garbageCollect(IntState& state, ReadOnlyState& reader);

    /// A convenience function which garbage collects given an
    /// arbitrary iterable sequence of global values.
    ///
    /// \param begin the begin iterator
    /// \param end the end iterator
    /// \return the number of objects freed
    /// \see garbageCollect(std::vector<Object*>)
    template <typename InputIterator>
    long garbageCollect(InputIterator begin, InputIterator end);

    size_t getTotal();

};

template <typename InputIterator>
long GC::garbageCollect(InputIterator begin, InputIterator end) {
    std::vector<Object*> globals;
    globals.insert(globals.end(), begin, end);
    return GC::garbageCollect(globals);
}

#endif // GC_HPP
