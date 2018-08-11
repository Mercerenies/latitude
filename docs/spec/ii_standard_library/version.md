
# The Version Object

    Version := Object clone.

Version objects embody Latitude versions. The version object is most
commonly used to check the version of the Latitude VM, but it can also
be cloned and used to version other entities.

## Simple Slots

    Version toString := "Version".
    Version major := #<current Latitude version information>.
    Version minor := #<current Latitude version information>.
    Version build := #<current Latitude version information>.
    Version release := #<current Latitude version information>.

## Methods

### `Version == (other).`

Returns whether the two objects have the same major, minor, build, and
release values.

### `Version < (other).`

Returns whether this version comes before the argument version. The
major version is compared first, then the minor, then the build. Two
objects with the same version number but different release types will
not compare less, equal, or greater.

## The Release Object

    Version Release := Enumeration clone.

The release type slot `release` will contain a value of this
enumeration. The enumeration provides the values `Dev`, `Alpha`,
`Beta`, and `Full`.

[[up](.)]
<br/>[[prev - The Symbol Object](symbol.md)]
<br/>[[next - The Global Object](global.md)]
