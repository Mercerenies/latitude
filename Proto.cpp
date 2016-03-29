#include "Proto.hpp"

using namespace std;

Slot::Slot() {}

Slot::Slot(ObjectPtr ptr) : obj(ptr) {}

SlotType Slot::getType() {
    if (obj)
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

Prim& Object::prim() {
    return primitive;
}

ObjectPtr clone(ObjectPtr obj) {
    ObjectPtr ptr(new Object());
    ptr->put("parent", obj);
    return ptr;
}

ObjectPtr meta(ObjectPtr obj) {
    return getInheritedSlot(obj, "meta");
}

Slot _getInheritedSlot(list<ObjectPtr>& parents, ObjectPtr obj, string name) {
    if (find(parents.begin(), parents.end(), obj) != parents.end()) {
        return Slot();
    } else {
        Slot check = (*obj)[name];
        Slot parent = (*obj)["parent"];
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
