#include "Allocator.hpp"
#include "Proto.hpp"

Allocator Allocator::instance = Allocator();

Allocator& Allocator::get() noexcept {
    return instance;
}

ObjectPtr Allocator::allocate() {
    return new Object();
}

void Allocator::free(ObjectPtr obj) {
    delete obj;
}

