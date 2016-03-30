#ifndef _GC_HPP_
#define _GC_HPP_

#include "Proto.hpp"
#include <set>
#include <array>

class GC {
private:
    static GC instance;
    std::set<ObjectSPtr> alloc;
    GC() = default;
public:
    static GC& get();
    ObjectPtr allocate();
    template <typename... Ts>
    void garbageCollect(Ts... globals);
};

template <typename... Ts>
void GC::garbageCollect(Ts... globals) {
    struct WeakLess {
        bool operator()(const ObjectPtr& obj0, const ObjectSPtr& obj1) {
            return obj0.lock() < obj1;
        }
        bool operator()(const ObjectSPtr& obj0, const ObjectPtr& obj1) {
            return obj0 < obj1.lock();
        }
        bool operator()(const ObjectPtr& obj0, const ObjectPtr& obj1) {
            return obj0.lock() < obj1.lock();
        }
    };
    std::set<ObjectPtr, WeakLess> visited;
    std::set<ObjectPtr, WeakLess> frontier;
    std::array<ObjectPtr, sizeof...(globals)> args = {globals...};
    for (auto elem : args)
        frontier.insert(elem);
    while (!frontier.empty()) {
        auto curr = frontier.begin();
        visited.insert(*curr);
        frontier.erase(curr);
        if (auto curr1 = curr->lock()) {
            for (auto key : curr1->directKeys()) {
                auto val = getInheritedSlot(*curr, key);
                if (visited.find(val) == visited.end())
                    frontier.insert(val);
            }
        }
    }
    std::set<ObjectSPtr> result;
    std::set_difference(alloc.begin(), alloc.end(),
                        visited.begin(), visited.end(),
                        inserter(result, result.begin()),
                        WeakLess());
    for (ObjectSPtr res : result) {
        alloc.erase(res);
        res.reset();
    }
}

#endif // _GC_HPP_
