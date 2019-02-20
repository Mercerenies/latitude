//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "Proto.hpp"
#include "Reader.hpp"
#include "GC.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "Allocator.hpp"
#include <tuple>

using namespace std;

bool PtrCompare::operator()(const ObjectPtr& a, const ObjectPtr& b) {
    return a < b;
}

ObjectHierarchyIterator ObjectHierarchy::begin() {
    return { curr };
}

ObjectHierarchyIterator ObjectHierarchy::end() {
    return { nullptr };
}

bool ObjectHierarchyIterator::operator==(ObjectHierarchyIterator other) {
    return curr == other.curr;
}

bool ObjectHierarchyIterator::operator!=(ObjectHierarchyIterator other) {
    return curr != other.curr;
}

ObjectPtr& ObjectHierarchyIterator::operator*() {
    return curr;
}

ObjectPtr* ObjectHierarchyIterator::operator->() {
    return &curr;
}

ObjectHierarchyIterator& ObjectHierarchyIterator::operator++() {
    ObjectPtr next = (*curr)[ Symbols::parent() ];
    if (next == curr)
        next = nullptr;
    curr = next;
    return *this;
}

ObjectHierarchyIterator ObjectHierarchyIterator::operator++(int) {
    auto self = *this;
    ++*this;
    return self;
}

Slot::Slot(ObjectPtr ptr) noexcept : Slot(ptr, Protection::NO_PROTECTION) {}

Slot::Slot(ObjectPtr ptr, Protection protect) noexcept : obj(ptr), protection(protect) {}

bool Slot::hasObject() const noexcept {
    return (obj != nullptr);
}

Slot* Object::getSlot(Symbolic key) {
    return slots.get(key);
}

const Slot* Object::getSlot(Symbolic key) const {
    return slots.get(key);
}

bool operator==(const Slot& a, const Slot& b) {
    return a.obj == b.obj && a.protection == b.protection;
}

bool operator!=(const Slot& a, const Slot& b) {
    return !(a == b);
}

ObjectPtr Object::operator [](Symbolic key) const {
    const Slot* slot = this->getSlot(key);
    if (slot == nullptr)
        return nullptr;
    return slot->obj;
}

void Object::put(Symbolic key, ObjectPtr ptr) {
    Slot* slot = this->getSlot(key);
    if (slot == nullptr)
        slots.put(key, ptr);
    else
        slot->obj = ptr;
}

void Object::remove(Symbolic key) {
    slots.remove(key);
}

set<Symbolic> Object::directKeys() const {
    set<Symbolic> result;
    for (auto curr : slots)
        result.insert(curr.first);
    return result;
}

bool Object::isProtected(Symbolic key, Protection p) const {
    const Slot* slot = this->getSlot(key);
    Protection p1 = slot ? slot->protection : Protection::NO_PROTECTION;
    return p == (p1 & p);
}

bool Object::hasAnyProtection(Symbolic key) const {
    const Slot* slot = this->getSlot(key);
    Protection p1 = slot ? slot->protection : Protection::NO_PROTECTION;
    return p1 != Protection::NO_PROTECTION;
}

bool Object::addProtection(Symbolic key, Protection p) {
    Slot* slot = this->getSlot(key);
    if (!slot)
        return false;
    slot->protection |= p;
    return true;
}

void Object::protectAll(Protection) {}

Prim& Object::prim() noexcept {
    return primitive;
}

ObjectPtr clone(ObjectPtr obj) {
    ObjectPtr ptr(GC::get().allocate());
    ptr->put(Symbols::parent(), obj);
    ptr->addProtection(Symbols::parent(), Protection::PROTECT_DELETE);
    ptr->prim(obj->prim());
    return ptr;
}

set<Symbolic> keys(ObjectPtr obj) {
    set<Symbolic> result;
    for (ObjectPtr curr : hierarchy(obj)) {
        for (auto elem : curr->directKeys()) {
            result.insert(elem);
        }
    }
    return result;
}

ObjectHierarchy hierarchy(ObjectPtr obj) {
    return { obj };
}

void hereIAm(ObjectPtr dyn, ObjectPtr here) {
    dyn->put(Symbols::get()["$whereAmI"], here);
}
