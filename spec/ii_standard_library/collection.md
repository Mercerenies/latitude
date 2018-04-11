
# The Collection Mixin Object

    Collection := Mixin clone.

A collection is any structure which can be reasonably iterated with
an [iterator](iterator.md). Specifically, any structure which defines
an `iterator` method returning a conforming iterator object is a valid
collection and is eligible to have this mixin injected.

If `iterator` returns a mutable iterator, then the object can become a
collection without defining any additional behavior explicitly. It is
often useful, however, to override certain methods, such as `visit` or
`map`, to be more efficient than the default implementations. However,
if `iterator` returns an *immutable* iterator, then `map` must be
overridden from `Collection` in order to provide a suitable
implementation. This is because the default implementation of `map`
simply clones the existing collection and mutates its iterator to
produce the new collection.

Finally, note that, in many places in `Collection`'s documentation, a
"function" is referenced. Unless otherwise specified, any procedure
object can be passed as a function, even an evaluating object.

## Simple Slots

    Collection toString := "Collection".
    Collection interface := '[map!, visit, map, foldl, foldr, size, length, toArray, all,
                              any, notall, notany, detect, countIf, count, find, containsIf,
                              contains, zip, zip!, take, drop, maximum, minimum, <>, sum,
                              product, append].

## Methods

### `Collection map! (f).`

This method mutates the collection in-place, by calling `f` once for
each object in the collection and mutating the values of the
collection to be those returned from `f`. This method can *only* be
used on mutable collections.

### `Collection visit (f).`

This method calls `f` once for each object in the collection and then
returns the collection object at the end.

### `Collection map (f).`

Returns a new collection of the same type as the caller, whose
elements are the values returned by calling `f` for each element of
the original collection. Note that the default implementation of `map`
merely clones the calling collection and then calls `map!` on the
result. If this behavior is illogical for a particular collection, or
if the collection in question is immutable, `map` must be explicitly
overriden in the particular case.

### `Collection foldl (base, f).`

Folds the collection from left to right, applying the binary function
`f` and accumulating the results. For example, `[a, b, c] foldl (0,
f)` will result in the calls `f( f( f(0, a), b ), c )`.

Note that `foldl` will never terminate on infinite collections.

### `Collection foldr (base, f).`

Folds the collection from right to left, applying the binary function
`f` and accumulating the results. For example, `[a, b, c] foldl (0,
f)` will result in the calls `f( a, f( b, f(c, 0) ) )`. Beware that,
unlike `foldl`, this method must store many partial computations on
the stack while iterating, so for large data structures, there is a
risk of overflowing the call stack. Additionally, unlike `foldl`, the
second argument passed to the accumulator function will be a
lazy-evaluated value, so infinite data structures can have terminating
`foldr` calls, provided that `f` is lazy in its second argument.

### `Collection size.`

Returns the length of the collection, by iterating over it. If a more
efficient technique can be provided, it is encouraged that collection
objects override this method.

If the collection contains infinite elements, this method will not
terminate.

### `Collection length.`

This method is a synonym for and delegates to `size`. It should not be
overriden.

If the collection contains infinite elements, this method will not
terminate.

### `Collection toArray.`

Returns an array containing all of the elements of the collection.

`toArray` will not terminate on infinite collections.

### `Collection all (f).`

Calls `f` for elements in the collection until it finds an element
which returns falsy. If it finds such an element, the falsy value is
returned. If it fails to find such an element, true is returned.

`all` will terminate on infinite collections for which `f` is not true
for all elements but will fail to terminate if `f` is true for all
elements.

### `Collection any (f).`

Calls `f` for elements in the collection until it finds an element
which returns truthy. If it finds such an element, the truthy value is
returned. Otherwise, false is returned.

`any` will terminate on infinite collections for which `f` is not
false for all elements but will fail to terminate if `f` is false for
all elements.

### `Collection notAll (f).`

Calls `f` for elements in the collection. If it finds an element for
which `f` is falsy, returns true. Otherwise, returns false.

`notAll` will terminate on infinite collections for which `f` is not
true for all elements but will fail to terminate if `f` is true for
all elements.

### `Collection notAny (f).`

Calls `f` for elements in the collection until it finds an element
which returns truthy. If it finds such an element, return
false. Otherwise, returns true.

`notAny` will terminate on infinite collections for which `f` is not
false for all elements but will fail to terminate if `f` is false for
all elements.

### `Collection detect (f).`

Calls `f` for elements in the collection until it finds an element
which returns truthy. If it finds such an element, returns the
element. Otherwise, returns `Nil`. Note that this is very similar to
`any`, but where `any` returns the truthy result of `f`, `detect`
returns the element which produced such a result.

`detect` will terminate for infinite collections, so long as the
collection contains some element for which `f` will return truthy.

### `Collection countIf (f).`

Iterates over the collection, counting every element which satisfies
`f`. The total number of matching elements is returned.

This method will never terminate on infinite collections.

### `Collection count (obj).`

Counts the number of objects which are equal (`==`) to
`obj`. Equivalent to `self countIf { obj == $1. }`.

### `Collection find (obj).`

Returns the first object in the collection which is equal (`==`) to
`obj`. Equivalent to `self detect { obj == $1. }`.

### `Collection containsIf (f).`

Returns whether or not the collection contains an element satisfying
`f`. This is identical to `any` except that the returned value is
normalized to either true or false.

### `Collection contains (obj).`

Returns whether or not the collection contains the given object, by
`==` equality.

### `Collection zip (coll).`

Zips the collection with the argument, producing an array
of [`Cons`](cons.md) objects. The length of the returned array will be
the length of the shorter of the two constituents.

This method will terminate if at least one of the constituents is
finite.

### `Collection zip! (coll).`

Modifies `self` in-place, replacing each element with
a [`Cons`](cons.md) cell. If `coll` is shorter than `self`, then it
will be "padded" with `Nil` in the zipped result.

### `Collection take (n).`

Returns an array containing the first `n` elements of the
collection. `n` must be an integer.

This method will always terminate for infinite collections.

### `Collection drop (n).`

Returns an iterable collection which iterates over the same elements
as `self`, except that it sticks the first `n` elements.

This method will always terminate for infinite collections.

### `Collection maximum.`

Returns the maximum element of the collection, which should be
comparable. If the collection is empty, `Nil` is returned. If multiple
distinct elements compare equal to the maximum, the first such element
is returned.

This method will not terminate on infinite collections.

### `Collection minimum.`

Returns the maximum element of the collection, which should be
comparable. If the collection is empty, `Nil` is returned. If multiple
distinct elements compare equal to the minimum, the first such element
is returned.

This method will not terminate on infinite collections.

### `Collection <> obj.`

Returns a [`Chain`](chain.md) which iterates over the elements of
`self` followed by those of `obj`. `obj` should itself be an iterable
collection.

### `Collection sum.`

Returns the sum (with `+`) of all the elements in the collection,
using `0` as the base case.

### `Collection product.`

Returns the product (with `*`) of all the elements in the collection,
using `1` as the base case.

### `Collection append.`

Returns the product (with `++`) of all the elements in the collection,
using `""` as the base case.

### `Collection empty?.`

Returns whether the collection is empty or not. This will terminate
even for infinite collections.
