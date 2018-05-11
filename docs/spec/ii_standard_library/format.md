
# The Format Module

    format.lat

The format module defines a printf-like construct for printing
formatted text.

## Module Methods

### `format (string).`

Constructs a `FormatString` object based on the given format
string. Characters in the string are interpreted literally, with the
exception that a tilde (`~`) begins a control operation. The following
control operations are available.

 * `~~` A literal tilde.
 * `~S` Insert the next argument at this position in the string, using
   `toString` to stringify.
 * `~A` Insert the next argument at this position in the string, using
   `pretty` to stringify.

A tilde followed by any other character will result in an
`IntegrityError` being raised.

## The Format String Object

    FormatString := Proc clone.

A format string stores instructions for inserting arguments into a
string and then printing or returning the resulting string object.

### Simple Slots

    FormatString toString := "FormatString".

### Methods

#### `FormatString pretty.`

Returns the textual string which was used to construct this format
string object.

#### `FormatString call (args...).`

Constructs a string based on the format string, using `args...` as the
arguments which will be consumed as the format directives need
arguments. The resulting string is returned.

#### `FormatString printf (args...).`

Prints a formatted string. The rules of `call` are used to produce a
string, which is then printed to the standard output stream.

### Sigils

#### `~fmt (string).`

Constructs a format string, as though by the `format` method.

[[up](.)]
<br/>[[prev - The Enumeration Module](enum.md)]
<br/>[[next - The OS Module](os.md)]
