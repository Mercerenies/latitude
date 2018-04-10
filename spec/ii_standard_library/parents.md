
# The Parents Object

    Parents := Object clone.

The parents object is a singleton object,
like [the slots object](slots.md), which inspection of an object's
inheritance hierarchy, with correct handling of evaluating objects in
the hierarchy.

## Simple Slots

    Parents toString := "Parents".

## Static Methods

### `Parents origin (object, symbol).`

Locates the object in the argument's inheritance hierarchy where the
slot with the given name is defined. If the slot was defined directly
on the argument, the argument object is returned. Otherwise, if it was
defined on a parent and inherited, then the relevant parent object is
returned. If the slot does not exist on the object, then a `SlotError`
is raised. The `missing` method of the object is not used in this
case, as only slots that actually exist in the hierarchy are checked.

### `Parents above (object, symbol).`

This method identifies the origin of the given slot on the argument
object, as though through `Parents origin`. It then returns a method
which calls the value of the slot on the origin's parent. That is, it
ignores the current value of the slot and attempts to access the
inherited value. Effectively, this method emulates the `super`
construct available in other languages. If the slot's value is a
method, it is returned without calling.

### `Parents hierarchy (object).`

Returns an array containing, in order, all of the objects in the
argument's inheritance hierarchy.
