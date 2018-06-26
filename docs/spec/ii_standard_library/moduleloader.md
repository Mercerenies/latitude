
# The Module Loader Object

    ModuleLoader := global clone.

The module loader object manages the loading of modules and storing of
loaded modules.

## Simple Slots

    ModuleLoader toString := "ModuleLoader".

## Methods

### `ModuleLoader loaded.`

Returns a dictionary detailing all of the modules which have been
loaded.

### `ModuleLoader clone.`

Returns a new module loader object, duplicating the `&loaded` slot as
well.

### `ModuleLoader loadPath.`

Returns an array of strings indicating where on the file system the
module loader should search for imports. The built-in module loader
searches the following directories.
 * The `std` directory of the Latitude executable path.
 * Any paths in the `LATITUDE_PATH` environment variable, if such a
   variable exists.
 * The current working directory (`Kernel cwd`), if running within the
   REPL.
 * The directory containing the script file, if the Latitude
   interpreter was invoked with a script file as an argument.

### `ModuleLoader resolveImport (package, path).`

If `path` is already a string, then `path` is returned unmodified. If
it is a symbol, then `loadPath` is searched for a path in which `path`
exists as a file. In this case, `path` will have `".lat"` appended
onto the end before searching. If no appropriate path can be found, a
`ModuleError` is raised. The package name, which should either be a
symbol or nil, is used to resolve ambiguities, in case two modules
with the same name are found belonging to different packages.

[[up](.)]
<br/>[[prev - The Module Object](module.md)]
<br/>[[next - The Number Object](number.md)]
