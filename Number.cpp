#include "Number.hpp"
#include <limits>
#include <cmath>
#include <complex>

using namespace std;

// Implementation details for type coercion inside the boost::variant for the number types
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

    struct LevelVisitor {
        template <typename U>
        int operator()(const U& first) const {
            return (int)NumeralT<U>::type::value;
        }
    };

    template <typename T>
    struct StrictCastVisitor {
        template <typename U>
        T operator()(const U& first) const {
            return (T)first;
        }
        T operator()(const Number::complex& first) const {
            return (T)real(first);
        }
    };

    struct PlusVisitor {
        template <typename U, typename V>
        Number::magic_t operator()(U& first, V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            typename Wider<U, V>::type result = first0 + second0;
            return Number::magic_t(result);
        }
    };

    struct TimesVisitor {
        template <typename U, typename V>
        Number::magic_t operator()(U& first, V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            typename Wider<U, V>::type result = first0 * second0;
            return Number::magic_t(result);
        }
    };

    struct NegateVisitor {
        template <typename U>
        Number::magic_t operator()(U& first) const {
            U result = - first;
            return Number::magic_t(result);
        }
    };

    struct RecipVisitor {
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

    struct PrimFloorVisitor {

        Number::smallint operator()(const Number::smallint& value) const {
            return value;
        }

        Number::bigint operator()(const Number::bigint& value) const {
            return value;
        }

        Number::bigint operator()(const Number::ratio& value) const {
            return static_cast<Number::bigint>(numerator(value) / denominator(value));
        }

        Number::bigint operator()(const Number::floating& value) const {
            return static_cast<Number::bigint>(floor(value));
        }

    };

    struct ModVisitor {
        template <typename U, typename V>
        Number::magic_t operator()(U& first, V& second) const {
            typedef typename Wider<U, V>::type wide_t;
            PrimFloorVisitor floor;
            if (second == 0)
                return fmod(Coerce<Number::floating>::act(first),
                            Coerce<Number::floating>::act(second));
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            auto div = Coerce<wide_t>::act(floor((wide_t)(first0 / second0)));
            wide_t result = first0 - div * second0;
            if ((!std::is_same<wide_t, Number::floating>::value) && ((second0 < 0) ^ (first0 < 0)))
                result += second0;
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

    struct PowerVisitor {

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
                return std::visit(*this, firstx, secondx);
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
                return std::visit(*this, firstx, secondx);
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

    struct EqualVisitor {
        template <typename U, typename V>
        bool operator()(const U& first, const V& second) const {
            auto first0 = Coerce<V>::act(first);
            auto second0 = Coerce<U>::act(second);
            return first0 == second0;
        }
    };

    struct LessVisitor {
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

    struct StringifyVisitor {
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
            stream << fixed << setprecision(2);
            (*this)(real(first));
            stream << showpos;
            (*this)(imag(first));
            stream << "i";
        }

        template <typename U>
        void operator()(const U& first) {
            stream << first;
        }

        string str() {
            return stream.str();
        }
    };

    struct LogVisitor {

        template <typename U>
        Number::magic_t operator()(const U& first) const {
            auto value = Coerce<Number::floating>::act(first);
            if (value < 0)
                return (*this)(Coerce<Number::complex>::act(value));
            else
                return Number::magic_t(std::log(value));
        }

        Number::magic_t operator()(const Number::complex& first) const {
            return Number::magic_t(std::log(first));
        }

    };

    struct FloatingOpVisitor {
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

    struct FloatingComplexOpVisitor {
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

    struct FloorVisitor {

        Number::magic_t operator()(const Number::smallint& value) const {
            return value;
        }

        Number::magic_t operator()(const Number::bigint& value) const {
            return value;
        }

        Number::magic_t operator()(const Number::ratio& value) const {
            if ((numerator(value) % denominator(value) != 0) && (value < 0))
                return static_cast<Number::bigint>(numerator(value) / denominator(value) - 1);
            else
                return static_cast<Number::bigint>(numerator(value) / denominator(value));
        }

        Number::magic_t operator()(const Number::floating& value) const {
            return static_cast<Number::bigint>(floor(value));
        }

        Number::magic_t operator()(const Number::complex& value) const {
            return Number::complex(0, 0);
        }

    };

    struct ComplexVisitor {
        template <typename U, typename V>
        Number::magic_t operator()(const U& first, const V& second) const {
            Number::floating first0 = static_cast<Number::floating>(first);
            Number::floating second0 = static_cast<Number::floating>(second);
            return Number::complex(first0, second0);
        }
        Number::magic_t operator()(const Number::complex& first, const Number::complex& second) const {
            return first + second * Number::complex(0, 1);
        }
        template <typename V>
        Number::magic_t operator()(const Number::complex& first, const V& second) const {
            return (*this)(first, Coerce<Number::complex>::act(second));
        }
        template <typename U>
        Number::magic_t operator()(const U& first, const Number::complex& second) const {
            return (*this)(Coerce<Number::complex>::act(first), second);
        }
    };

    struct AndVisitor {

        Number::magic_t operator()(const Number::smallint& first, const Number::smallint& second) const {
            return Number::magic_t(first & second);
        }

        template <typename V>
        Number::magic_t operator()(const Number::bigint& first, const V& second) const {
            return (*this)(first, Coerce<Number::bigint>::act(second));
        }

        template <typename U>
        Number::magic_t operator()(const U& first, const Number::bigint& second) const {
            return (*this)(Coerce<Number::bigint>::act(first), second);
        }

        Number::magic_t operator()(const Number::bigint& first, const Number::bigint& second) const {
            return Number::magic_t((Number::bigint)(first & second));
        }

        template <typename U, typename V>
        Number::magic_t operator()(const U& first, const V& second) const {
            return Number::magic_t(0L); // Doesn't make sense, so return a default value
        }

    };

    struct OrVisitor {

        Number::magic_t operator()(const Number::smallint& first, const Number::smallint& second) const {
            return Number::magic_t(first | second);
        }

        template <typename V>
        Number::magic_t operator()(const Number::bigint& first, const V& second) const {
            return (*this)(first, Coerce<Number::bigint>::act(second));
        }

        template <typename U>
        Number::magic_t operator()(const U& first, const Number::bigint& second) const {
            return (*this)(Coerce<Number::bigint>::act(first), second);
        }

        Number::magic_t operator()(const Number::bigint& first, const Number::bigint& second) const {
            return Number::magic_t((Number::bigint)(first | second));
        }

        template <typename U, typename V>
        Number::magic_t operator()(const U& first, const V& second) const {
            return Number::magic_t(0L); // Doesn't make sense, so return a default value
        }

    };

    struct XorVisitor {

        Number::magic_t operator()(const Number::smallint& first, const Number::smallint& second) const {
            return Number::magic_t(first ^ second);
        }

        template <typename V>
        Number::magic_t operator()(const Number::bigint& first, const V& second) const {
            return (*this)(first, Coerce<Number::bigint>::act(second));
        }

        template <typename U>
        Number::magic_t operator()(const U& first, const Number::bigint& second) const {
            return (*this)(Coerce<Number::bigint>::act(first), second);
        }

        Number::magic_t operator()(const Number::bigint& first, const Number::bigint& second) const {
            return Number::magic_t((Number::bigint)(first ^ second));
        }

        template <typename U, typename V>
        Number::magic_t operator()(const U& first, const V& second) const {
            return Number::magic_t(0L); // Doesn't make sense, so return a default value
        }

    };

    struct ComplVisitor {

        Number::magic_t operator()(const Number::smallint& first) const {
            return Number::magic_t(~first);
        }

        Number::magic_t operator()(const Number::bigint& first) const {
            return Number::magic_t((Number::bigint)~first);
        }

        template <typename U>
        Number::magic_t operator()(const U& first) const {
            return Number::magic_t(0L); // Doesn't make sense, so return a default value
        }

    };

    struct DestructuringVisitor {

        template <typename U>
        std::tuple<Number, Number> operator()(const U& first) const {
            return std::make_tuple(Number(first), Number((Number::smallint)0));
        }

        std::tuple<Number, Number> operator()(const Number::complex& first) const {
            return std::make_tuple(Number(first.real()), Number(first.imag()));
        }

    };

}

Number::Number()
    : value(std::make_unique<magic_t>()) {}

Number::Number(smallint arg)
    : value(std::make_unique<magic_t>(arg)) {}

Number::Number(bigint arg)
    : value(std::make_unique<magic_t>(arg)) {}

Number::Number(ratio arg)
    : value(std::make_unique<magic_t>(arg)) {}

Number::Number(floating arg)
    : value(std::make_unique<magic_t>(arg)) {}

Number::Number(complex arg)
    : value(std::make_unique<magic_t>(arg)) {}

Number::Number(const Number& num)
    : value(std::make_unique<magic_t>(*num.value)) {}

Number& Number::operator =(Number other) {
    swap(this->value, other.value);
    return *this;
}

bool Number::operator ==(const Number& other) const {
    auto& second = *other.value;
    return std::visit(MagicNumber::EqualVisitor(), *value, second);
}

bool Number::operator <(const Number& other) const {
    auto& second = *other.value;
    return std::visit(MagicNumber::LessVisitor(), *value, second);
}

Number& Number::operator +=(const Number& other) {
    auto& second = *other.value;
    if ((value->index() == 0) && (second.index() == 0)) {
        // Possibility of overflow (two smallints)
        bool promote = false;
        smallint x0 = std::get<smallint>(*value);
        smallint x1 = std::get<smallint>(second);
        if ((x1 > 0) && (x0 > numeric_limits<smallint>::max() - x1))
            promote = true;
        if ((x1 < 0) && (x0 < numeric_limits<smallint>::min() - x1))
            promote = true;
        if (promote)
            value = std::make_unique<magic_t>(bigint(x0));
    }
    value = std::make_unique<magic_t>(std::visit(MagicNumber::PlusVisitor(), *value, second));
    return *this;
}

Number& Number::operator -=(const Number& other) {
    return (*this += -other);
}

Number& Number::operator *=(const Number& other) {
    auto& second = *other.value;
    if ((value->index() == 0) && (second.index() == 0)) {
        // Possibility of overflow (two smallints)
        bool promote = false;
        smallint x0 = std::get<smallint>(*value);
        smallint x1 = std::get<smallint>(second);
        if ((x1 != 0) && (abs(x0) > numeric_limits<smallint>::max() / abs(x1)))
            promote = true;
        if (promote)
            value = std::make_unique<magic_t>(bigint(x0));
    }
    value = std::make_unique<magic_t>(std::visit(MagicNumber::TimesVisitor(), *value, second));
    return *this;
}

Number& Number::operator /=(const Number& other) {
    return (*this *= other.recip());
}

Number& Number::operator %=(const Number& other) {
    auto& second = *other.value;
    value = std::make_unique<magic_t>(std::visit(MagicNumber::ModVisitor(), *value, second));
    return *this;
}

Number Number::pow(const Number& other) const {
    auto& first = *this->value;
    auto& second = *other.value;
    Number result;
    result.value = std::make_unique<magic_t>(std::visit(MagicNumber::PowerVisitor(), first, second));
    return result;
}

Number Number::operator -() const {
    Number curr = *this;
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::NegateVisitor(), *curr.value));
    return curr;
}

Number Number::recip() const {
    Number curr = *this;
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::RecipVisitor(), *curr.value));
    return curr;
}

Number& Number::operator &=(const Number& other) {
    auto& second = *other.value;
    value = std::make_unique<magic_t>(std::visit(MagicNumber::AndVisitor(), *value, second));
    return *this;
}

Number& Number::operator |=(const Number& other) {
    auto& second = *other.value;
    value = std::make_unique<magic_t>(std::visit(MagicNumber::OrVisitor(), *value, second));
    return *this;
}

Number& Number::operator ^=(const Number& other) {
    auto& second = *other.value;
    value = std::make_unique<magic_t>(std::visit(MagicNumber::XorVisitor(), *value, second));
    return *this;
}

Number Number::operator ~() const {
    Number next;
    next.value = std::make_unique<magic_t>(std::visit(MagicNumber::ComplVisitor(), *value));
    return next;
}

Number Number::sin() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::sin;
    function<complex(complex)> cfunc = [](const complex& c) { return std::sin(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::cos() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::cos;
    function<complex(complex)> cfunc = [](const complex& c) { return std::cos(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::tan() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::tan;
    function<complex(complex)> cfunc = [](const complex& c) { return std::tan(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::sinh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::sinh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::sinh(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::cosh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::cosh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::cosh(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::tanh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::tanh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::tanh(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::exp() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::exp;
    function<complex(complex)> cfunc = [](const complex& c) { return std::exp(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::asin() const {
    Number curr = *this;
    function<bool(double)> pred = [](double d) { return (d < -1) || (d > 1); };
    function<double(double)> func = (double(*)(double))std::asin;
    function<complex(complex)> cfunc = [](const complex& c) { return std::asin(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingComplexOpVisitor(func, cfunc, pred), *curr.value));
    return curr;
}

Number Number::acos() const {
    Number curr = *this;
    function<bool(double)> pred = [](double d) { return (d < -1) || (d > 1); };
    function<double(double)> func = (double(*)(double))std::acos;
    function<complex(complex)> cfunc = [](const complex& c) { return std::acos(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingComplexOpVisitor(func, cfunc, pred), *curr.value));
    return curr;
}

Number Number::atan() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::atan;
    function<complex(complex)> cfunc = [](const complex& c) { return std::atan(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::asinh() const {
    Number curr = *this;
    function<double(double)> func = (double(*)(double))std::asinh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::asinh(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingOpVisitor(func, cfunc), *curr.value));
    return curr;
}

Number Number::acosh() const {
    Number curr = *this;
    function<bool(double)> pred = [](double d) { return (d < 1); };
    function<double(double)> func = (double(*)(double))std::acosh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::acosh(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingComplexOpVisitor(func, cfunc, pred), *curr.value));
    return curr;
}

Number Number::atanh() const {
    Number curr = *this;
    function<bool(double)> pred = [](double d) { return (d == -1) || (d == 1); };
    function<double(double)> func = (double(*)(double))std::atanh;
    function<complex(complex)> cfunc = [](const complex& c) { return std::atanh(c); };
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloatingComplexOpVisitor(func, cfunc, pred), *curr.value));
    return curr;
}

Number Number::log() const {
    Number curr = *this;
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::LogVisitor(), *curr.value));
    return curr;
}

Number Number::floor() const {
    Number curr = *this;
    curr.value = std::make_unique<magic_t>(std::visit(MagicNumber::FloorVisitor(), *curr.value));
    return curr;
}

string Number::asString() const {
    MagicNumber::StringifyVisitor visitor;
    std::visit(visitor, *value);
    return visitor.str();
}

Number Number::realPart() const {
    return std::get<0>(std::visit(MagicNumber::DestructuringVisitor(), *value));
}

Number Number::imagPart() const {
    return std::get<1>(std::visit(MagicNumber::DestructuringVisitor(), *value));
}

auto Number::asSmallInt() const
    -> smallint {
    return std::visit(MagicNumber::StrictCastVisitor<smallint>(), *value);
}

int Number::hierarchyLevel() const {
    return std::visit(MagicNumber::LevelVisitor(), *value);
}

Number complexNumber(const Number& real, const Number& imag) {
    Number curr;
    curr.value = std::make_unique<Number::magic_t>(std::visit(MagicNumber::ComplexVisitor(), *real.value, *imag.value));
    return curr;
}

std::optional<Number> constantNan() {
    if (numeric_limits<Number::floating>::has_quiet_NaN)
        return Number(numeric_limits<Number::floating>::quiet_NaN());
    else
        return std::nullopt;
}

std::optional<Number> constantInf() {
    if (numeric_limits<Number::floating>::has_infinity)
        return Number(numeric_limits<Number::floating>::infinity());
    else
        return std::nullopt;
}

std::optional<Number> constantNegInf() {
    if (numeric_limits<Number::floating>::has_infinity)
        return - Number(numeric_limits<Number::floating>::infinity());
    else
        return std::nullopt;
}

Number constantEps() {
    return Number(numeric_limits<Number::floating>::epsilon());
}

std::optional<Number> parseInteger(const char* integer) {
    int radix;
    if (integer[0] == 'D') {
        radix = 10;
    } else if (integer[0] == 'X') {
        radix = 16;
    } else if (integer[0] == 'O') {
        radix = 8;
    } else if (integer[0] == 'B') {
        radix = 2;
    } else {
        return std::nullopt;
    }
    ++integer;
    long sign = 1;
    if (*integer == '+') {
        ++integer;
    } else if (*integer == '-') {
        ++integer;
        sign *= -1;
    }
    Number accum = 0L;
    while (*integer != 0) {
        int value;
        if ((*integer >= '0') && (*integer <= '9'))
            value = *integer - '0';
        else if ((*integer >= 'A') && (*integer <= 'Z'))
            value = *integer - 'A' + 10;
        else if ((*integer >= 'a') && (*integer <= 'z'))
            value = *integer - 'a' + 10;
        else
            return std::nullopt;
        if (value >= radix)
            return std::nullopt;
        accum = accum * (long)radix + (long)value;
        ++integer;
    }
    return sign * accum;
}
