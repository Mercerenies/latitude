#include "Proto.hpp"
#include "Reader.hpp"
#include "GC.hpp"
#include "Standard.hpp"
#include "Garnish.hpp"
#include <tuple>

using namespace std;

Slot::Slot() {}

Slot::Slot(ObjectPtr ptr) : obj(ptr) {}

SlotType Slot::getType() {
    if (!obj.expired())
        return SlotType::PTR;
    else
        return SlotType::INH;
}

ObjectPtr Slot::getPtr() {
    if (getType() == SlotType::PTR)
        return obj;
    else
        return ObjectPtr();
}

Slot Object::operator [](Symbolic key) {
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

set<Symbolic> Object::directKeys() {
    set<Symbolic> result;
    for (auto curr : slots)
        result.insert(curr.first);
    return result;
}

Prim& Object::prim() {
    return primitive;
}

ObjectPtr clone(ObjectPtr obj) {
    ObjectPtr ptr(GC::get().allocate());
    auto ptr1 = ptr.lock();
    ptr1->put(Symbols::get()["parent"], obj);
    return ptr;
}

ObjectPtr meta(ObjectPtr dyn, ObjectPtr obj) {
    return getInheritedSlot(dyn, obj, Symbols::get()["meta"]);
}

auto _getInheritedSlot(list<ObjectPtr>& parents, ObjectPtr obj, Symbolic name)
    -> tuple<Slot, ObjectPtr> {
    auto obj1 = obj.lock();
    if (find_if(parents.begin(), parents.end(), [&obj1](auto xx){
                return xx.lock() == obj1;
            }) != parents.end()) {
        return make_tuple(Slot(), ObjectPtr());
    } else {
        Slot check = (*obj1)[name];
        Slot parent = (*obj1)[ Symbols::get()["parent"] ];
        switch (check.getType()) {
        case SlotType::PTR:
            return make_tuple(check.getPtr(), obj1);
        case SlotType::INH:
            parents.push_back(obj);
            if (parent.getType() == SlotType::PTR)
                return _getInheritedSlot(parents, parent.getPtr(), name);
            else
                return make_tuple(Slot(), ObjectPtr());
        }
        return make_tuple(Slot(), ObjectPtr());
    }
}

///// Think about it
auto _tryToGetSlot(ObjectPtr dyn, ObjectPtr obj, Symbolic name)
    -> tuple<Slot, ObjectPtr> {
    list<ObjectPtr> parents;
    auto result = _getInheritedSlot(parents, obj, name);
    auto slot = get<0>(result);
    if (slot.getType() == SlotType::PTR) {
        return result;
    } else {
        parents.clear();
        auto result1 = _getInheritedSlot(parents, obj, Symbols::get()["missing"]);
        auto slot1 = get<0>(result1);
        if (slot1.getType() == SlotType::PTR) {
            ObjectPtr dyn1 = clone(dyn);
            dyn1.lock()->put(Symbols::get()["$1"], garnish(dyn, name)); // TODO Change to lex, not dyn
            auto mthdResult = callMethod(obj.lock(), slot1.getPtr(), dyn1);
            return make_tuple(mthdResult, get<1>(result1));
        } else {
            return result1;
        }
    }
}

ObjectPtr getInheritedSlot(ObjectPtr dyn, ObjectPtr obj, Symbolic name) {
    auto slot = get<0>(_tryToGetSlot(dyn, obj, name));
    if (slot.getType() == SlotType::PTR) {
        return slot.getPtr();
    } else {
        throw doSlotError(obj, obj, name);
    }
}

bool hasInheritedSlot(ObjectPtr dyn, ObjectPtr obj, Symbolic name) {
    auto slot = get<0>(_tryToGetSlot(dyn, obj, name));
    return slot.getType() == SlotType::PTR;
}

ObjectPtr getInheritedOrigin(ObjectPtr dyn, ObjectPtr obj, Symbolic name) {
    auto origin = get<1>(_tryToGetSlot(dyn, obj, name));
    if (!origin.expired())
        return origin;
    else
        throw doSlotError(obj, obj, name);
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
