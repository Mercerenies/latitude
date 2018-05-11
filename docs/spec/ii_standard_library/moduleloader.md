
# The Module Loader Object

    ModuleLoader := global clone.

The module loader object manages the loading of modules and storing of
loaded modules.

## Simple Slots

    ModuleLoader toString := "ModuleLoader".

## Methods

### `ModuleLoader &loaded.`

This non-traditional subobject of [`&LoadedModules`](loadedmodules.md)
stores the actual modules that have been loaded.

### `ModuleLoader clone.`

Returns a new module loader object, duplicating the `&loaded` slot as
well.

### `ModuleLoader loadPath.`

Returns an array of strings indicating where on the file system the
module loader should search for imports. The built-in module loader
searches in the `std` directory of the Latitude executable path, as
well as in any paths in the `LATITUDE_PATH` environment variable, if
such a variable exists.

### `ModuleLoader resolveImport (path).`

If `path` is already a string, then `path` is returned unmodified. If
it is a symbol, then `loadPath` is searched for a path in which `path`
exists as a file. In this case, `path` will have `".lat"` appended
onto the end before searching. If no appropriate path can be found, a
`ModuleError` is raised.

[[up](.)]
<br/>[[prev - The Module Object](module.md)]
<br/>[[next - The Number Object](number.md)]
