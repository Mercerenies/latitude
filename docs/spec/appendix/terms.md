
# Terms and Conventions

## Type Errors

Many methods documented here make certain assumptions about their
arguments. Often, these methods will not explicitly specify the
behavior if the argument does not satisfy those assumptions. Unless
otherwise stated, either an `ArgError` or a `TypeError` will be raised
in these cases.

A `TypeError` will be raised if the argument does not satisfy some
rigid notion of "type" or does not have the correct object
hierarchy. An `ArgError` will be raised in any other case. For
instance, a method that expects an integer will raise a `TypeError` if
given a symbol but will raise an `ArgError` if given a non-integer
number.

## Indexing

In Latitude, most lists and sequences are indexed using a standard
convention. Nonnegative indices count from the front of the sequence,
where `0` is the first element. Negative indices count from the back
of the sequence, where `-1` is the last element. Indices after the end
of the list or before the beginning are considered to be out of
bounds, which will result in
a
[`BoundsError`](../ii_standard_library/exception.md#boundserror). Indices
which are not integers will produce an `ArgError`.

When a subsequence or substring is accessed, out-of-bounds indices
will usually be truncated to be the front or back of the
list. Subsequences are, unless otherwise stated, indexed using
half-open intervals, so the start index is included in the list but
the end index is not. If the end index points to a position at or
before the start index, the result will be the empty sequence or
string.

## Loop Macros

Many loop-like constructs in Latitude define specialized versions
ending in a `*`. The specialized loop versions add special flow
control tools for exiting the loop abnormally or jumping ahead to the
next iteration, at the cost of a performance penalty over using the
un-specialized version. For instance, `global loop` is a loop which
will run forever; `global loop*` behaves the same way but with
additionall macros defined.

The following macros are defined in these specialized looping
constructs. Macros beginning with a `$` are assumed to be defined in
the dynamic scope.
 * `last` and `$last` are 1-ary methods which exit the loop and force
   the return value of the loop to be the sole argument.
 * `next` and `$next` are 0-ary methods which skip to the next
   iteration of the loop.

[[up](.)]
<br/>[[next - Definitions](definitions.md)]
