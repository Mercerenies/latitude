#include "Cont.hpp"

Signal::Signal(Symbolic id, ObjectPtr obj) noexcept
    : identifier(id) , object(obj) {}

bool Signal::match(Symbolic other) const noexcept {
    return (other == identifier);
}

ObjectPtr Signal::getObject() const noexcept {
    return object;
}

ProtoError::ProtoError(ObjectPtr obj) noexcept
    : object(obj) {}

ObjectPtr ProtoError::getObject() const noexcept {
    return object;
}

[[ noreturn ]] void throwProtoError(const ObjectPtr& obj) {
    throw ProtoError(obj);
}
