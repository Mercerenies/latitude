
# The Range Object

    Range := Object clone.

A range object is a [`collection`](collection.md) parameterized by a
start position, an end position, and a step value. Note that all
ranges are half-open intervals, so `start` is inclusive and `end`
exclusive.

## Simple Slots

    Range toString := "Range".

## Methods

### `Range start.`

Returns the start value for the range.

### `Range finish.`

Returns the ending value for the range.

### `Range step.`

Returns the step value for the range.

### `Range iterator.`

Returns a [`RangeIterator`](iterator.md#rangeiterator) representing
this collection as an iterator.

### `Range do (block).`

Equivalent to `self visit (block)`.

## Static Methods

### `Range make (a, b, d).`

Constructs a range which starts at `a`, has a finishing value of `b`,
and steps by `d`. All three arguments shall be numbers, with `d`
nonzero.

[[up](.)]
<br/>[[prev - The Process Object](process.md)]
<br/>[[next - The Slots Object](slots.md)]
