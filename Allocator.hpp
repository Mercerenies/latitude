#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <vector>
#include "Proto.hpp"

/// \file
///
/// \brief The Allocator class and the structures it depends on

struct ObjectEntry;
struct CountedArray;

/// \brief An ObjectEntry contains a Latitude Object instance and some
/// allocator metadata.
///
/// An ObjectEntry contains a Latitude Object instance, a flag
/// indicating whether it is in use or free, and an index referring
/// back to its location in the array, to allow for constant-time
/// freeing of object memory.
struct ObjectEntry {
    /// The object within the entry.
    Object object;
    /// Whether or not the object is being used by the VM.
    bool in_use;
    /// The index of the entry's bucket in the Allocator.
    unsigned int index;
};

/// \brief A CountedArray keeps a count of the number of elements
/// within it that are in use right now.
///
/// A CountedArray keeps a count of the number of elements in the
/// array which are being used right now, to determine which array, or
/// "bucket", to use when a new object is required.
struct CountedArray {
    /// A count of the number of elements that are in use.
    size_t used;
    /// The internal array of ObjectEntry instances.
    std::vector<ObjectEntry> array;
    /// Constructs a new CountedArray with the correct number of
    /// elements. Every CountedArray in the program will contain a
    /// constant, but unspecified, number of elements.
    CountedArray();
};

/// \brief A singleton object managing allocation and deallocation of
/// Latitude objects.
///
/// A singleton object which manages allocation and deallocation of
/// objects. Note that the GC object manages the garbage collection
/// algorithm to remove old memory. This object manages the
/// lower-level task of actually performing the memory claiming and
/// freeing.
class Allocator {
private:
    static Allocator instance;
    std::vector<CountedArray> vec;
    Allocator();
public:

    /// Retrieves the singleton allocator.
    ///
    /// \returns the allocator instance
    static Allocator& get() noexcept;
    ObjectPtr allocate();
    void free(ObjectPtr);
    void DEBUG();
};

#endif // ALLOCATOR_HPP
