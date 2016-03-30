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

// Returns true on success, false on failure
// The iterator returned by Iterable must be at least a ForwardIterator
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
