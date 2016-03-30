#include "Proto.hpp"
#include "GC.hpp"

using namespace std;

Slot::Slot() {}

Slot::Slot(ObjectPtr ptr) : obj(ptr) {}

SlotType Slot::getType() {
    if (obj.lock()) // TODO Is this right? Should we be comparing obj to NULL instead?
        return SlotType::PTR;
    else
        return SlotType::INH;
}

ObjectPtr Slot::getPtr() {
    if (getType() == SlotType::PTR)
        return obj;
    else
        return shared_ptr<Object>();
}

Slot Object::operator [](string key) {
    auto iter = slots.find(key);
    if (iter == slots.end())
        return Slot();
    else
        return iter->second;
}

void Object::put(std::string key, ObjectPtr ptr) {
    slots.emplace(key, Slot(ptr));
}

set<string> Object::directKeys() {
    set<string> result;
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
    ptr1->put("parent", obj);
    return ptr;
}

ObjectPtr meta(ObjectPtr obj) {
    return getInheritedSlot(obj, "meta");
}

Slot _getInheritedSlot(list<ObjectPtr>& parents, ObjectPtr obj, string name) {
    auto obj1 = obj.lock();
    if (find_if(parents.begin(), parents.end(), [&obj1](auto xx){
                return xx.lock() == obj1;
            }) != parents.end()) {
        return Slot();
    } else {
        Slot check = (*obj1)[name];
        Slot parent = (*obj1)["parent"];
        switch (check.getType()) {
        case SlotType::PTR:
            return check.getPtr();
        case SlotType::INH:
            parents.push_back(obj);
            if (parent.getType() == SlotType::PTR)
                return _getInheritedSlot(parents, parent.getPtr(), name);
            else
                return Slot();
        }
        return Slot();
    }
}

ObjectPtr getInheritedSlot(ObjectPtr obj, string name) {
    list<ObjectPtr> parents;
    auto slot = _getInheritedSlot(parents, obj, name);
    if (slot.getType() == SlotType::PTR)
        return slot.getPtr();
    else
        return ObjectPtr();
}

void _keys(list<ObjectPtr>& parents, set<string>& result, ObjectPtr obj) {
    auto obj1 = obj.lock();
    if (find_if(parents.begin(), parents.end(), [&obj1](auto xx){
                return xx.lock() == obj1;
            }) != parents.end())
        return;
    parents.push_back(obj);
    auto curr = obj.lock()->directKeys();
    result.insert(curr.begin(), curr.end());
    _keys(parents, result, getInheritedSlot(obj, "parent"));
}

set<string> keys(ObjectPtr obj) {
    set<string> result;
    list<ObjectPtr> parents;
    _keys(parents, result, obj);
    return result;
}
