#ifndef _NUMBER_HPP_
#define _NUMBER_HPP_

#include <ios>
#include <string>
#include <type_traits>
#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/size.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/operators.hpp>

class Number : private boost::operators<Number> {
public:
    typedef long smallint;
    typedef boost::multiprecision::cpp_int bigint;
    typedef boost::multiprecision::cpp_rational ratio;
    typedef double floating;
    typedef boost::variant<smallint, bigint, ratio, floating> magic_t;
private:
    magic_t value;
public:
    Number() = default;
    Number(const Number&) = default;
    Number(smallint);
    Number(bigint);
    Number(ratio);
    Number(floating);
    bool operator ==(const Number& other);
    bool operator <(const Number& other);
    Number& operator +=(const Number& other);
    Number& operator -=(const Number& other);
    Number& operator *=(const Number& other);
    Number& operator /=(const Number& other);
    Number& operator %=(const Number& other);
    Number operator -() const;
    Number recip() const;
    std::string asString() const;
    smallint asSmallInt() const;
    int hierarchyLevel() const;
};

namespace MagicNumber {
    using namespace boost;
    using namespace boost::mpl;
    using namespace std;

    typedef mpl::vector<Number::smallint,
                        Number::bigint,
                        Number::ratio,
                        Number::floating> number_hierarchy_t;

    template <typename T>
    struct NumeralT {
        typedef typename std::enable_if<
            mpl::distance<
                mpl::begin<number_hierarchy_t>::type,
                typename mpl::find<number_hierarchy_t, T>::type
                >::type::value !=
                    mpl::size<number_hierarchy_t>::type::value,
            typename distance<
                mpl::begin<number_hierarchy_t>::type,
                typename mpl::find<number_hierarchy_t, T>::type
                >::type >::type type;
    };

    template <int N>
    struct NumeralN {
        typedef typename at<
            number_hierarchy_t,
            boost::integral_constant<int, N>
            >::type type;
    };

    template <typename U, typename V>
    struct Wider {
        typedef typename boost::conditional<
            ((NumeralT<U>::type::value) > (NumeralT<V>::type::value)),
            U,
            V>::type type;
    };

    template <typename U, typename V>
    struct Narrower {
        typedef typename boost::conditional<
            ((NumeralT<U>::type::value) > (NumeralT<V>::type::value)),
            V,
            U>::type type;
    };

    template <typename U>
    struct Coerce {
        template <typename V>
        static typename Wider<U, V>::type act(V value) {
            return static_cast< typename Wider<U, V>::type >(value);
        }
    };

    struct PlusVisitor : boost::static_visitor<Number::magic_t> {
        template <typename U, typename V>
        Number::magic_t operator()(U& first, V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            typename Wider<U, V>::type result = first0 + second0;
            return Number::magic_t(result);
        }
    };

    struct TimesVisitor : boost::static_visitor<Number::magic_t> {
        template <typename U, typename V>
        Number::magic_t operator()(U& first, V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            typename Wider<U, V>::type result = first0 * second0;
            return Number::magic_t(result);
        }
    };

    struct NegateVisitor : boost::static_visitor<Number::magic_t> {
        template <typename U>
        Number::magic_t operator()(U& first) const {
            U result = - first;
            return Number::magic_t(result);
        }
    };

    struct RecipVisitor : boost::static_visitor<Number::magic_t> {
        template <typename U>
        Number::magic_t operator()(U& first) const {
            if (first == 0)
                return Number::magic_t(1.0 / Coerce<Number::floating>::act(first));
            auto first0 = Coerce<Number::ratio>::act(first);
            typename Wider<U, Number::ratio>::type result = 1 / first0;
            return Number::magic_t(result);
        }
    };

    struct ModVisitor : boost::static_visitor<Number::magic_t> {
        template <typename U, typename V>
        Number::magic_t operator()(U& first, V& second) const {
            if (second == 0)
                return fmod(Coerce<Number::floating>::act(first),
                            Coerce<Number::floating>::act(second));
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            typename Wider<U, V>::type result =
                first0 - (int)(first0 / second0) * second0;
            return Number::magic_t(result);
        }
    };

    struct EqualVisitor : boost::static_visitor<bool> {
        template <typename U, typename V>
        bool operator()(U& first, V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            return first0 == second0;
        }
    };

    struct LessVisitor : boost::static_visitor<bool> {
        template <typename U, typename V>
        bool operator()(U& first, V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            return first0 < second0;
        }
    };

    struct StringifyVisitor : boost::static_visitor<> {
        ostringstream stream;

        StringifyVisitor();

        void operator()(const double& first);

        template <typename U>
        void operator()(const U& first) {
            stream << first;
        }

        std::string str();
    };

    struct LevelVisitor : boost::static_visitor<int> {
        template <typename U>
        int operator()(const U& first) const {
            return (int)NumeralT<U>::type::value;
        }
    };

    template <typename T>
    struct StrictCastVisitor : boost::static_visitor<T> {
        template <typename U>
        T operator()(const U& first) const {
            return (T)first;
        }
    };

};

#endif // _NUMBER_HPP_
