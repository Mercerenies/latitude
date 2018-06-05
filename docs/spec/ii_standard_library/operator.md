
# The Operator Object

    Operator := Object clone.

Operator objects are stored in the operator table and used for
precedence resolution. An `Operator` instance has two data slots
associated with it: `prec` for precedence and `assoc` for
associativity. Operators are usually constructed via
the [`global operator`](global.md#global-operator-prec-assoc) method.

## Methods

### `Operator prec.`

Returns the precedence. This slot is usually overridden in instances.
The precedence value must be a nonnegative integer less than 256. It's
default value on the `Operator` object itself is 30.

### `Operator assoc.`

Returns the associativity. This slot is usually overridden in
instances. The associativity should be an element of the
`Associativity` enumeration.

### `Operator toString.`

Returns `"Operator"` if called on the `Operator` object itself.
Otherwise, returns an expression which produces an identical operator,
by means of
the [`global operator`](global.md#global-operator-prec-assoc)
constructor.

## The Associativity Object

    Associativity := Enumeration clone.

Closely related to the `Operator` object is the `Associativity`
enumeration object, defining the values `Left`, `Right`, and `None`.

[[up](.)]
<br/>[[prev - The Root Object](object.md)]
<br/>[[next - The Parents Object](parents.md)]
