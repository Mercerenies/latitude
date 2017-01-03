#include "Proto.hpp"
#include "Reader.hpp"
#include "GC.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include "Macro.hpp"
#include <tuple>

using namespace std;

Slot::Slot() noexcept : obj(nullptr) {}

Slot::Slot(ObjectPtr ptr) noexcept : obj(ptr) {}

SlotType Slot::getType() const noexcept{
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
    ptr->put(Symbols::get()["parent"], obj);
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
