#ifndef _GC_HPP_
#define _GC_HPP_

#include "Proto.hpp"
#include <set>
#include <array>

/*
 * A singleton object representing the global garbage collector. All
 * language `Object` instances should be allocated through this object
 * so that they can be cleaned up through this object.
 */
class GC {
private:
    static GC instance;
    std::set<ObjectSPtr> alloc;
    GC() = default;
public:
    static GC& get();
    /*
     * Creates an object and returns a pointer to it.
     */
    ObjectPtr allocate();
    /*
     * Cleans up objects. Any object references which C++ or the embedded
     * code maintains should be passed in as arguments, as the algorithm
     * will assume anything that is unreachable from the arguments should
     * be freed. As an aside to this, `garbageCollect` called with no
     * arguments will free every object that was created using the garbage
     * collector.
     */
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
        auto stream(outStream());
        visited.insert(*curr);
        frontier.erase(curr);
        if (auto curr1 = curr->lock()) {
            for (auto key : curr1->directKeys()) {
                auto val = getInheritedSlot(curr1, key);
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
