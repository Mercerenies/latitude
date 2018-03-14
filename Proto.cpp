#include "Proto.hpp"
#include "Reader.hpp"
#include "GC.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include "Allocator.hpp"
#include <tuple>

using namespace std;

void ObjectPtr::up() {
    if (impl != nullptr) {
        ObjectEntry* ptr = reinterpret_cast<ObjectEntry*>(impl);
        ptr->ref_count++;
    }
}

void ObjectPtr::down() {
    if (impl != nullptr) {
        ObjectEntry* ptr = reinterpret_cast<ObjectEntry*>(impl);
        assert(ptr->ref_count > 0);
        ptr->ref_count--;
        if (ptr->ref_count == 0) {
            //std::cout << "<FREE " << impl << ">" << std::endl;
            GC::get().free(impl);
        }
    }
}

ObjectPtr::ObjectPtr() : impl(nullptr) {}

ObjectPtr::ObjectPtr(nullptr_t) : ObjectPtr() {}

ObjectPtr::ObjectPtr(Object* ptr) : impl(ptr) {
    up();
}

ObjectPtr::ObjectPtr(const ObjectPtr& ptr) : impl(ptr.impl) {
    up();
}

ObjectPtr::ObjectPtr(ObjectPtr&& ptr) : impl(ptr.impl) {
    ptr.impl = nullptr;
    // Created and destroyed a reference; leave it alone
}

ObjectPtr::~ObjectPtr() {
    down();
}

ObjectPtr& ObjectPtr::operator=(const ObjectPtr& ptr) {
    down();
    impl = ptr.impl;
    up();
    return *this;
}

ObjectPtr& ObjectPtr::operator=(ObjectPtr&& ptr) {
    down();
    impl = ptr.impl;
    ptr.impl = nullptr;
    // Created and destroyed a reference; leave it alone
    return *this;
}

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

Slot::Slot(ObjectPtr ptr) noexcept : Slot(ptr, NO_PROTECTION) {}

Slot::Slot(ObjectPtr ptr, protection_t protect) noexcept : obj(ptr), protection(protect) {}

bool Slot::hasObject() const noexcept {
    return (obj != nullptr);
}

Slot Object::getSlot(Symbolic key) const {
    auto iter = slots.find(key);
    if (iter == slots.end())
        return Slot();
    else
        return iter->second;
}

ObjectPtr Object::operator [](Symbolic key) const {
    return this->getSlot(key).obj;
}

void Object::put(Symbolic key, ObjectPtr ptr) {
    auto iter = slots.find(key);
    if (iter == slots.end())
        slots[key] = Slot(ptr);
    else
        iter->second.obj = ptr;
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
    protection_t p1 = this->getSlot(key).protection;
    return p == (p1 & p);
}

bool Object::hasAnyProtection(Symbolic key) const {
    return this->getSlot(key).protection != NO_PROTECTION;
}

bool Object::addProtection(Symbolic key, protection_t p) {
    auto iter = slots.find(key);
    if (iter == slots.end())
        return false;
    iter->second.protection |= p;
    return true;
}

void Object::protectAll(protection_t) {}

Prim& Object::prim() noexcept {
    return primitive;
}

ObjectPtr clone(ObjectPtr obj) {
    ObjectPtr ptr(GC::get().allocate());
    ptr->put(Symbols::parent(), obj);
    ptr->addProtection(Symbols::parent(), PROTECT_DELETE);
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
    _keys(parents, result, (*obj)[ Symbols::parent() ]);
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
        obj = (*obj)[ Symbols::parent() ];
    }
    return parents;
}

void hereIAm(ObjectPtr dyn, ObjectPtr here) {
    dyn->put(Symbols::get()["$whereAmI"], here);
}
