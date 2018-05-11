
# The Unicode Module

    unicode.lat

The Unicode module defines some basic functionality for getting
Unicode properties of strings and characters.

## Module Methods

### `category (str).`

Returns the Unicode general category, as a `Category` instance, of the
first character of the string `str`. If `str` is the empty string,
`ArgError` is raised.

## The Category Object

    Category := Enumeration clone.

`Category` is an enumeration object representing the possible general
categories of a Unicode character. Its possible values are: `Lu`,
`Ll`, `Lt`, `Lm`, `Lo`, `Mn`, `Mc`, `Me`, `Nd`, `Nl`, `No`, `Pc`,
`Pd`, `Ps`, `Pe`, `Pi`, `Pf`, `Po`, `Sm`, `Sc`, `Sk`, `So`, `Zs`,
`Zl`, `Zp`, `Cc`, `Cf`, `Cs`, `Co`, `Cn`.

### Simple Slots

    Category toString := "Category".

### Methods

#### `Category major.`

Returns a string representing the major category of the general
category. One of: Letter, Mark, Number, Punctuation, Symbol,
Separator, or Other.

#### `Category minor.`

Returns a string representing the minor category of the general
category.

#### `Category name.`

Returns the major category followed by the minor category.

[[up](.)]
<br/>[[prev - The Time Module](time.md)]
