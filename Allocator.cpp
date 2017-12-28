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
    unsigned int index = 0;
    for (CountedArray& carray : vec) {
        if (carray.used < BUCKET_SIZE) {
            // There is an opening, so use it (first fit)
            for (ObjectEntry& entry : carray.array) {
                if (!entry.in_use) {
                    carray.used++;
                    entry.in_use = true;
                    entry.index = index;
                    entry.object = Object();
                    return &entry.object;
                }
            }
        }
        ++index;
    }
    // No openings available anywhere, so rehash and move on
    vec.push_back(CountedArray());
    return allocate();
    //return new Object();
}

void Allocator::free(ObjectPtr obj) {
    ObjectEntry* entry = reinterpret_cast<ObjectEntry*>(obj.get());
    CountedArray& carray = vec[entry->index];
    entry->in_use = false;
    carray.used--;
}
