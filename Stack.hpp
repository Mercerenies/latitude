#ifndef STACK_HPP
#define STACK_HPP

/// \file
/// \brief StackNode and NodePtr, as well as the helper methods that
/// operate on them.

template <typename T>
class StackNode;

/// A NodePtr is a smart pointer to a StackNode.
template <typename T>
using NodePtr = std::shared_ptr<StackNode<T>>;

/// A home-baked linked stack implementation which allows sharing of
/// data between continuations for efficiency. StackNode sacrifices
/// spatial locality for the ability to share common tails of
/// stacks. StackNode instances are immutable, in the sense that push
/// and pop operations will always return new StackNode instances, so
/// these objects can be shared across multiple data structures. Note
/// that StackNode is designed to be wrapped in a NodePtr, so that
/// `nullptr` designates an empty stack. Thus, a StackNode always
/// consists of exactly one element and a pointer to the rest of the
/// stack.
///
/// \tparam T the type of elements in the stack
template <typename T>
class StackNode {
private:
    NodePtr<T> next;
    T data;
public:
    /// Constructs a stack node containing the single element
    /// given. The resulting stack will have exactly one element in
    /// it.
    ///
    /// \param data0 the element to place on the stack
    StackNode(const T& data0);
    /// Returns a reference to the element on the stack.
    ///
    /// \return a reference to the stack element
    const T& get();
    template <typename S>
    friend NodePtr<S> pushNode(NodePtr<S> node, const S& data);
    template <typename S>
    friend NodePtr<S> popNode(NodePtr<S> node);
};

/// Adds an element to the stack, returning a modified stack.
///
/// \tparam T the type of stack elements
/// \param node a stack
/// \param data the element to push
/// \return the new stack
template <typename T>
NodePtr<T> pushNode(NodePtr<T> node, const T& data);

/// Pops an element off the stack, returning a modified stack.
///
/// \pre The stack must be nonempty; a `nullptr` argument will result
/// in undefined behavior
/// \tparam T the type of stack elements
/// \param node a stack
/// \return the new stack
/// \see StackNode::get()
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

#endif // STACK_HPP
