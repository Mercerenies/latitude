
#include "Dump.hpp"
#include <boost/variant.hpp>

DebugObject::DebugObject(ObjectPtr ptr)
    : impl(ptr) {}

ObjectPtr DebugObject::get() const {
    return impl;
}

struct DebugPrimVisitor : boost::static_visitor<void> {

    std::ostream& out;

    DebugPrimVisitor(std::ostream& out) : out(out) {}

};

std::ostream& operator <<(std::ostream& out, const DebugObject& obj) {
    // First, we know we can safely print the pointer's numerical value.
    out << obj.get().get();
    // Decide what to do next based on the prim field.
    out << "[";
    DebugPrimVisitor visitor { out };
    //boost::apply_visitor(visitor, obj->prim());
    out << "]";
    // Finish.
    return out;
}
