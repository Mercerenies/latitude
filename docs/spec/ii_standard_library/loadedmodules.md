
# The Loaded Modules Object

    &LoadedModules := #<non-traditional object>.

The loaded modules object acts as a dictionary object, maintaining the
loaded modules so that future attempts to load the same module will be
able to simply return the existing module.

## Simple Slots

    &LoadedModules toString := "&LoadedModules".

## Methods

### `&LoadedModules missing (symbol).`

This is equivalent to the `missing` method defined on `Object`. It is
provided so that `Slots has?` will work appropriately on this object.

[[up](.)]
<br/>[[next - The Array Object](array.md)]
