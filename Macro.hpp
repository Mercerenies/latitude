#ifndef _MACRO_HPP_
#define _MACRO_HPP_

#include <tuple>
#include <iterator>

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

#endif // _MACRO_HPP_
