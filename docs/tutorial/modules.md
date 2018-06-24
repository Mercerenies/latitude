
# Modules

Latitude libraries are normally organized into modules. Doing so
provides the user with convenient ways of loading your library and
importing specific names from it.

## Script Files

As we've seen, Latitude scripts can be invoked from the command line
like so.

    $ latitude filename.lats

Latitude script files, by convention, end in `.lats`. A script file
can also be invoked from within a Latitude program or the REPL.

    Kernel load: "filename.lats".

`Kernel` is a global singleton object that contains several methods
for interacting with the Latitude VM at a relatively low level. A
Latitude script is run like any other method in Latitude. It will
receive its own lexical and dynamic scopes<sup><a name="footnote-01a"
href="#user-content-footnote-01f">1</a></sup>, and it will return the
value of the final expression in the file.

    result := Kernel load: "one-plus-one.lats".
    println: result. ; Prints 2

    ;; one-plus-one.lats
    1 + 1.

## Module Files

A module file is formatted slightly differently than a regular script
file in Latitude. For one thing, we use the extension `.lat` rather
than `.lats` for Latitude modules. Second, Latitude modules must
always have a header. The header of a module consists of lines
beginning with `;;*` and containing information about the module.

    ;;* MODULE example-module-name
    ;;* PACKAGE com.example.latitude.tutorial

Latitude package names use a reverse domain name scheme. The module
name should normally be the filename without the extension. If the
module is in a subfolder, then the module name should include forward
slashes `/` to indicate the path from the package's root directory. So
if a file named `foo.lat` was located in the folder `examples` within
`util`, an appropriate header might be

    ;;* MODULE util/examples/foo
    ;;* PACKAGE com.example.latitude.tutorial

The Latitude standard library is separated into two portions: the core
library and the standard modules. The core library is the portion of
the standard library which is imported into the Latitude VM at
startup, containing things like `Kernel` and `Object`. The standard
modules are a set of modules which are available for importing under
the package name `'(latitude)`.

Module files are also loaded slightly differently. When a script file
is executed, it is run like a method and its final statement is its
return value. Modules behave similarly, but the module loading
mechanism expects that the return value be the module object itself.
That is, modules should attach all of their functionality to a single
object and return that object.

Latitude provides a newly-constructed module object to modules which
are loading by means of the `$whereAmI` variable<sup><a
name="footnote-02a" href="#user-content-footnote-02f">2</a></sup>.
While it is by no means required that you use this variable, there are
benefits to doing so, and it provides the user of your module with a
consistent, uniform import interface. As with many dynamic variables,
it is often convenient to place the module object in an
appropriately-named lexical variable. Thus, the usual structure of a
Latitude module is

    ;;* MODULE my-module
    ;;* PACKAGE com.example.latitude.tutorial

    my-module := $whereAmI.

    ;; Attach all of the module's functionality to
    ;; the my-module object ...

    my-module.

## Importing Modules

...

[[up](.)]
<br/>[[prev - Input and Output](io.md)]

<hr/>

<a name="footnote-01f"
href="#user-content-footnote-01a"><sup>1</sup></a> When using `Kernel
load`, the lexical closure of the loaded file will be the global
scope, so any lexically-scoped variables defined in the calling file's
scope will not be visible to the loaded file. If you wish to
explicitly choose the lexical closure of the loaded file, you may use
`Kernel evalFile`, which behaves identically to `Kernel load` except
that it takes a second argument: the lexical closure.

<a name="footnote-02f"
href="#user-content-footnote-02a"><sup>2</sup></a> The dynamic
variable `$whereAmI` can actually be used in several other contexts as
well. When loading a module, the variable is equal to the module
object. When running a Latitude script from the command line, the
variable is equal to the filename, as a string. When running the
Latitude REPL, the variable is equal to the `REPL` object.
