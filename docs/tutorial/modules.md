
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

The simplest and most common way to import a module is with a `use`
statement.

    use 'my-module.

This will define the name `my-module` in the current lexical scope.
The value of that name will be the object returned from the module
file, usually the module object. Thus, any methods or values attached
to the module object can now be accessed via the `my-module` object.

    use 'my-module.

    my-module someFunctionality (100).

If you use a particular function frequently, you may import it
explicitly.

    use 'my-module import '[someFunctionality].

    someFunctionality (100).

You may also import all names from the module, although this is not
recommended as it can very easily become confusing to determine which
names came from which modules.

    use 'my-module importAll.

    someFunctionality (100).

Modules can also be aliased with `as`. This, in particular, is
especially useful if you have a deeply nested module such as our
`util/examples/foo` example earlier. It would be highly inconvenient
to continue using the name `util/examples/foo` every time you need to
reference the module.

    use 'util/examples/foo as 'foo.

Finally, it is always possible that the same module name could be in
use in multiple packages. In this case, you may explicitly qualify the
module with a package name.

    ;; Short form (for one import)
    fromPackage '(com.example.latitude.tutorial) use 'util/examples/foo.

    ;; Long form (for multiple imports)
    fromPackage '(com.example.latitude.tutorial) do {
      use 'util/examples/foo.
      use 'util/examples/bar.
    }

## Load Path

When a module is loaded with `use` or a similar form, the module's
name is looked up in the load path on the system. The load path is
determined at VM startup and can be accessed via `$moduleLoader
loadPath`. It is built from the following paths.

 * A directory containing all of the standard Latitude modules, such
   as `'format` and `'random`.
 * The current working directory (also accessible through `Kernel
   cwd`), if running in the REPL.
 * The directory containing the script file, if a script file was
   invoked from the command line (i.e., `latitude some-file.lats`)
 * Any directories listed in the environment variable `LATITUDE_PATH`
   (if it exists), separated by semicolons.

If you install a new package in a non-standard location, the
recommended way to inform Latitude of it is by appending the path to
the `LATITUDE_PATH` environment variable. However, if you do not have
permissions to change environemtn variables or for some reason do not
want to, then you can clone the module loader within Latitude.

    $moduleLoader := $moduleLoader clone.
    $moduleLoader loadPath := $moduleLoader loadPath dup. ; Make sure we don't mess up the original
    $moduleLoader loadPath pushBack: "/some/new/path/name/".

It is good practice not to mutate the current module loader but to
instead clone it and change the clone. In doing so, your changes will
only affect the current file and anything imported from the command
file. As soon as the current file finishes executing, the original
module loader will return, so as not to disrupt any other code that
expects the module loader to behave in the default way.

## Summary

You now have an understanding of the basics of creating and loading
modules. There are more customizations you can perform on the module
loader, but most of them are scarcely needed, as the default module
loader suffices for many common programming tasks. In the next
chapter, we'll discuss mixin objects, a way to implement reusable
interfaces of Latitude objects.

[[up](.)]
<br/>[[prev - Input and Output](io.md)]
<br/>[[next - Mixin Objects](mixins.md)]

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
