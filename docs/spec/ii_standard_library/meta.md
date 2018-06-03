
# The Meta Object

    meta := Object clone.

The meta object is expected to exist on all traditional objects and
all lexical scope objects. It defines certain special behavior that
the interpreter will refer to. The built-in meta object defines
appropriate default behavior. If a clone of the built-in meta object
is used as the lexical meta object, care must be taken that the
methods still behave according to this specification, as many
preconditions are not checked when the VM invokes meta methods.

Further, all of the slots listed here should either be directly
defined on a given meta object or directly defined on one of its
parents. Implementations are free to ignore `missing` when making meta
calls, for efficiency reasons, so the behavior is undefined if a
particular slot on `meta` is intended to be accessed through
`missing`.

## Simple Slots

    meta toString := "meta".
    meta sigil := Object clone.

## Methods

### `meta meta.`

This method should always return the calling meta object. If `meta` is
cloned, this method should be updated so that it returns the cloned
meta object.

### `meta sys.`

This is a subobject which contains implementation-dependent code. All
lexical meta objects are expected to have `sys`, and its value should
never be modified from the original. The contents of `sys` may change
from release to release, so users should never rely on direct calls to
methods or slots within it.

### `meta missed.`

This method is invoked as a last resort during a full retrieval if the
slot does not exist and `missing` does not exist on the object
either. This most often occurs when dealing with non-traditional
objects, which might not have a `missing` slot. The built-in meta
object has a `missed` method which throws a `SystemError` object with
a generic message.

### `meta operators.`

A dictionary mapping symbols to [`Operator`](operator.md) objects.
This slot is used to determine operator precedence when parsing files.

For more details on how this slot is used,
see
[Operator Precedence](../i_syntax_and_semantics/ch2_lexical.md#operator-precedence).

## Sigils

### `~l (method).`

The `~l` sigil constructs a [`Cached`](cached.md) procedure and wraps
it in a method. A `Cached` procedure will compute its contents once
and then, if called again, will return the cached result. The `~l`
sigil wraps this in a method, so that the returned value is
automatically called when the variable is accessed later. In this way,
`~l` can be used to simulate lazy-evaluated values in Latitude.

### `~star (method).`

This sigil provides a convenient way to convert a looping construct
into its starred form. That is, assuming the looping method is written
in the correct way, this method will take an ordinary loop and convert
it to the specialized form which supports
the [standard loop macros](../appendix/terms.md#loop-macros).

Specifically, `~star` constructs a wrapper around `method` which calls
`method` in a specialized environment. In this environment, three
global methods are overridden.

 * `loop` is overridden so that it delegates to `loop*`.
 * `while` is overridden so that it delegates to `while*`.
 * `loopCall` is overridden to perform the proper delegations itself.

The new implementation of `loopCall` expects a `Kernel invoke` or
`Object send` partial object (that is, the procedure object returns by
these methods before `call` is invoked on them). `loopCall` will add a
handler to this partial object which defines the appropriate macros
`next` and `last` to delegate to `$next` and `$last`, respectively.

`loopCall` should be called on the partial invocation object which
will become the loop body and will be executed inside a `while` or
`loop` body.

For example, suppose we wanted to define a loop which runs until the
world ends.

    untilWorldEnds := {
      body := Conditional send #'($1).
      loopCall (body). ; Tell the macro that this is the loop block object.
      while { worldEnded? not. } do {
        loopCall call.
      }.
    }.

    ;; Now we want to define a * form for the loop. Since we used loopCall
    ;; (which does no actual work in the original), we can use the ~star
    ;; sigil here.
    untilWorldEnds* := ~star #'(untilWorldEnds).

[[up](.)]
<br/>[[prev - The Global Object](global.md)]
<br/>[[next - The Cell Module](cell.md)]
