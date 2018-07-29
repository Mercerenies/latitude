//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#include "GC.hpp"
#include "Allocator.hpp"
#include <stack>
#include <set>
#include <algorithm>

#define GC_PRINT 0

using namespace std;

GC::GC()
    : alloc(), count(TOTAL_COUNT), limit(8192L), tracing(false) {}

GC& GC::get() noexcept {
    return instance;
}

ObjectPtr GC::allocate() {
    ObjectPtr ptr = Allocator::get().allocate();
#if GC_PRINT > 2
    std::cout << "<<Allocating " << ptr << ">>" << std::endl;
#endif
    alloc.insert(ptr.get());
    return ptr;
}

void GC::free(Object* obj) {
    if (alloc.erase(obj) > 0) {
        Allocator::get().free(obj);
    }
}

void GC::freeAll() {
    auto curr = alloc.begin();
    while (curr != alloc.end()) {
        free(*curr);
        curr = alloc.begin();
    }
}

template <typename Container>
void addToFrontier(const Container& visited, Container& frontier, Object* val) {
    if (val == nullptr)
        return;
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
void addSlotsToFrontier(const Container& visited, Container& frontier, Object* curr) {
    for (auto key : curr->directKeys()) {
#if GC_PRINT > 1
        std::cout << "<<Key " << Symbols::get()[key] << ">>" << std::endl;
#endif
        auto val = (*curr)[key];
        addToFrontier(visited, frontier, val.get());
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
        addToFrontier(visited, frontier, fst.get());
        addStackToFrontier(visited, frontier, stack);
        stack.push(fst);
    }
}

template <typename Container>
void addStackToFrontier(const Container& visited, Container& frontier, NodePtr<ObjectPtr> stack) {
    if (stack) {
        auto fst = stack->get();
        addToFrontier(visited, frontier, fst);
        addStackToFrontier(visited, frontier, popNode(stack));
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

template <typename Container, typename T>
void addMapToFrontier(const Container& visited, Container& frontier, map<T, ObjectPtr>& map) {
    for (auto& value : map)
        addToFrontier(visited, frontier, value.second.get());
}

template <typename Container>
void addContinuationToFrontier(const Container& visited, Container& frontier, IntState& state) {
    addStackToFrontier(visited, frontier, state.lex       );
    addStackToFrontier(visited, frontier, state.dyn       );
    addStackToFrontier(visited, frontier, state.arg       );
    addStackToFrontier(visited, frontier, state.sto       );
    addStackToFrontier(visited, frontier, state.hand      );
}

template <typename Container, typename Container1>
void addSeqToFrontier(const Container& visited, Container& frontier, Container1& vec) {
    for (auto& value : vec)
        addToFrontier(visited, frontier, value.get());
}

template <typename Container>
void addContinuationToFrontier(const Container& visited, Container& frontier, const ReadOnlyState& state) {
    addSeqToFrontier  (visited, frontier, state.lit );
}

template <typename Container>
void addContinuationToFrontier(const Container& visited, Container& frontier, TransientState& trans) {
    addToFrontier     (visited, frontier, trans.ptr .get());
    addToFrontier     (visited, frontier, trans.slf .get());
    addToFrontier     (visited, frontier, trans.ret .get());
}

template <typename Container>
void addContinuationToFrontier(const Container& visited, Container& frontier, Object* curr) {
    auto state0 = boost::get<StatePtr>(&curr->prim());
    if (state0) {
        auto state = *state0;
#if GC_PRINT > 1
        std::cout << "<<Freeing continuation " << state << ">>" << std::endl;
#endif
        addContinuationToFrontier(visited, frontier, *state);
    }
}

long GC::garbageCollect(std::vector<Object*> globals) {
#if GC_PRINT > 0
    std::cout << "<<ENTER GC>>" << std::endl;
#endif
    if (tracing) {
        std::cout << "GC: Running.... there are " << getTotal() << " objects in memory." << std::endl;
    }
    std::set<Object*> visited;
    std::set<Object*> frontier;
    for (const auto& elem : globals)
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
    std::set<Object*> result;
    std::set_difference(alloc.begin(), alloc.end(),
                        visited.begin(), visited.end(),
                        inserter(result, result.begin()));
    long total = result.size();
#if GC_PRINT > 0
    std::cout << "<<Set to delete " << result.size() << " objects>>" << std::endl;
#endif
    for (Object* res : result) {
        this->free(res);
    }
#if GC_PRINT > 0
    std::cout << "<<EXIT GC>>" << std::endl;
#endif
    if (tracing) {
        std::cout << "GC: Finished running.... there are now " << getTotal()
                  << " objects." << std::endl;
    }
    return total;
}

long GC::garbageCollect(VMState& vm) {
    // This little type is necessary to make the templated function addContinuationToFrontier
    // think that vectors are set-like.
    struct VectorProxy {
        vector<Object*> value;
        void insert(Object* val) {
            value.push_back(val);
        }
        auto find(Object* val) const -> decltype(value.begin()) {
            return ::find_if(value.begin(), value.end(), [&val](Object* val1){
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
    addContinuationToFrontier<decltype(globals)>({}, globals, vm.state);
    addContinuationToFrontier<decltype(globals)>({}, globals, vm.reader);
    addContinuationToFrontier<decltype(globals)>({}, globals, vm.trans);
    return garbageCollect(globals.value);
}

void GC::setTracing(bool val) {
    tracing = val;
}

size_t GC::getTotal() const {
    return alloc.size();
}

size_t GC::getLimit() const {
    return limit;
}

void GC::tick(VMState& vm) {
    --count;
    if ((count <= 0) && (getTotal() > limit)) {
        garbageCollect(vm);
        count = TOTAL_COUNT;
        if (getTotal() > limit) {
            if (tracing) {
                limit *= 2;
                std::cout << "GC: Increasing limit to " << limit << "." << std::endl;
            }
        }
    }
}
