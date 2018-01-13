
# The Number Object

    Number

The number object represents numerical values in Latitude. Numerical
literals always return subobjects of this object. Internally, numbers
are stored using any one of several representations. A Latitude
implementation must support at least the following representations, in
order from narrowest to widest representation.

 * Integers
 * Rational numbers
 * Floating-point numbers
 * Complex numbers

The integer and rational number types must be exact and support
arbitrary precision. When an arithmetic operation is performed, the
result of the operation will, whenever possible, be the widest of the
following.

 * The widest representation of any of the argument numbers
 * The narrowest representation that can accurately describe the
   result

Additionally, many implementations will supply a fixed-precision
integer type, which will be used in cases where it is small enough. In
this case, fixed-size integers are considered narrower than the
general integer representation and must automatically shift into wider
types when the value becomes too large or too small to fit into a
fixed-size integer.

Note that, unless otherwise specified, when this documentation refers
to an integer, it is referring to a numerical value using an integer
representation. So `2` is an integer, whereas `2.00` is a
floating-point number which happens to be equal in magnitude to an
integer. Likewise, even though `2` and `(2 / 1)` are equivalent values
and are exact, it is not correct to describe `(2 / 1)` as an
integer-represented value. It is a rational number which happens to be
exactly representable as an integer.

## Simple Slots

    Number ee := #<2.718281828...>.
    Number pi := #<3.141592653...>.
    Number ii := @(0, 1).
    Number nan := #<NaN, or Nil if not supported>.
    Number inf := #<positive infinity, or Nil if not supported>.
    Number ninf := #<negative infinity, or Nil if not supported>.
    Number epsilon := #<the machine's floating-point epsilon>.

## Methods

### `Number ordinal.`

