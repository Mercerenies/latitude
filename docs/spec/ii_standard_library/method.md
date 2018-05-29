
# The Method Object

    Method := Proc clone.

A method object is constructed whenever the literal `{ ... }` syntax
is used. Methods are usually (but not necessarily) evaluating
objects. In particular, the `Method` object itself is not an
evaluating object. Methods are the primary means of adding
functionality to objects, since methods will automatically evaluate
when referenced.

## Simple Slots

    Method toString := "Method".

## Methods

### `Method handle (handler).`

Invokes `self`, in a context which contains an exception handler
`handler`. The `handler` should be a method which takes one
argument. `self` will be called with no arguments. If `self` exits
normally, the return value of `self` will be returned from
`handle`. If `self` raises an exception, the handler `handler` will be
called with the exception as an argument (in addition to any other
handlers), and then the VM will terminate. Note that, if an exception
is raised, there is no circumstance in which `handle` exits
normally. Either the VM will terminate or a continuation jump will
occur, so the return value of `handler` will never be used anywhere.

### `Method resolve (cond) do (handler).`

This method registers an exception handler, as with `handle`, but the
handler is prepared to not only handle the exception but resolve
it. `self` will be called with no arguments, and if an exception
occurs inside of the body of `self` (and no inner handler performs a
continuation jump), then `cond` will be called with one argument: the
exception. If `cond` returns truthy, `handler` will be called with the
same exception as an argument, and its return value will be returned
from `resolve`. If `cond` returns falsy, the exception is propogated
to the next handler as normal. If no exception occurs, the return
value of `self` is returned from `resolve`.

### `Method catch (target) do (handler).`

This method emulates the more traditional try-catch statement in other
languages. `self` is called with no arguments. If no exception occurs,
then the return value of `self` is returned from `catch`. If an
exception occurs and it contains `target` in its hierarchy, the
handler `handler` will be called with one argument, as though by
`resolve`. If the exception is not a subobject of `target`, the
handler will fall through and allow outer handlers to run. In the
former case, the return value of `handler` will be returned from
`catch`.

### `Method catchAll (handler).`

This is equivalent to `#'self catch (Exception, #'handler)`. Notice
that this *does not* catch all objects. It only catches `Exception`
and subobjects thereof.

### `Method default (handler).`

This method is semantically equivalent to `catchAll`. While `catchAll`
is intended to be used in cases where side effects should be performed
to handle an error, this method is intended to be used in cases where
a "default" value should be silently returned in the case of an error,
like `foo := { somethingThatMightFail. } default { 0. }`.

### `Method protect (unwind).`

This is equivalent to `thunk: { }, #'self, #'unwind`. `self` is called
with no arguments. If it exits normally, then the return value is
returned. If a continuation jump exits `self` artificially, then
`unwind` is called, in accordance with the description
in [Thunks](../i_syntax_and_semantics/ch6_controlflow.md#thunks).

### `Method closure.`

This slot initially contains the global scope value `global`. When a
method literal is created, the closure of that particular method is
assigned to this slot. The behavior of the method is undefined if this
slot is reassigned or deleted, but the existing closure object can be
mutated in-place freely.

### `Method call (args ...).`

The `call` method on a method object invokes `self` with the given
argument list. The caller of the `self` invocation is, again, the
`self` object.

### `Method == target.`

If a method is compared for equality, it will be called, and its
result compared for equality. `target`, if a method, will also be
called.

### `Method < target.`

If a method is compared for inequality, it will be called, and its
result compared for inequality. `target`, if a method, will also be
called.

### `Method clone.`

When a method is cloned, the `closure` field is directly assigned into
the new object as well. This is primarily of internal importance, as
`closure` must always exist on the evaluating object being called
directly and not on a parent object.

### `Method shield.`

Returns a non-evaluating `Proc` which has the same code as `self`.
Note carefully that proc objects are always called on themselves, so
methods which use `self` will behave strangely when shielded.

[[up](.)]
<br/>[[prev - The Kernel Object](kernel.md)]
<br/>[[next - The Mixin Object](mixin.md)]
