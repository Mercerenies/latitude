#include "GC.hpp"
#include <stack>
#include <set>
#include <algorithm>

#define GC_PRINT 0

using namespace std;

GC GC::instance = GC();

GC& GC::get() noexcept {
    return instance;
}

ObjectPtr GC::allocate() {
    ObjectSPtr ptr = new Object();
#if GC_PRINT > 2
    std::cout << "<<Allocating " << ptr << ">>" << std::endl;
#endif
    alloc.insert(ptr);
    return ptr;
}

template <typename Container>
void addToFrontier(const Container& visited, Container& frontier, ObjectPtr val) {
    if (visited.find(val) == visited.end()) {
#if GC_PRINT > 1
        std::cout << "<<Inserting " << val << ">>" << std::endl;
#endif
        frontier.insert(val);
#if GC_PRINT > 2
        std::cout << "<<Inserted " << val << ">>" << std::endl;
#endif
    }
}

template <typename Container>
void addSlotsToFrontier(const Container& visited, Container& frontier, ObjectSPtr curr) {
    for (auto key : curr->directKeys()) {
#if GC_PRINT > 1
        std::cout << "<<Key " << Symbols::get()[key] << ">>" << std::endl;
#endif
        auto val = (*curr)[key].getPtr();
        addToFrontier(visited, frontier, val);
    }
#if GC_PRINT > 1
    std::cout << "<<End keys>>" << std::endl;
#endif
}

template <typename Container>
void addStackToFrontier(const Container& visited, Container& frontier, stack<ObjectPtr>& stack) {
    if (!stack.empty()) {
        auto fst = stack.top();
        stack.pop();
        addToFrontier(visited, frontier, fst);
        addStackToFrontier(visited, frontier, stack);
        stack.push(fst);
    }
}

template <typename Container>
void addWindToFrontier(const Container& visited, Container& frontier, stack<WindPtr>& stack) {
    if (!stack.empty()) {
        auto fst = stack.top();
        stack.pop();
        addToFrontier(visited, frontier, fst->before.lex);
        addToFrontier(visited, frontier, fst->before.dyn);
        addToFrontier(visited, frontier, fst->after.lex);
        addToFrontier(visited, frontier, fst->after.dyn);
        addWindToFrontier(visited, frontier, stack);
        stack.push(fst);
    }
}

template <typename Container>
void addContinuationToFrontier(const Container& visited, Container& frontier, IntState& state) {
    addToFrontier(visited, frontier, state.ptr);
    addToFrontier(visited, frontier, state.slf);
    addToFrontier(visited, frontier, state.ret);
    addStackToFrontier(visited, frontier, state.lex);
    addStackToFrontier(visited, frontier, state.dyn);
    addStackToFrontier(visited, frontier, state.arg);
    addStackToFrontier(visited, frontier, state.sto);
    addStackToFrontier(visited, frontier, state.hand);
}

template <typename Container>
void addContinuationToFrontier(const Container& visited, Container& frontier, ObjectSPtr curr) {
    auto state0 = boost::get<StatePtr>(&curr->prim());
    if (state0) {
        auto state = *state0;
#if GC_PRINT > 1
        std::cout << "<<Freeing continuation " << state << ">>" << std::endl;
#endif
        addContinuationToFrontier(visited, frontier, *state);
    }
}

long GC::garbageCollect(std::vector<ObjectPtr> globals) {
#if GC_PRINT > 0
    std::cout << "<<ENTER GC>>" << std::endl;
#endif
    /* // This shouldn't be necessary anymore since we're using the standard lt operator
    struct WeakLess {
        bool operator()(const ObjectPtr& obj0, const ObjectPtr& obj1) const noexcept {
            return obj0 < obj1;
        }
    };
    */
    std::set<ObjectPtr> visited;
    std::set<ObjectPtr> frontier;
    for (auto elem : globals)
        frontier.insert(elem);
    while (!frontier.empty()) {
        auto curr = *(frontier.begin());
        auto stream(outStream());
        visited.insert(curr);
        frontier.erase(curr);
#if GC_PRINT > 1
        std::cout << "<<Got lock on " << curr << ">>" << std::endl;
#endif
        assert(alloc.find(curr) != alloc.end());
#if GC_PRINT > 2
        std::cout << "<<Add slots on locked>>" << std::endl;
#endif
        addSlotsToFrontier(visited, frontier, curr);
#if GC_PRINT > 2
        std::cout << "<<Add continuation on locked>>" << std::endl;
#endif
        addContinuationToFrontier(visited, frontier, curr);
#if GC_PRINT > 2
        std::cout << "<<Finished with locked>>" << std::endl;
#endif
    }
#if GC_PRINT > 0
    std::cout << "<<Done gathering>>" << std::endl;
#endif
    std::set<ObjectSPtr> result;
    std::set_difference(alloc.begin(), alloc.end(),
                        visited.begin(), visited.end(),
                        inserter(result, result.begin()));
    long total = result.size();
#if GC_PRINT > 0
    std::cout << "<<Set to delete " << result.size() << " objects>>" << std::endl;
#endif
    for (ObjectSPtr res : result) {
        alloc.erase(res);
        delete res;
    }
#if GC_PRINT > 0
    std::cout << "<<EXIT GC>>" << std::endl;
#endif
    return total;
}

long GC::garbageCollect(IntState& state) {
    // This little type is necessary to make the templated function addContinuationToFrontier
    // think that vectors are set-like.
    struct VectorProxy {
        vector<ObjectPtr> value;
        void insert(ObjectPtr val) {
            value.push_back(val);
        }
        auto find(ObjectPtr val) const -> decltype(value.begin()) {
            return ::find_if(value.begin(), value.end(), [&val](ObjectPtr val1){
                    return val == val1;
                });
        }
        auto begin() const -> decltype(value.begin()) {
            return value.begin();
        }
        auto end() const -> decltype(value.end()) {
            return value.end();
        }
    };
    VectorProxy globals;
    addContinuationToFrontier<decltype(globals)>({}, globals, state);
    return garbageCollect(globals.value);
}

size_t GC::getTotal() {
    return alloc.size();
}
