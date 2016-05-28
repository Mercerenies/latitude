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
    std::string asString() const;
    smallint asSmallInt() const;
    int hierarchyLevel() const;
};

#endif // _NUMBER_HPP_
