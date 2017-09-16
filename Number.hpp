#ifndef NUMBER_HPP
#define NUMBER_HPP

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

/// \file
///
/// \brief The Number class and its helpers.

/// A Latitude number can be stored in one of five ways, depending on
/// the precision needed. They are listed here, in increasing order of
/// "wideness". That is, if an operation is performed that involves
/// two numbers, the resulting number will be at least as wide as the
/// wider of the two operands.
///
/// -# `smallint` A fixed-precision integer value
/// -# `bigint` An arbitrary precision integer value
/// -# `ratio` A fraction consisting of arbitrary precision integers
/// -# `floating` A floating-point real value
/// -# `complex` A floating-point complex value
class Number : private boost::operators<Number> {
public:

    /// A fixed-precision integer is a C++ long integer.
    typedef long smallint;

    /// An arbitrary precision integer.
    typedef boost::multiprecision::cpp_int bigint;

    /// A rational number.
    typedef boost::multiprecision::cpp_rational ratio;

    /// A floating-point number, using the appropriate C++ type.
    typedef double floating;

    /// A C++ complex type, consisting of floating-point values.
    typedef std::complex<double> complex;

    /// \brief This is the internal type of the Number instance which
    /// contains the actual value.
    typedef boost::variant<smallint, bigint, ratio, floating, complex> magic_t;

private:
    std::unique_ptr<magic_t> value;
public:

    /// \details Constructs a zero number of the narrowest type.
    Number();

    /// Constructs a number.
    ///
    /// \param arg a small integer
    Number(smallint arg);

    /// Constructs a number.
    ///
    /// \param arg a large integer
    Number(bigint arg);

    /// Constructs a number.
    ///
    /// \param arg a rational number
    Number(ratio arg);

    /// Constructs a number.
    ///
    /// \param arg a floating-point number
    Number(floating arg);

    /// Constructs a number.
    ///
    /// \param arg a complex number
    Number(complex arg);

    /// Copy-constructs a number.
    ///
    /// \param num the other number
    Number(const Number& num);

    /// Copy-assigns a number.
    ///
    /// \param other the other number
    Number& operator=(Number other);

    /// Returns whether the two numbers are the same. The comparison
    /// is done by coercing to the wider type.
    ///
    /// \param other the other number
    /// \return whether the values are equal
    bool operator ==(const Number& other) const;

    /// Returns whether the number is less than another number. The
    /// comparison is done by coercing to the wider type. Comparing
    /// complex numbers with this operator always returns false.
    ///
    /// \param other the other number
    /// \return whether the value is less than the argument
    bool operator <(const Number& other) const;

    /// Adds the number in-place.
    ///
    /// \param other the number to add
    /// \return the current number
    Number& operator +=(const Number& other);

    /// Subtracts the number in-place.
    ///
    /// \param other the number to subtract
    /// \return the current number
    Number& operator -=(const Number& other);

    /// Multiplies the number in-place.
    ///
    /// \param other the number to multiply
    /// \return the current number
    Number& operator *=(const Number& other);

    /// Divides the number in-place.
    ///
    /// \param other the divisor
    /// \return the current number
    Number& operator /=(const Number& other);

    /// Replaces the number with its value modulo some divisor.
    ///
    /// \param other the divisor
    /// \return the current number
    Number& operator %=(const Number& other);

    /// Returns the current number to the power of the argument. If
    /// the answer does not exist, then the appropriate floating-point
    /// exceptional value, such as NaN, is returned. If floating-point
    /// special values are unsupported on the current system, the
    /// behavior is undefined.
    ///
    /// \param other the exponent
    /// \return the resulting value
    Number pow(const Number& other) const;

    /// Negates the current number.
    ///
    /// \return the additive inverse
    Number operator -() const;

    /// Reciprocates the value. If the value is zero, then the
    /// appropriate floating-point exceptional value is returned, with
    /// an architecture-defined fallback if that fails.
    ///
    /// \return the multiplicative inverse
    Number recip() const;

    /// Performs bitwise AND on the number in-place.
    ///
    /// \param other the other number
    /// \return the current number
    Number& operator &=(const Number& other);

    /// Performs bitwise OR on the number in-place.
    ///
    /// \param other the other number
    /// \return the current number
    Number& operator |=(const Number& other);

    /// Performs bitwise XOR on the number in-place.
    ///
    /// \param other the other number
    /// \return the current number
    Number& operator ^=(const Number& other);

    /// Performs bitwise NOT on the number in-place.
    ///
    /// \return the result
    Number operator ~() const;

    /// Returns the sine of the number.
    ///
    /// \return the result of the computation
    Number sin() const;

    /// Returns the cosine of the number.
    ///
    /// \return the result of the computation
    Number cos() const;

    /// Returns the tangent of the number.
    ///
    /// \return the result of the computation
    Number tan() const;

    /// Returns the hyperbolic sine of the number.
    ///
    /// \return the result of the computation
    Number sinh() const;

    /// Returns the hyperbolic cosine of the number.
    ///
    /// \return the result of the computation
    Number cosh() const;

    /// Returns the hyperbolic tangent of the number.
    ///
    /// \return the result of the computation
    Number tanh() const;

    /// Returns e to the power of the number.
    ///
    /// \return the result of the computation
    Number exp() const;

    /// Returns the inverse sine of the number.
    ///
    /// \return the result of the computation
    Number asin() const;

    /// Returns the inverse cosine of the number.
    ///
    /// \return the result of the computation
    Number acos() const;

    /// Returns the inverse tangent of the number.
    ///
    /// \return the result of the computation
    Number atan() const;

    /// Returns the inverse hyperbolic sine of the number.
    ///
    /// \return the result of the computation
    Number asinh() const;

    /// Returns the inverse hyperbolic cosine of the number.
    ///
    /// \return the result of the computation
    Number acosh() const;

    /// Returns the inverse hyperbolic tangent of the number.
    ///
    /// \return the result of the computation
    Number atanh() const;

    /// Returns the natural logarithm of the number.
    ///
    /// \return the result of the computation
    Number log() const;

    /// Returns the floor of the number.
    ///
    /// \return the result of the computation
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

#endif // NUMBER_HPP
