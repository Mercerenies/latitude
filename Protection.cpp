
#include "Protection.hpp"

Protection::Protection(unsigned char impl)
    : internal(impl) {}

Protection Protection::NO_PROTECTION = Protection(0);
Protection Protection::PROTECT_ASSIGN = Protection(1);
Protection Protection::PROTECT_DELETE = Protection(2);

Protection operator&(Protection a, Protection b) {
    return Protection(a.internal & b.internal);
}

Protection operator|(Protection a, Protection b) {
    return Protection(a.internal | b.internal);
}

bool operator==(Protection a, Protection b) {
    return a.internal == b.internal;
}

bool operator!=(Protection a, Protection b) {
    return a.internal != b.internal;
}
