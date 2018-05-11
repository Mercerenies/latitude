
# The Mixin Object

    Mixin := Object clone.

A `Mixin` object is a way to emulate multiple inheritance in
Latitude.

In Latitude, objects fundamentally only have one parent
object. Sometimes, however, it is desirable to "mix in" certain
qualities of other objects, while retaining the original parent. To
allow this, the Latitude `Mixin` object provides a means to "inject"
itself into the hierarchy of another object. This is done by
constructing a new object and placing it in the target's inheritance
hierarchy, and then defining appropriate methods on that object which
delegate explicitly to those defined on the mixin.

## Simple Slots

    Mixin toString := "Mixin".

## Methods

### `Mixin interface.`

This slot consists of a list containing the names of all of the
mixin's public methods. When the mixin is injected into an object, all
of the methods listed here will become available in that object's
inheritance hierarchy.

### `Mixin inject (obj).`

This method injects the mixin into `obj`'s inheritance hierarchy. A
newly constructed object is placed between `obj` and its direct parent
in its inheritance hierarchy. This new object has slots corresponding
to all of the public methods in `self interface`, all of which simply
delegate to the appropriate method defined on the mixin itself.

Additionally, the newly constructed object will have a `toString`
field which delegates to `toString` on the mixin object, as well as a
`mixin` slot whose value is simply the mixin object.

### `Mixin member? (obj)`.

Returns true if `obj` has had `self` injected into its
hierarchy. Specifically, returns true if some object in `obj`'s
inheritance hierarchy has a slot `mixin` whose value is equal (`===`)
to the calling mixin object.

[[up](.)]
