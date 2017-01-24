#ifndef _NUMBER_HPP_
#define _NUMBER_HPP_

#include <ios>
#include <string>
#include <type_traits>
#include <complex>
#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/size.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

class Number : private boost::operators<Number> {
public:
    typedef long smallint;
    typedef boost::multiprecision::cpp_int bigint;
    typedef boost::multiprecision::cpp_rational ratio;
    typedef double floating;
    typedef std::complex<double> complex;
    typedef boost::variant<smallint, bigint, ratio, floating, complex> magic_t;
private:
    magic_t value;
public:
    Number() = default;
    Number(const Number&) = default;
    Number(smallint);
    Number(bigint);
    Number(ratio);
    Number(floating);
    Number(complex);
    bool operator ==(const Number& other) const;
    bool operator <(const Number& other) const;
    Number& operator +=(const Number& other);
    Number& operator -=(const Number& other);
    Number& operator *=(const Number& other);
    Number& operator /=(const Number& other);
    Number& operator %=(const Number& other);
    Number pow(const Number& other) const;
    Number operator -() const;
    Number recip() const;
    Number& operator &=(const Number& other);
    Number& operator |=(const Number& other);
    Number& operator ^=(const Number& other);
    Number operator ~() const;
    Number sin() const;
    Number cos() const;
    Number tan() const;
    Number sinh() const;
    Number cosh() const;
    Number tanh() const;
    Number exp() const;
    Number asin() const;
    Number acos() const;
    Number atan() const;
    Number asinh() const;
    Number acosh() const;
    Number atanh() const;
    Number log() const;
    Number floor() const;
    std::string asString() const;
    smallint asSmallInt() const;
    int hierarchyLevel() const;
    friend Number complex_number(const Number&, const Number&);
};

Number complex_number(const Number& real, const Number& imag);

boost::optional<Number> constantNan();
boost::optional<Number> constantInf();
boost::optional<Number> constantNegInf();
Number constantEps();

#endif // _NUMBER_HPP_
