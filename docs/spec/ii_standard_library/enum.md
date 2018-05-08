
# The Enumeration Module

    enum.lat

The enumeration module defines a basic type for implementing
enumerations.

## The Enumeration Object

    Enumeration := Object clone.

This object contains all of the methods of the enumeration module, as
well as the means of defining new enumeration types. It is the parent
of all enumeration types defined by this module.

### Simple Slots

    Enumeration toString := "Enumeration".

### Methods

#### `Enumeration values.`

Returns the list of possible values that this enumeration type can
carry.

#### `Enumeration value (n).`

Returns the `n`th enumeration value (0-based) that this type can
carry. Raises `BoundsError` if out of bounds. This is equivalent to
`self values nth (n).`

### Static Methods

#### `Enumeration of (list).`

Defines a new enumeration type. `list` should be a list of symbols. A
new subclass of `Enumeration` is constructed. For each symbol in
`list`, a slot on this new subobject is defined with the given symbol
name and with value a clone of the new enumeration subobject; these
clones are the instances of the new enumeration type. Each clone has a
`toString` which evaluates to the name to which it is associated.

