
# The FileHeader Object

    FileHeader := Object clone.

A `FileHeader` object is created as the result of a call
to
[`Kernel readHeader`](kernel.md#kernel-readheader-filename). `FileHeader`
is a data-only object, in that it has no nontrivial methods defined on
it and is primarily used to store structured data.

## Simple Slots

    FileHeader toString := "FileHeader".
    FileHeader packageName := Nil.
    FileHeader moduleName := Nil.