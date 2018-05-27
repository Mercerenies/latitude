
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

In many loop-like constructs in Latitude, there are a few standard
(dynamically-scoped) macros defined, as follows.

 * `$break` is a (non-evaluating) procedure object which, when called
   with one argument, will exit the loop, providing its argument as
   the return value of the loop.
 * `$continue` is a (non-evaluating) procedure object which, when
   called, will jump to the next iteration of the loop.

[[up](.)]
<br/>[[next - Definitions](definitions.md)]