
#include "Protection.hpp"

Protection::Protection(unsigned char impl)
    : internal(impl) {}

const Protection Protection::NO_PROTECTION = Protection(0);
const Protection Protection::PROTECT_ASSIGN = Protection(1);
const Protection Protection::PROTECT_DELETE = Protection(2);

Protection& Protection::operator&=(Protection other) {
    internal &= other.internal;
    return *this;
}

Protection& Protection::operator|=(Protection other) {
    internal |= other.internal;
    return *this;
}

Protection operator&(Protection a, Protection b) {
    return a &= b;
}

Protection operator|(Protection a, Protection b) {
    return a |= b;
}

bool operator==(Protection a, Protection b) {
    return a.internal == b.internal;
}

bool operator!=(Protection a, Protection b) {
    return a.internal != b.internal;
}
