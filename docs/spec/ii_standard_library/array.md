
# The Array Object

    Array := Object clone.

The array is the simplest nontrivial type of collection in
Latitude. It stores elements in its ordinal slots, using a scheme that
allows `O(1)` access and `O(1)` insertion and removal at the beginning
and the end. With `O(n)` insertion and removal anywhere else in the
list.

## Methods

### `Array empty?.`

Returns whether or not the array is empty.

Complexity: `O(1)`

### `Array pushFront (value).`

Pushes the value onto the front of the array, returning the value.

Complexity: `O(1)`

### `Array pushBack (value).`

Pushes the value onto the back of the array, returning the value.

Complexity: `O(1)`

### `Array popFront.`

Removes and returns the front value of the array. Throws a
`BoundsError` if the array is empty.

Complexity: `O(1)`

### `Array popBack.`

Removes and returns the back value of the array. Throws a
`BoundsError` if the array is empty.

Complexity: `O(1)`

### `Array nth (n).`

Returns the nth element of the array, using
the [standard indexing](../appendix/terms.md#indexing) rules. If the
index (after adjusting for the standard rules) is still out of bounds,
then a `BoundsError` is thrown.

Complexity: `O(1)`

### `Array nth (n) = value.`

Assigns the value to the nth position of the array, using
the [standard indexing](../appendix/terms.md#indexing) rules. If the
index (after adjusting for the standard rules) is still out of bounds,
then a `BoundsError` is thrown.

Complexity: `O(1)`

### `Array size.`

Overrides the `size` method of the `Collection` mixin to be constant time.

Complexity: `O(1)`

### `Array join (delimiter).`

Joins together all of the elements in the array (as though using
`toString`), placing `delimiter` between each pair of
elements. `stringify` will be called on `delimiter`.

Complexity: `O(n)`

### `Array joinText (delimiter).`

Joins together all of the elements in the array (as though using
`pretty`), placing `delimiter` between each pair of
elements. `stringify` will be called on `delimiter`.

Complexity: `O(n)`

### `Array toString.`

Returns a string representation of the array, by calling `toString` on
all of the constituent elements, separating each pair of elements with
a comma, and enclosing the result in square brackets.

### `Array == arr.`

Compares `self` to `arr` for element-wise `==` equality, returning
whether or not the two arrays contain the same elements.

Complexity: `O(n)`

### `Array < arr.`

Compares, using lexicographic ordering, `self` to `arr`. All of the
elements should be pairwise comparable with the corresponding element
from the other array.

Complexity: `O(n)`

### `Array clone.`

Clones the array and produces a new array. This is a very slightly
nontrivial task, as some care has to be taken to ensure that bounds
changes to the original array do not automatically shift the new
array. Note that this does *not* deep-copy the elements. It simply
makes a new array with the same internal memory as the original.

Complexity: `O(1)`

### `Array iterator.`

Returns an [`ArrayIterator`](iterator.md#arrayiterator) object which
will iterate over the data in the array. The array iterator is
invalidated if the size of the array is altered (usually via push or
pop operations) but remains valid if individual elements are changed
without changing the size.

[[up](.)]
<br/>[[prev - The Argument List Object](arglist.md)]
<br/>[[next - Booleans and the Nil Object](boolnil.md)]
