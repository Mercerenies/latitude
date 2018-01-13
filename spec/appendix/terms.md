
# Terms

## Indexing

In Latitude, most lists and sequences are indexed using a standard
convention. Nonnegative indices count from the front of the sequence,
where `0` is the first element. Negative indices count from the back
of the sequence, where `-1` is the last element. Indices after the end
of the list or before the beginning are considered to be out of
bounds, which will usually result in a `BoundsError` [TODO: Link].

When a subsequence or substring is accessed, out-of-bounds indices
will usually be truncated to be the front or back of the
list. Subsequences are, unless otherwise stated, indexed using
half-open intervals, so the start index is included in the list but
the end index is not. If the end index points to a position at or
before the start index, the result will be the empty sequence or
string.

[TODO: How should this behave when given non-integer indices? Probably
`ArgError`, but the behavior is all over the place right now.]
