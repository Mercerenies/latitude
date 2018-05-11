
# The FilePath Object

    FilePath := Object clone.

`FilePath` is a static object which provides several methods for
operating on and accessing different parts of pathnames stored as
strings. Note that all `FilePath` methods assume that `/` is the
directory delimiter, even on Windows. For maximum portability, use `/`
as the delimiter.

## Simple Slots

    FilePath toString := "FilePath".

## Static Methods

### `FilePath directory (path).`

Returns the directory of the pathname. If the path already refers to a
directory, returns the directory which contains the provided path,
effectively stripping off the bottommost directory.

### `FilePath filename (path).`

Returns the filename in `path`, excluding the directory but including
any file extension.

### `FilePath rawname (path).`

Returns the filename in `path`, without the extension. If no extension
exists, this is equivalent to `filename`.

### `FilePath extension (path).`

Returns the file extension in `path`. If no extension exists, returns
the empty string.

[[up](.)]
