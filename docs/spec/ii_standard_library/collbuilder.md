
# The Collection Builder Object and Builders

    Collection Builder := Object clone.

A builder is used to construct a specific type of data structure,
usually a collection. The `CollectionBuilder` object is designed to be
the parent of all such collection builders.

Note that, while it is not strictly required that builder objects
inherit from `CollectionBuilder`, it is often logical to do so.
However, a minimal builder object could feasibly inherit from `Object`
directly or even be a non-traditional object. Specifically, collection
builder objects are required to have the following methods defined on
them.

 * `append`
 * `finish`
 * `clone`
 * `toString`

In particular, all Latitude builders must be cloneable, so the
underlying object that is being built must also be cloneable.

## Methods

### `CollectionBuilder append (element).`

Appends an element to the builder's data structure.

### `CollectionBuilder finish.`

Terminates building and returns the finished object. After calling
`finish`, the methods `append` and `finish` may not have meaningful
semantics on the builder object anymore.

## Collection Builders

(stub)

[[up](.)]
<br/>[[prev - The Collection Mixin Object](collection.md)]
<br/>[[next - The Conditional Object](conditional.md)]
