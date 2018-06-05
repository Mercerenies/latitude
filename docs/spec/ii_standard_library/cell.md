
# The Cell Module

Mono cells are similar to cons cells, provided by the core library,
except that a `Cell` object contains only one value,
where [`Cons`](cons.md) carry two (`car` and `cdr`). However, mono
cells provide the same protections so that evaluating objects can be
safely stored within them.

## The Cell Object

    Cell := Object clone.

A mono cell stores a single value.

### Methods

#### `Cell value.`

Returns the cell's value. Does not evaluate the value if it is a
method.

#### `Cell value = (arg).`

Sets the cell's value. Does not evaluate the argument, even if it is a
method.

#### `Cell toString.`

If called on the `Cell` object itself, returns `"Cell"`. Otherwise,
returns an appropriate representation which includes the value of the
cell, stringified.

#### `Cell iterator.`

Cells are iterable and always contain exactly one element. Returns a
`CellIterator`.

### Static Methods

#### `Cell make (arg).`

Makes a new cell whose value is `arg`, not evaluated.

## The Cell Iterator Object

    CellIterator := Iterator clone.

`CellIterator` iterates over cells, which always contain exactly one
value. `CellIterator` is a mutable iterator.

[[up](.)]
<br/>[[prev - The Meta Object](meta.md)]
<br/>[[next - The Format Module](format.md)]
