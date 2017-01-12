#ifndef _STACK_HPP_
#define _STACK_HPP_

template <typename T>
class StackNode;

template <typename T>
using NodePtr = std::shared_ptr<StackNode<T>>;

// A home-baked stack implementation which allows sharing of data between continuations for efficiency
template <typename T>
class StackNode {
private:
    NodePtr<T> next;
    T data;
public:
    StackNode(const T& data0);
    const T& get();
    template <typename S>
    friend NodePtr<S> pushNode(NodePtr<S> node, const S& data);
    template <typename S>
    friend NodePtr<S> popNode(NodePtr<S> node);
};

template <typename T>
NodePtr<T> pushNode(NodePtr<T> node, const T& data);

template <typename T>
NodePtr<T> popNode(NodePtr<T> node);

template <typename T>
StackNode<T>::StackNode(const T& data0)
    : next(nullptr), data(data0) {}

template <typename T>
const T& StackNode<T>::get() {
    return data;
}

template <typename T>
NodePtr<T> pushNode(NodePtr<T> node, const T& data) {
    NodePtr<T> ptr(new StackNode<T>(data));
    ptr->next = node;
    return ptr;
}

template <typename T>
NodePtr<T> popNode(NodePtr<T> node) {
    return node->next;
}

#endif // _STACK_HPP_
