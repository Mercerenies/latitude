#include "Number.hpp"
#include <limits>
#include <cmath>

using namespace std;

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

    struct PowerVisitor : boost::static_visitor<Number::magic_t> {

        template <typename U, typename V>
        U ipow(U base, V exp) const {
            U result(1);
            while (exp) {
                if (exp & 1)
                    result *= base;
                exp >>= 1;
                base *= base;
            }
            return result;
        }

        template <typename U, typename V>
        U pow(U base, V exp) const {
            return ipow(base, exp);
        }

        Number::floating pow(Number::floating base, Number::floating exp) const {
            return std::pow(base, exp);
        }

        template <typename U>
        Number::magic_t operator()(U& first, Number::smallint& second) const {
            // With exponents, assume that a smallint base is going to be a problem since numbers
            // grow so quickly
            auto first0 = Coerce<Number::bigint>::act(first);
            if (second > 0) {
                // If positive, use the simple integer power formula to compute
                // Keep the type of U
                return Number::magic_t(pow(first0, second));
            } else if (second == 0) {
                // If zero, check the first value and either use the floating point NaN or return 1 in type U
                if (first0 == 0)
                    return pow(0.0, 0.0);
                else
                    return Number::magic_t(U(1));
            } else {
                // Reciprocate the base, flip the sign of the exponent, and try again
                RecipVisitor recip;
                NegateVisitor negate;
                Number::magic_t firstx = recip(first0);
                Number::magic_t secondx = negate(second);
                return boost::apply_visitor(*this, firstx, secondx);
            }
        }

        template <typename U>
        Number::magic_t operator()(U& first, Number::bigint& second) const {
            // With exponents, assume that a smallint base is going to be a problem since numbers
            // grow so quickly
            auto first0 = Coerce<Number::bigint>::act(first);
            if (second > 0) {
                // If positive, use the simple integer power formula to compute
                // Keep the type of U
                return Number::magic_t(pow(first0, second));
            } else if (second == 0) {
                // If zero, check the first value and either use the floating point NaN or return 1 in type U
                if (first0 == 0)
                    return pow(0.0, 0.0);
                else
                    return Number::magic_t(U(1));
            } else {
                // Reciprocate the base, flip the sign of the exponent, and try again
                RecipVisitor recip;
                NegateVisitor negate;
                Number::magic_t firstx = recip(first0);
                Number::magic_t secondx = negate(second);
                return boost::apply_visitor(*this, firstx, secondx);
            }
        }

        template <typename U>
        Number::magic_t operator()(U& first, Number::ratio& second) const {
            // If the exponent is a rational, just delegate to the floating point computation
            auto first0 = Coerce<Number::floating>::act(first);
            auto second0 = Coerce<Number::floating>::act(second);
            return Number::magic_t(pow(first0, second0));
        }

        template <typename U>
        Number::magic_t operator()(U& first, Number::floating& second) const {
            // If the exponent is floating, obviously delegate to the floating point computation
            auto first0 = Coerce<Number::floating>::act(first);
            return Number::magic_t(pow(first0, second));
        }

    };

    struct EqualVisitor : boost::static_visitor<bool> {
        template <typename U, typename V>
        bool operator()(const U& first, const V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            return first0 == second0;
        }
    };

    struct LessVisitor : boost::static_visitor<bool> {
        template <typename U, typename V>
        bool operator()(const U& first, const V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            return first0 < second0;
        }
    };

    struct StringifyVisitor : boost::static_visitor<> {
        ostringstream stream;

        StringifyVisitor()
            : stream() {}

        void operator()(const Number::floating& first) {
            stream << fixed << setprecision(2) << first;
        }

        void operator()(const Number::ratio& first) {
            stream << numerator(first) << "/" << denominator(first);
        }

        template <typename U>
        void operator()(const U& first) {
            stream << first;
        }

        string str() {
            return stream.str();
        }
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

    struct FloatingOpVisitor : boost::static_visitor<Number::floating> {
        typedef std::function<Number::floating(Number::floating)> function_type;
        function_type func;

        FloatingOpVisitor(function_type f)
            : func(f) {}

        template <typename U>
        Number::floating operator()(const U& first) const {
            return func(Coerce<Number::floating>::act(first));
        }

    };

};

