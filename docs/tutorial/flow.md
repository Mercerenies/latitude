
# Simple Flow Control

Like every other nontrivial language, Latitude provides methods for
conditional branching, looping, and the like.

## Do Blocks

The simplest form of flow control, arguably not even flow control, is
the `do` block, which takes a single method and runs it,
unconditionally once.

    do {
      putln "This will be printed once".
    }.

This sort of block is only really useful in situations where some
intermediate local variables are needed, but you don't want those
variables polluting the enclosing scope.

    foo := do {
      a := intermediateValue1.
      b := intermediateValue2.
      ;; ... Some complicated expression involving a and b
    }.
    ;; Now a and b only exist inside the do-block and
    ;; don't pollute the outer scope

## Booleans

Before we start talking about conditionals, it would be prudent to
discuss what constitutes a true and false value. Latitude defines a
`Boolean` "type" and "instances" `True` and `False`. Remember that,
since Latitude is prototype-oriented, `Boolean` is simply an object,
from which the `True` and `False` values are cloned.

Now, any object can be used in a place where a conditional is
expected. When a conditional is evaluated, the `toBool` message is
sent to that object, with no arguments. The result of `toBool` must be
either the `True` or `False` object, and that result determines the
truthiness of the value.

Most built-in objects in Latitude are truthy. In particular, any
numbers, strings, symbols, and methods are truthy (this includes 0 and
""). The only falsy values included in the core Latitude library (that
is, the portion of the standard library that does not require imports)
are the `False` value itself and the special `Nil` object.

`Nil` is a relatively simple object, cloned directly from
`Object`.<sup><a name="footnote-01a"
href="#user-content-footnote-01f">1</a></sup> It behaves as an "empty"
or "nullary" object, in the cases where you need to show that a slot
or a value is well-defined but empty. When checking whether an object
is `Nil`, prefer passing the `nil?` method rather than checking for
equality against the `Nil` object. The reason for this is that it is
possible for programmers to clone `Nil` and make their own nil-like
objects.

By convention, Latitude objects are truthy unless they represent
failure in some sense. `False` is falsy for obvious reasons. `Nil`
usually represents the return value of a failed lookup or a
computation that failed to produce a result, so it is also falsy. The
`'unit-test` module provides a `FailedTest` type, representing a
failed unit test, which is also falsy. In general, when defining your
own types, make them truthy unless you can specifically justify
falsehood.

Latitude objects will respond to the messages `and`, `or`, and `not`
in the logical ways. `(a) and (b)` returns `a` if it is falsy, or `b`
otherwise. `(a) or (b)` returns `a` if it is truthy, or `b` otherwise.
`(a) not` returns `True` if `a` is false and `False` if it is true. By
default, these operators *do not* short-circuit. Like most things in
Latitude, the Boolean operators are simply Latitude methods, so they
follow the normal Latitude argument passing rules. However, these
operators will call any methods they are passed, so if you wish to
enable short-circuit evaluation, simply pass methods instead of flat
values.

    ;; a and b will be evaluated immediately
    (a) or (b).
    ;; b will only be evaluated if a is falsy
    { a. } or { b. }.

This paradigm is used frequently in Latitude to give the user of an
API control over whether or not to stall evaluation of arguments.

One thing to be careful about is the "target" of these short methods.
Passing methods to other methods is incredibly common in Latitude,
and, strictly speaking, every method in Latitude has a `self`.
Sometimes, however, it is not entirely obvious what that `self` *is*.
For example, in the logical operator case, what do you think the
result of `{ self. } or { True. }` will be?

    % { self. } or { True. }.
    Conditional

As it turns out, when a conditional is being evaluated, such as by a
logical operator or, later, by an if-statement, it will always be
called on a specialized `Conditional` object. That conditional object
is not really that useful. It's truthy, so we see it as the result of
`or` rather than `True`, but there's not a whole lot to be done on it.
The point to be made is that some care must be taken when using `self`
in an inner scope, as it is sometimes not immediately obvious what
`self` actually *is*. When in doubt, consult the documentation for the
method you're calling.

## Conditionals

After all that build-up, let's discuss the most basic conditional: the
if-statement. No language would be complete without it.

...

[[up](.)]
<br/>[[prev - Variables](vars.md)]

<hr/>

<a name="footnote-01f"
href="#user-content-footnote-01a"><sup>1</sup></a> If you happen to
run `Parents hierarchy (Nil)` or `Nil parent`, you will likely notice
that there is in fact a third object in the inheritance hierarchy.
`Nil` is in fact cloned from `Object`; that is still true. The third
object you see there is a mixin object, which will be discussed later.
