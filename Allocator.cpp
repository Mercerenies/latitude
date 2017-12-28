
#include "Allocator.hpp"
#include "GC.hpp"
#include "Proto.hpp"

constexpr size_t START_CAP = 10;
constexpr size_t BUCKET_SIZE = 100;

CountedArray::CountedArray() :
    used(0), array(BUCKET_SIZE, ObjectEntry()) {}

Allocator Allocator::instance = Allocator();

Allocator::Allocator() : vec() {
    vec.reserve(START_CAP);
}

Allocator::~Allocator() {
    GC::get().freeAll();
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
                    entry.ref_count = 0;
                    entry.object = Object();
                    return ObjectPtr(&entry.object);
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

void Allocator::free(Object* obj) {
    ObjectEntry* entry = reinterpret_cast<ObjectEntry*>(obj);
    CountedArray& carray = vec[entry->index];
    entry->in_use = false;
    entry->object = Object(); // TODO This is probably slowing the GC down; can we make it more efficient?
    carray.used--;
}