As detailed
in
[Symbol Table](../i_syntax_and_semantics/ch3_object.md#symbol-table),
every positive integer is uniquely associated with a special symbol,
called an ordinal symbol. Given a positive integer, this method
returns the ordinal symbol associated with it. A `TypeError` is thrown
if the value is non-positive or non-integer.

[TODO: For storage reasons, we need to define an upper cap on this; it
can't go on forever.]

### `Number times (block).`

Executes the block `self` times. If `self` is a non-integer, it is
rounded down to the nearest integer before executing the block. Each
time the block is executed, it is passed an argument, where the
argument ranges from `0` up to, but not including, `self`.

Inside of the block, a dynamic variable named `$break` will be bound
to a method which, if called with an argument, will immediately exit
the loop and return the given value. If the loop exits normally, the
returned value is `Nil`.

Note that `n times (block).` is equivalent to `0 upto n do (block).`

### `Number upto (n) do (block).`

Executes the block once for each integer from `self` up to, but not
including, `n`, passing the current integer value as an argument to
the block. If either of `self` or `n` is a non-integer, they will be
rounded down to the nearest integer before the loop begins.

Inside of the block, a dynamic variable named `$break` will be bound
to a method which, if called with an argument, will immediately exit
the loop and return the given value. If the loop exits normally, the
returned value is `Nil`.

### `Number downto (n) do (block).`

Executes the block once for each integer from `self` down to, but not
including, `n`, passing the current integer value as an argument to the
block. If either of `self` or `n` is a non-integer, they will be
rounded down to the nearest integer before the loop begins.

Inside of the block, a dynamic variable named `$break` will be bound
to a method which, if called with an argument, will immediately exit
the loop and return the given value. If the loop exits normally, the
returned value is `Nil`.

### `Number asciiChr.`

Returns the character whose ASCII value is equal to `self`. If `self`
is not an integer, or if `self` is out of ASCII bounds, an `ArgError`
is thrown.

Note carefully that ASCII is a *7-bit* encoding. As such, a caller
outside of the range 0 to 127 will throw an exception. In particular,
values from 128 to 255, which in latin1 (ISO/IEC 8859-1) would be
valid characters, will result in an exception being thrown.

### `Number chr.`

Returns the character whose Unicode code point is equal to `self`. If
`self` is not an integer, or if `self` is out of the range of
assignable Unicode code points, an `ArgError` is thrown. Note that
`self` could be an unassigned code point, in which case the
appropriate character will be returned, even if it is not a conforming
UTF-8 character.

### `Number toString.`

Returns a string representation of the number. If the value is an
integer, the string representation will be the exact digits of the
integer, possibly preceded by a negative sign. If the value is a
rational number, the string representation will include the numerator
and denominator, in reduced form, and some clear indication that the
number is rational. If the value is a floating-point number, the
string representation should indicate clearly that the number is a
floating-point value and should show at least one decimal place. If
the number is a complex number, the string representation should
clearly indicate that the number is complex and should include the
real and imaginary parts, each to at least one decimal place.

[TODO: This may become more strict in the future.]

### `Number == arg.`

Returns whether the two values refer to the same number. This method
is independent of representation, so for instance `1` and `1 / 1`
refer to the same number. In the case of floating-point or complex
numbers, the system's definition of floating-point equality shall be
used.

As with many programming languages, the equality method on numbers may
fail to be transitive or reflexive when used on non-exact numerical
representations.

### `Number < arg.`

Returns whether `self` is less than the argument. If either `self` or
`arg` is complex, an `ArgError` will be thrown.

### `Number + arg.`

Returns the sum of the two numbers. The resulting value will be in the
wider of the two representations of the arguments, with the exception
that a fixed integer will be coerced into an arbitrary-precision
integer if an overflow would occur.

### `Number - arg.`

Returns the difference of the two numbers. The resulting value will be
in the wider of the two representations of the arguments, with the
exception that a fixed integer will be coerced into an
arbitrary-precision integer if an overflow would occur.

### `Number * arg.`

Returns the product of the two numbers. The resulting value will be in
the wider of the two representations of the arguments, with the
exception that a fixed integer will be coerced into an
arbitrary-precision integer if an overflow would occur.

### `Number / arg.`

Returns the quotient of the two numbers. The resulting value will be
in the wider of the two representations of the arguments, with the
caveat that the resulting value will be at least as wide as a rational
number.

In the case of real-valued division by zero, the floating-point value
positive infinity. In the case of complex-valued division by zero, the
result will be a complex number whose real and imaginary parts of
infinity or NaN. On machines that do not support infinity and NaN, the
behavior of division by zero is undefined.

### `Number mod (arg).`

Returns `self` modulo the argument. The resulting value will be in the
wider of the two representations. If either argument is complex, an
`ArgError` will be thrown. The sign of the returned value will always
match the sign of the divisor.

### `Number ^ arg.`

Returns `self` to the power of `arg`. The representation of the result
depends heavily on the representation of `arg`.

If `arg` is an integer, the result will be of the same representation
as `self`. However, fixed-size integers will be coerced into
arbitrary-size integers as needed, and the result will be at least as
wide as `arg`. Also, if `arg` is negative, the result will be at least
as wide as a rational number.

If `arg` is rational or floating-point, the result will be at least as
wide as a floating-point number. If `arg` is negative or `self` is
complex, the result will be complex. Otherwise, the result will be
floating-point.

If `arg` is complex, the result will always be complex.

In any case that would be mathematically indeterminate or result in an
infinite value, the appropriate floating-point or complex value will
be returned. If such a value is not available on the system, the
behavior is undefined.

### `Number real.`

Returns the real part of `self` if it is complex. If the value is
real, the value itself is returned. In the former case, the resulting
value will be a floating-point number.

### `Number imag.`

Returns the imaginary part of `self` if it is complex. If the value is
real, the integer `0` is returned. In the former case, the resulting
value will be a floating-point number.

### `Number abs.`

Returns the absolute value of the number. If the number is real, the
resulting value will have the same representation as the original
number. If the number is complex, the result will be floating-point.

### `Number floor.`

Returns the greatest integer which is less than or equal to `self`. If
`self` is complex, an `ArgError` is raised.

### `Number round.`

Returns the integer which is nearest to `self`. If `self` is complex,
an `ArgError` is raised.

### `Number ceil.`

Returns the least integer which is greater than or equal to `self`. If
`self` is complex, an `ArgError` is raised.

### `Number bitAnd (arg).`

Returns the bitwise conjunction of the two values, which must be
integers. Negative numbers are treated as being in two's complement
form. If either argument is not an integer, an `ArgError` is raised.

### `Number bitOr (arg).`

Returns the bitwise disjunction of the two values, which must be
integers. Negative numbers are treated as being in two's complement
form. If either argument is not an integer, an `ArgError` is raised.

### `Number bitXor (arg).`

Returns the bitwise symmetric disjunction of the two values, which
must be integers. Negative numbers are treated as being in two's
complement form. If either argument is not an integer, an `ArgError`
is raised.

### `Number bitNot.`

Returns the bitwise negation of the value. Negative numbers are
treated as being in two's complement form. If `self` is not an
integer, an `ArgError` is raised.

### `Number bitShift (n).`

Returns `self` shifted to the right by `n` bits. If `n` is negative,
returns `self` shifted to the left by `- n` bits. Negative numbers are
treated as being in two's complement form. If either `self` or `n` is
non-integer, an `ArgError` is raised.

### `Number sin.`

Returns the sine of `self`. The result will be complex if `self` is
complex, or floating-point otherwise.

### `Number cos.`

Returns the cosine of `self`. The result will be complex if `self` is
complex, or floating-point otherwise.

### `Number tan.`

Returns the tangent of `self`. The result will be complex if `self` is
complex, or floating-point otherwise.

### `Number csc.`

Returns the cosecant of `self`. The result will be complex if `self` is
complex, or floating-point otherwise.

### `Number sec.`

Returns the secant of `self`. The result will be complex if `self` is
complex, or floating-point otherwise.

### `Number cot.`

Returns the cotangent of `self`. The result will be complex if `self` is
complex, or floating-point otherwise.

### `Number sinh.`

Returns the hyperbolic sine of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number cosh.`

Returns the hyperbolic cosine of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number tanh.`

Returns the hyperbolic tangent of `self`. The result will be complex
if `self` is complex, or floating-point otherwise.

### `Number csch.`

Returns the hyperbolic cosecant of `self`. The result will be complex
if `self` is complex, or floating-point otherwise.

### `Number sech.`

Returns the hyperbolic secant of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number coth.`

Returns the hyperbolic cotangent of `self`. The result will be complex
if `self` is complex, or floating-point otherwise.

### `Number asin.`

Returns the inverse sine of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number acos.`

Returns the inverse cosine of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number atan.`

Returns the inverse tangent of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number acsc.`

Returns the inverse cosecant of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number asec.`

Returns the inverse secant of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number acot.`

Returns the inverse cotangent of `self`. The result will be complex if
`self` is complex, or floating-point otherwise.

### `Number asinh.`

Returns the inverse hyperbolic sine of `self`. The result will be
complex if `self` is complex, or floating-point otherwise.

### `Number acosh.`

Returns the inverse hyperbolic cosine of `self`. The result will be
complex if `self` is complex, or floating-point otherwise.

### `Number atanh.`

Returns the inverse hyperbolic tangent of `self`. The result will be
complex if `self` is complex, or floating-point otherwise.

### `Number acsch.`

Returns the inverse hyperbolic cosecant of `self`. The result will be
complex if `self` is complex, or floating-point otherwise.

### `Number asech.`

Returns the inverse hyperbolic secant of `self`. The result will be
complex if `self` is complex, or floating-point otherwise.

### `Number acoth.`

Returns the inverse hyperbolic cotangent of `self`. The result will be
complex if `self` is complex, or floating-point otherwise.

### `Number exp.`

Returns the mathematical constant `e` (`2.718281828...`) to the power
of `self`. The result will be complex if `self` is complex, or
floating-point otherwise.

### `Number log.`

Returns the natural logarithm of `self`. The result will be complex if
`self` is complex, or floating-point otherwise. If `self` is zero, the
result is the appropriate infinite floating-point or complex value. If
such a value is not supported on the system, the behavior is
undefined.

[TODO: A negative real argument to this should return a complex value
but currently returns NaN. This needs to be fixed.]

### `Number isBasicInt?.`

Returns whether the value is a fixed-size integer. If the
implementation does not support fixed-size integers, this method
always returns false.

### `Number isInteger?.`

Returns whether the value is represented as an integer, either
fixed-size or arbitrary-size.

### `Number isRational?.`

Returns whether the value is represented as a rational number or a
narrower numerical type.

### `Number isFloating?.`

Returns whether the value is represented as a floating-point number.

### `Number isReal?.`

Returns whether the value is a real number. A real number is a number
that is strictly narrower in representation than a complex number.

### `Number isComplex?.`

Returns whether the value is represented as a complex number. This
includes values with zero imaginary part, such as `@(1, 0)`, but does
not include values which lack an imaginary part, such as `1`.

## Static Methods

### `Number rect (real, imag).`

This method returns a new complex number with the given real and
imaginary parts.

### `Number polar (magn, dir).`

This method returns a new complex number with the given magnitude and
direction.

[TODO The rest of Number.]
