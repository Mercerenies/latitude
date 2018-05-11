
# The OS Module

    os.lat

This module provides one static object, `OS`, which carries
information about the host operating system.

## The OS Object

    OS := Object clone.

This object contains information about the current operating system.

### Simple Slots

    OS toString := "OS".

### Static Methods

#### `OS class.`

Returns a symbol representing the class of operating system. Possible
values: `'windows`, `'posix`, or `'unknown`.

[[up](.)]
<br/>[[prev - The Format Module](format.md)]
<br/>[[next - The Random Module](random.md)]
