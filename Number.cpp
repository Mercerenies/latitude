#include "Number.hpp"
#include <limits>
#include <cmath>
#include <complex>

using namespace std;

namespace MagicNumber {
    using namespace boost;
    using namespace boost::mpl;
    using namespace std;

    typedef mpl::vector<Number::smallint,
                        Number::bigint,
                        Number::ratio,
                        Number::floating,
                        Number::complex> number_hierarchy_t;

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

    template <>
    struct Coerce<Number::complex> {
        template <typename V>
        using result_type = typename std::enable_if<
            std::is_same<typename Wider<Number::complex, V>::type,
                         Number::complex>::value,
            Number::complex>::type;
        template <typename V>
        using nresult_type = typename std::enable_if<
            !std::is_same<typename Wider<Number::complex, V>::type,
                          Number::complex>::value,
            typename Wider<Number::complex, V>::type>::type;
        template <typename V>
        static result_type<V> act(V value) {
            return Number::complex((double)value, 0);
        }
        template <typename V>
        static nresult_type<V> act(V value) {
            return static_cast< typename Wider<Number::complex, V>::type >(value);
        }
        static Number::complex act(Number::complex value) {
            return value;
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
        T operator()(const Number::complex& first) const {
            return (T)real(first);
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
        Number::magic_t operator()(Number::complex& first) const {
            return Number::magic_t(1.0 / first);
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
        Number::magic_t operator()(Number::complex& first, Number::complex& second) const {
            return Number::magic_t(Number::complex(0, 0));
        }
        template <typename U>
        Number::magic_t operator()(U& first, Number::complex& second) const {
            return Number::magic_t(Number::complex(0, 0));
        }
        template <typename V>
        Number::magic_t operator()(Number::complex& first, V& second) const {
            return Number::magic_t(Number::complex(0, 0));
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
        Number::complex pow(U base, Number::complex exp) const {
            return std::pow(Coerce<Number::complex>::act(base), exp);
        }

        template <typename V>
        Number::complex pow(Number::complex base, V exp) const {
            return std::pow(base, Coerce<Number::complex>::act(exp));
        }

        Number::complex pow(Number::complex base, Number::complex exp) const {
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
                if (first0 == Coerce<U>::act((Number::smallint)0))
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
                if (first0 == Coerce<U>::act((Number::smallint)0))
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
            // If the exponent is a ratio, treat it as though the exponent were a floating point number
            auto first0 = Coerce<Number::floating>::act(first);
            auto second0 = Coerce<Number::floating>::act(second);
            return (*this)(first0, second0);
        }

        template <typename U>
        Number::magic_t operator()(U& first, Number::floating& second) const {
            // If the exponent is a floating point, check the sign of the base
            auto first0 = Coerce<Number::floating>::act(first);
            auto second0 = Coerce<Number::floating>::act(second);
            if (abs(first0) == StrictCastVisitor<Number::floating>()(first0)) {
                // In the nonnegative case, delegate to floating point
                return Number::magic_t(pow(first0, second0));
            } else {
                // In the negative case, delegate to complex
                auto first1 = Coerce<Number::complex>::act(first0);
                auto second1 = Coerce<Number::complex>::act(second0);
                return (*this)(first1, second1);
            }
        }

        template <typename U>
        Number::magic_t operator()(U& first, Number::complex& second) const {
            // If the exponent is complex, delegate to the complex exponential function
            auto first0 = Coerce<Number::complex>::act(first);
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
        bool operator()(const Number::complex& first, const Number::complex& second) const {
            return false;
        }
        template <typename V>
        bool operator()(const Number::complex& first, const V& second) const {
            return false;
        }
        template <typename U>
        bool operator()(const U& first, const Number::complex& second) const {
            return false;
        }
    };

    struct StringifyVisitor : boost::static_visitor<> {
        ostringstream stream;

        StringifyVisitor()
            : stream() {}

        void operator()(const Number::floating& first) {
            if (isfinite(first)) {
                stream << fixed << setprecision(2) << first;
            } else {
                if (first > 0)
                    stream << "inf";
                else if (first < 0)
                    stream << "ninf";
                else
                    stream << "nan";
            }
        }

        void operator()(const Number::ratio& first) {
            stream << "(" << numerator(first) << " / " << denominator(first) << ")";
        }

        void operator()(const Number::complex& first) {
            stream << fixed << setprecision(2) << "@(" << real(first) << ", " << imag(first) << ")";
        }

        template <typename U>
        void operator()(const U& first) {
            stream << first;
        }

        string str() {
            return stream.str();
        }
    };

    struct FloatingOpVisitor : boost::static_visitor<Number::magic_t> {
        typedef std::function<Number::floating(Number::floating)> function_type;
        typedef std::function<Number::complex(Number::complex)> complex_function_type;
        function_type func;
        complex_function_type cfunc;

        FloatingOpVisitor(function_type f, complex_function_type cf)
            : func(f), cfunc(cf) {}

        template <typename U>
        Number::magic_t operator()(const U& first) const {
            return Number::magic_t(func(Coerce<Number::floating>::act(first)));
        }

        Number::magic_t operator()(const Number::complex& first) const {
            return Number::magic_t(cfunc(first));
        }

    };

    struct FloatingComplexOpVisitor : boost::static_visitor<Number::magic_t> {
        typedef FloatingOpVisitor::function_type function_type;
        typedef FloatingOpVisitor::complex_function_type complex_function_type;
        typedef std::function<bool(Number::floating)> predicate_type;
        // Simulate inheritance without letting overload resolution complicate things.
        FloatingOpVisitor visitor;
        predicate_type pred;

        FloatingComplexOpVisitor(function_type f, complex_function_type cf, predicate_type p)
            : visitor(f, cf), pred(p) {}

        template <typename U>
        Number::magic_t operator()(const U& first) const {
            Number::floating value = Coerce<Number::floating>::act(first);
            if (pred(value))
                return visitor(Coerce<Number::complex>::act(value));
            else
                return visitor(value);
        }

        Number::magic_t operator()(const Number::complex& first) const {
            return visitor(first);
        }

    };

    struct FloorVisitor : boost::static_visitor<Number::magic_t> {

        Number::magic_t operator()(const Number::smallint& value) const {
            return value;
        }

        Number::magic_t operator()(const Number::bigint& value) const {
            return value;
        }

        Number::magic_t operator()(const Number::ratio& value) const {
            return static_cast<Number::bigint>(numerator(value) / denominator(value));
        }

        Number::magic_t operator()(const Number::floating& value) const {
            return static_cast<Number::bigint>(floor(value));
        }

        Number::magic_t operator()(const Number::complex& value) const {
            return Number::complex(0, 0);
        }

    };

}

Number::Number(smallint arg)
    : value(arg) {}

Number::Number(bigint arg)
    : value(arg) {}

Number::Number(ratio arg)
    : value(arg) {}

Number::Number(floating arg)
    : value(arg) {}

Number::Number(complex arg)
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
    function<complex(complex)> cfunc = [](const complex& c) { return std::sin(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::cos() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::cos;
    function<complex(complex)> cfunc = [](const complex& c) { return std::cos(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::tan() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::tan;
    function<complex(complex)> cfunc = [](const complex& c) { return std::tan(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::sinh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::sinh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::sinh(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::cosh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::cosh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::cosh(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::tanh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::tanh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::tanh(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::exp() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::exp;
    function<complex(complex)> cfunc = [](const complex& c) { return std::exp(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::asin() const {
    Number curr = *this;
    function<bool(double)> pred = [](double d) { return (d < -1) || (d > 1); };
    function<double(double)> func = (double(*)(double))std::asin;
    function<complex(complex)> cfunc = [](const complex& c) { return std::asin(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingComplexOpVisitor(func, cfunc, pred), curr.value);
    return curr;
}

Number Number::acos() const {
    Number curr = *this;
    function<bool(double)> pred = [](double d) { return (d < -1) || (d > 1); };
    function<double(double)> func = (double(*)(double))std::acos;
    function<complex(complex)> cfunc = [](const complex& c) { return std::acos(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingComplexOpVisitor(func, cfunc, pred), curr.value);
    return curr;
}

Number Number::atan() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::atan;
    function<complex(complex)> cfunc = [](const complex& c) { return std::atan(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::asinh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::asinh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::asinh(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::acosh() const {
    Number curr = *this;
    function<bool(double)> pred = [](double d) { return (d < 1); };
    function<double(double)> func = (double(*)(double))std::acosh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::acosh(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingComplexOpVisitor(func, cfunc, pred), curr.value);
    return curr;
}

Number Number::atanh() const {
    Number curr = *this;
    function<bool(double)> pred = [](double d) { return (d == -1) || (d == 1); };
    function<double(double)> func = (double(*)(double))std::atanh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::atanh(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingComplexOpVisitor(func, cfunc, pred), curr.value);
    return curr;
}

Number Number::log() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::log;
    function<complex(complex)> cfunc = [](const complex& c) { return std::log(c); };
    curr.value = boost::apply_visitor(MagicNumber::FloatingOpVisitor(func, cfunc), curr.value);
    return curr;
}

Number Number::floor() const {
    Number curr = *this;
    curr.value = boost::apply_visitor(MagicNumber::FloorVisitor(), curr.value);
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

boost::optional<Number> constantNan() {
    if (numeric_limits<Number::floating>::has_quiet_NaN)
        return Number(numeric_limits<Number::floating>::quiet_NaN());
    else
        return boost::none;
}

boost::optional<Number> constantInf() {
    if (numeric_limits<Number::floating>::has_infinity)
        return Number(numeric_limits<Number::floating>::infinity());
    else
        return boost::none;
}

boost::optional<Number> constantNegInf() {
    if (numeric_limits<Number::floating>::has_infinity)
        return - Number(numeric_limits<Number::floating>::infinity());
    else
        return boost::none;
}

Number constantEps() {
    return Number(numeric_limits<Number::floating>::epsilon());
}
