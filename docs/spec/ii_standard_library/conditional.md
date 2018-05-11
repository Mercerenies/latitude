
# The Conditional Object

    Conditional := Object clone.

The conditional object is an object which is the target of several
methods providing basic flow control to the language. Usually, this
object is not interfaced with directly, instead favoring use of the
global methods with the same names, such as `if` and `while`.

## Simple Slots

    Conditional toString := "Conditional".

## Methods

### `Conditional and (arg).`

Sets the conditional object's truthiness to false unless `arg` is true.

### `Conditional or (arg).`

Sets the conditional object's truthiness to true unless `arg` is false.

### `Conditional then (mthd).`

Stores the true branch for the current conditional.

### `Conditional else (mthd).`

Stores the false branch for the current conditional and calls the
appropriate branch immediately.

## Static Methods

### `Conditional if (obj) then (block1) else (block2).`

Equivalent to the global `if` method, this method constructs a new
partial `Conditional` object. This partially constructed object has
its conditional value set to `obj` (evaluated). On this partially
constructed object, any Boolean operators, like `and` and `or`, can be
called on it, followed by exactly one call to `then` and exactly one
call to `else`. The final `else` call triggers the actual conditional
behavior, which calls either the `then` or `else` argument depending
on the conditional value.

Example usage: `Conditional if (obj) then { case1. } else { case2. }.`

[[up](.)]
<br/>[[prev - The Collection Mixin Object](collection.md)]
<br/>[[next - The Cons Object](cons.md)]
