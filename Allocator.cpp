#include "Allocator.hpp"
#include "Proto.hpp"

constexpr size_t START_CAP = 10;
constexpr size_t BUCKET_SIZE = 100;

CountedArray::CountedArray() :
    used(0), array(BUCKET_SIZE, ObjectEntry()) {}

Allocator Allocator::instance = Allocator();

Allocator::Allocator() : vec() {
    vec.reserve(START_CAP);
}

Allocator& Allocator::get() noexcept {
    return instance;
}

ObjectPtr Allocator::allocate() {
    for (CountedArray& carray : vec) {
        if (carray.used < BUCKET_SIZE) {
            // There is an opening, so use it (first fit)
            for (ObjectEntry& entry : carray.array) {
                if (!entry.in_use) {
                    carray.used++;
                    entry.in_use = true;
                    return &entry.object;
                }
            }
        }
    }
    // No openings available anywhere, so rehash and move on
    std::cout << "Grow " << vec.size() << std::endl;
    vec.push_back(CountedArray());
    return allocate();
    //return new Object();
}

void Allocator::free(ObjectPtr obj) {
    // Find the object in one of the arrays
    for (CountedArray& carray : vec) {
        void* begin = static_cast<void*>(&carray.array.front());
        void* end = static_cast<void*>(&carray.array.back());
        if (((void*)obj >= begin) && ((void*)obj < end)) {
            // Found it
            carray.used--;
            // This should be safe because Object is the first field in ObjectEntry, and ObjectEntry is
            // standard layout.
            reinterpret_cast<ObjectEntry*>(obj)->in_use = false;
            return;
        }
    }
    //delete obj;
}
