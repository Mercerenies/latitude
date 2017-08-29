#ifndef _MACRO_HPP_
#define _MACRO_HPP_

#include <list>
#include <tuple>
#include <iterator>
#include <type_traits>
#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/find.hpp>
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
 *
template <typename... Ts>
ObjectPtr doCallWithArgs(Scope scope, ObjectPtr self, ObjectPtr mthd, Ts... args) {
    return doCall(scope, self, mthd, std::list<ObjectPtr> { args... });
}
*/

template <typename T>
struct _VariantVisitor {
    template <typename S>
    bool operator()(const S&) const {
        return std::is_same<T, S>::value;
    }
};

template <typename T, typename... Ss>
bool variantIsType(const boost::variant<Ss...>& variant) {
    typedef boost::mpl::vector<Ss...> pack_t;
    static_assert(!std::is_same<
                  typename boost::mpl::find<pack_t, T>::type,
                  typename boost::mpl::end<pack_t>::type>::value,
                  "invalid type given to variant check");
    return boost::apply_visitor(_VariantVisitor<T>(), variant);
}

#endif // _MACRO_HPP_
