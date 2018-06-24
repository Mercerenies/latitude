
# The FileHeader Object

    FileHeader := Object clone.

A `FileHeader` object is created as the result of a call
to
[`Kernel readHeader`](kernel.md#kernel-readheader-filename). `FileHeader`
is a data-only object, in that it has no nontrivial methods defined on
it and is primarily used to store structured data.

A Latitude file header is a sequence of zero or more lines at the top
of the file beginning with `;;*`. Each line of the header should
consist of a key and a value, separated by spaces. A file header can
contain the following keys.

 * `MODULE` The name of the module, usually the same as the file name
   without the extension. Valid Latitude module names are a subset of
   the valid Latitude identifiers consisting of identifiers which
   contain only alphanumeric characters, as well as `/_-`.
 * `PACKAGE` The name of the containing package. Modules that are
   released together in a single library should share a package name.
   Package names consist of multiple segments separated by `.`, and
   each segment must be a nonempty sequence of alphanumeric or `-`
   characters which does not begin with a number or `-` and does not
   end with a `-`.

The order of the lines
in a header does not matter, but each key can appear at most once in a
header.

Every Latitude module is required to have a module and package name.

Example header:

    ;;* MODULE kinematics
    ;;* PACKAGE com.example.physics

## Simple Slots

    FileHeader toString := "FileHeader".
    FileHeader packageName := Nil.
    FileHeader moduleName := Nil.

[[up](.)]
<br/>[[prev - The Exception Object and Built-in Exceptions](exception.md)]
<br/>[[next - The FilePath Object](filepath.md)]
