#ifndef _MACRO_HPP_
#define _MACRO_HPP_

#include <list>
#include <tuple>
#include <iterator>
#include "Reader.hpp"

template <typename ForwardIterator, typename... Ts>
void _bindArguments(ForwardIterator& begin, ForwardIterator& end, Ts&... args);

template <typename ForwardIterator, typename T, typename... Ts>
void _bindArguments(ForwardIterator& begin, ForwardIterator& end, T& arg, Ts&... args) {
    if (begin == end)
        return;
    arg = (*begin).lock();
    ++begin;
    _bindArguments(begin, end, args...);
}

template <typename ForwardIterator>
void _bindArguments(ForwardIterator& begin, ForwardIterator& end) {}

/*
 * Takes a forward iterable structure and a number of arguments by reference.
 * Binds the arguments to each element of the structure, unless the size does not
 * match, in which case nothing is changed and false is returned. Otherwise,
 * the arguments are bound and true is returned.
 */
template <typename Iterable, typename... Ts>
bool bindArguments(const Iterable& lst, Ts&... args) {
    auto begin = lst.begin();
    auto end = lst.end();
    auto dist = std::distance(begin, end);
    if (dist != sizeof...(args))
        return false;
    else
        return (_bindArguments(begin, end, args...), true);
}

/*
 * Performs a function call given a set of arguments. All of the arguments should be (assignable to) ObjectPtr.
 */
template <typename... Ts>
ObjectPtr doCallWithArgs(ObjectPtr lex, ObjectPtr dyn, ObjectPtr self, ObjectPtr mthd, Ts... args) {
    return doCall(lex, dyn, self, mthd, std::list<ObjectPtr> { args... });
}

#endif // _MACRO_HPP_
