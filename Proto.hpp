#ifndef _PROTO_HPP_
#define _PROTO_HPP_

#include "Stream.hpp"
#include <list>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <boost/variant.hpp>
#include <boost/blank.hpp>

class Stmt; // From Reader.hpp

class Slot;
class Object;
struct Method;

using LStmt = std::list< std::shared_ptr<Stmt> >;
using ObjectPtr = std::shared_ptr<Object>;
using SystemCall = std::function<ObjectPtr(std::list<ObjectPtr>)>;
using Prim = boost::variant<boost::blank, double, std::string,
                            Method, SystemCall, StreamPtr>;

struct Method {
    ObjectPtr lexicalScope;
    LStmt code;
};

enum class SlotType { PTR, INH };

class Slot {
private:
    ObjectPtr obj;
public:
    Slot();
    Slot(ObjectPtr ptr);
    SlotType getType();
    ObjectPtr getPtr();
};

class Object {
private:
    std::unordered_map<std::string, Slot> slots;
    Prim primitive;
public:
    virtual ~Object() = default;
    virtual Slot operator [](std::string key);
    virtual void put(std::string key, ObjectPtr ptr);
    Prim& prim();
    template <typename T>
    Prim prim(const T& prim0);
};

template <typename T>
Prim Object::prim(const T& prim0) {
    Prim old = primitive;
    primitive = prim0;
    return old;
}

ObjectPtr clone(ObjectPtr obj);
ObjectPtr meta(ObjectPtr obj);
ObjectPtr getInheritedSlot(ObjectPtr obj, std::string name);

#endif // _PROTO_HPP_