Number::Number(smallint arg)
    : value(arg) {}

Number::Number(bigint arg)
    : value(arg) {}

Number::Number(ratio arg)
    : value(arg) {}

Number::Number(floating arg)
    : value(arg) {}

bool Number::operator ==(const Number& other) const {
    auto second = other.value;
    return boost::apply_visitor(MagicNumber::EqualVisitor(), value, second);
}

bool Number::operator <(const Number& other) const {
    auto second = other.value;
    return boost::apply_visitor(MagicNumber::LessVisitor(), value, second);
}

Number& Number::operator +=(const Number& other) {
    auto second = other.value;
    if ((value.which() == 0) && (second.which() == 0)) {
        // Possibility of overflow (two smallints)
        bool promote = false;
        smallint x0 = boost::get<smallint>(value);
        smallint x1 = boost::get<smallint>(second);
        if ((x1 > 0) && (x0 > numeric_limits<smallint>::max() - x1))
            promote = true;
        if ((x1 < 0) && (x0 < numeric_limits<smallint>::min() - x1))
            promote = true;
        if (promote)
            value = bigint(x0);
    }
    value = boost::apply_visitor(MagicNumber::PlusVisitor(), value, second);
    return *this;
}

Number& Number::operator -=(const Number& other) {
    return (*this += -other);
}

Number& Number::operator *=(const Number& other) {
    auto second = other.value;
    if ((value.which() == 0) && (second.which() == 0)) {
        // Possibility of overflow (two smallints)
        bool promote = false;
        smallint x0 = boost::get<smallint>(value);
        smallint x1 = boost::get<smallint>(second);
        if ((x1 != 0) && (abs(x0) > numeric_limits<smallint>::max() / abs(x1)))
            promote = true;
        if (promote)
            value = bigint(x0);
    }
    value = boost::apply_visitor(MagicNumber::TimesVisitor(), value, second);
    return *this;
}

Number& Number::operator /=(const Number& other) {
    return (*this *= other.recip());
}

Number& Number::operator %=(const Number& other) {
    auto second = other.value;
    value = boost::apply_visitor(MagicNumber::ModVisitor(), value, second);
    return *this;
}

Number Number::pow(const Number& other) const {
    auto first = this->value;
    auto second = other.value;
    Number result;
    result.value = boost::apply_visitor(MagicNumber::PowerVisitor(), first, second);
    return result;
}

Number Number::operator -() const {
    Number curr = *this;
    curr.value = boost::apply_visitor(MagicNumber::NegateVisitor(), curr.value);
    return curr;
}

Number Number::recip() const {
    Number curr = *this;
    curr.value = boost::apply_visitor(MagicNumber::RecipVisitor(), curr.value);
    return curr;
}

Number Number::sin() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::sin;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::cos() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::cos;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::tan() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::tan;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::sinh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::sinh;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::cosh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::cosh;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::tanh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::tanh;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::exp() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::exp;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::asin() const {
    if ((*this < (smallint)-1) || (*this > (smallint)1))
        return nan();
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::asin;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::acos() const {
    if ((*this < (smallint)-1) || (*this > (smallint)1))
        return nan();
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::acos;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::atan() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::atan;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::asinh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::asinh;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::acosh() const {
    if (*this < (smallint)1)
        return nan();
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::acosh;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::atanh() const {
    // TODO Can we use inf or -inf instead of nan here?
    if ((*this == (smallint)1) || (*this == (smallint)-1))
        return nan();
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::atanh;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

Number Number::log() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::log;
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func), curr.value);
    return curr;
}

string Number::asString() const {
    MagicNumber::StringifyVisitor visitor;
    boost::apply_visitor(visitor, value);
    return visitor.str();
}

auto Number::asSmallInt() const
    -> smallint {
    return boost::apply_visitor(MagicNumber::StrictCastVisitor<smallint>(), value);
}

int Number::hierarchyLevel() const {
    return boost::apply_visitor(MagicNumber::LevelVisitor(), value);
}

Number nan() {
    return Number((Number::smallint)0) / Number((Number::smallint)0);
}
