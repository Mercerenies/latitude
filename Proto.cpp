#include "Proto.hpp"
#include "Reader.hpp"
#include "GC.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include <tuple>

using namespace std;

ObjectPtr::ObjectPtr() : impl(nullptr) {}

ObjectPtr::ObjectPtr(Object* ptr) : impl(ptr) {}

Object& ObjectPtr::operator*() const {
    return *impl;
}

Object* ObjectPtr::operator->() const {
    return impl;
}

Object* ObjectPtr::get() const {
    return impl;
}

bool ObjectPtr::operator==(const ObjectPtr& ptr) const {
    return this->impl == ptr.impl;
}

bool ObjectPtr::operator!=(const ObjectPtr& ptr) const {
    return this->impl != ptr.impl;
}

bool ObjectPtr::operator<(const ObjectPtr& ptr) const {
    return this->impl < ptr.impl;
}

Slot::Slot() noexcept : Slot(nullptr, false) {}

Slot::Slot(ObjectPtr ptr) noexcept : Slot(ptr, NO_PROTECTION) {}

Slot::Slot(ObjectPtr ptr, protection_t protect) noexcept : obj(ptr), protection(protect) {}

SlotType Slot::getType() const noexcept {
    if (obj != nullptr)
        return SlotType::PTR;
    else
        return SlotType::INH;
}

ObjectPtr Slot::getPtr() const {
    if (getType() == SlotType::PTR)
        return obj;
    else
        return nullptr;
}

void Slot::putPtr(ObjectPtr ptr) {
    obj = ptr;
}

void Slot::addProtection(protection_t p) noexcept {
    protection |= p;
}

bool Slot::isProtected(protection_t p) const noexcept {
    return p == (protection & p);
}

bool Slot::hasAnyProtection() const noexcept {
    return (protection != NO_PROTECTION);
}

Slot Object::operator [](Symbolic key) const {
    auto iter = slots.find(key);
    if (iter == slots.end())
        return Slot();
    else
        return iter->second;
}

void Object::put(Symbolic key, ObjectPtr ptr) {
    auto iter = slots.find(key);
    if (iter == slots.end())
        slots[key] = Slot(ptr);
    else
        iter->second.putPtr(ptr);
}

void Object::remove(Symbolic key) {
    slots.erase(key);
}

set<Symbolic> Object::directKeys() const {
    set<Symbolic> result;
    for (auto curr : slots)
        result.insert(curr.first);
    return result;
}

bool Object::isProtected(Symbolic key, protection_t p) const {
    return (*this)[key].isProtected(p);
}

bool Object::hasAnyProtection(Symbolic key) const {
    return (*this)[key].hasAnyProtection();
}

void Object::addProtection(Symbolic key, protection_t p) {
    auto iter = slots.find(key);
    if (iter == slots.end())
        slots.emplace(key, Slot(nullptr, p)); // <- Issue #16 null pointer is introduced here
    else
        iter->second.addProtection(p);
}

void Object::protectAll(protection_t) {}

Prim& Object::prim() noexcept {
    return primitive;
}

ObjectPtr clone(ObjectPtr obj) {
    ObjectPtr ptr(GC::get().allocate());
    ptr->put(Symbols::get()["parent"], obj);
    ptr->addProtection(Symbols::get()["parent"], PROTECT_DELETE);
    ptr->prim(obj->prim());
    return ptr;
}

void _keys(list<ObjectPtr>& parents, set<Symbolic>& result, ObjectPtr obj) {
    if (find_if(parents.begin(), parents.end(), [&obj](auto xx){
                return xx == obj;
            }) != parents.end())
        return;
    parents.push_back(obj);
    auto curr = obj->directKeys();
    for (auto elem : curr)
        result.insert(elem);
    _keys(parents, result, (*obj)[ Symbols::get()["parent"] ].getPtr());
}

set<Symbolic> keys(ObjectPtr obj) {
    set<Symbolic> result;
    list<ObjectPtr> parents;
    _keys(parents, result, obj);
    return result;
}

list<ObjectPtr> hierarchy(ObjectPtr obj) {
    list<ObjectPtr> parents;
    while (find_if(parents.begin(), parents.end(), [&obj](auto obj1) {
                return obj == obj1;
            }) == parents.end()) {
        parents.push_back(obj);
        obj = (*obj)[ Symbols::get()["parent"] ].getPtr();
    }
    return parents;
}

void hereIAm(ObjectPtr dyn, ObjectPtr here) {
    dyn->put(Symbols::get()["$whereAmI"], here);
}
