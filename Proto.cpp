#include "Proto.hpp"
#include "Reader.hpp"
#include "GC.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include <tuple>

using namespace std;

Slot::Slot() noexcept {}

Slot::Slot(ObjectPtr ptr) noexcept : obj(ptr) {}

SlotType Slot::getType() const noexcept{
    if (!obj.expired())
        return SlotType::PTR;
    else
        return SlotType::INH;
}

ObjectPtr Slot::getPtr() const {
    if (getType() == SlotType::PTR)
        return obj;
    else
        return ObjectPtr();
}

Slot Object::operator [](Symbolic key) const {
    auto iter = slots.find(key);
    if (iter == slots.end())
        return Slot();
    else
        return iter->second;
}

void Object::put(Symbolic key, ObjectPtr ptr) {
    if (slots.find(key) == slots.end())
        slots.emplace(key, Slot(ptr));
    else
        slots[key] = Slot(ptr);
}

set<Symbolic> Object::directKeys() const {
    set<Symbolic> result;
    for (auto curr : slots)
        result.insert(curr.first);
    return result;
}

Prim& Object::prim() noexcept {
    return primitive;
}

ObjectPtr clone(ObjectPtr obj) {
    ObjectPtr ptr(GC::get().allocate());
    auto ptr1 = ptr.lock();
    ptr1->put(Symbols::get()["parent"], obj);
    ptr1->prim(obj.lock()->prim());
    return ptr;
}

void _keys(list<ObjectPtr>& parents, set<Symbolic>& result, ObjectPtr obj) {
    auto obj1 = obj.lock();
    if (find_if(parents.begin(), parents.end(), [&obj1](auto xx){
                return xx.lock() == obj1;
            }) != parents.end())
        return;
    parents.push_back(obj);
    auto curr = obj.lock()->directKeys();
    for (auto elem : curr)
        result.insert(elem);
    _keys(parents, result, (*obj1)[ Symbols::get()["parent"] ].getPtr());
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
                return obj.lock() == obj1.lock();
            }) == parents.end()) {
        parents.push_back(obj);
        obj = (*obj.lock())[ Symbols::get()["parent"] ].getPtr();
    }
    return parents;
}

void hereIAm(ObjectPtr dyn, ObjectPtr here) {
    dyn.lock()->put(Symbols::get()["$whereAmI"], here);
}
