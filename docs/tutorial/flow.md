
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
if-statement. No language would be complete without it. In Latitude,
an if-statement looks like this.<sup><a name="footnote-02a"
href="#user-content-footnote-02f">2</a></sup>

    if (conditional) then {
      trueCase.
    } else {
      falseCase.
    }.

This may look quite a bit like a special syntactic form, but it is
not. If-statements are methods like any other. In this case, we are
passing the `if` message to the global scope. The `if` message always
returns a conditional object, on which we then send the `then` message
(which returns the same object) and, subsequently, the `else` message.
However, we don't have to worry about those details most of the time;
we just think of it as a single statement and move on.

The actual behavior of the if-statement is reasonably simple. If the
conditional form is truthy then the true case is returned; otherwise,
the false case is returned. The `else` block is *required* for this
form of conditional.

Latitude also provides single-branch suffix forms called `ifTrue` and
`ifFalse`, which behave in the way you might expect.

    obj ifTrue { body. }.
    obj ifFalse { body. }.

In the first case, the body is called if `obj` is truthy, and no
action is performed otherwise. In the second case, the body is called
if `obj` is falsy, and no action is performed otherwise. `ifTrue` and
`ifFalse` always return the object `obj` which was initially checked
for truthiness.

These suffix forms make it possible to write Smalltalk-style
conditionals, provided the return value is not used. It would,
however, be relatively unusual to see Latitude code written in this
way.

    obj ifTrue {
      putln "It's true".
    } ifFalse {
      putln "It's false".
    }.

In addition to the if-statement, Latitude provides a `cond` form that
should be familiar to Lisp coders.

    cond {
      when (cond1) do { body1. }.
      when (cond2) do { body2. }.
      else { body3. }.
    }.

A `cond` form is the equivalent to a chain of if-else-if calls. The
first `when` which has a truthy conditional will have its body
evaluated and returned. `else`, in this context, is roughly equivalent
to `when (True) do`.

Finally, Latitude supports a `case` statement that behaves similarly
to the `cond` form.

    case (obj) do {
      when (test1) do { body1. }.
      when (test2) do { body2. }.
      else { body3. }.
    }.

Like `cond`, the `case` statement will perform the body associated
with the first matching `when` form. For a statement of the form `when
(test) do { body. }.`, the test `test =~ obj` will be performed to
determine whether the match was successful. `=~` defaults to behaving
just like `==` but can be overridden. In particular, `else` is
equivalent to `when (...) do`, where `...` is the global `Ellipsis`
object, for whom `=~` always returns true.

## Loops

Latitude has two basic looping "primitives", and then several more
abstract constructs are built on top of them. The most basic looping
construct in Latitude is `loop`, which performs its body forever,
unconditionally. If you run the below snippet, just make sure you know
how to exit it. On Unix-like operating systems, an interrupt signal
(CTRL+C) will work.

    loop {
      putln: "This will never stop printing".
    }.

However, more commonly, we would like our loops to terminate at some
point. While it is possible to break from an infinite loop (we will
see a very powerful construct in the next chapter that allows this),
usually we want to terminate the loop normally when some condition is
met. Latitude has a typical while-loop for those purposes.

    while { condition. } do {
      body.
    }.

Note that the condition is enclosed in braces. This is because, like
all of these constructs, `while` is just a method in Latitude. If we
failed to put our condition in braces, then it would be evaluated
once, and the result of that evaluation would be passed to `while`,
whereas with braces the method body will be evaluated at every loop
iteration.

Latitude also provides several abstract looping constructs. There are
too many to cover in this chapter, but here are a few of the basic
ones.

    10 times { putln "This will print ten times.". }
    1 upto 11 do { putln "This will also print ten times.". }.
    11 downto 1 do { putln "This prints ten times as well.". }.

In all three cases, the current iteration will be passed as an
argument to the loop body and can therefore be accessed with `$1`.

    10 times {
      putln: "Iteration number " ++ $1.
    }.

Many of the other looping constructs rely on iterators and
collections, which will be discussed later.

Each of the looping constructs described here (but, for efficiency
reasons, *not* the collection ones which will be described later) has
a variant which ends in an asterisk: `loop*`, `while*`, `times*`,
`upto*`, and `downto*`. This variant behaves like the original
construct except that it also enables exiting the loop prematurely.

Specifically, two additional methods become available: `next` and
`last`. `next` takes no arguments and jumps back to the start of the
loop body. `last` takes a single argument and exits the loop, using
the argument as the loop's return value.

    loop* {
      putln "This will print forever".
      next.
      putln "This one will never print".
    }.

&nbsp;

    loop* {
      putln "This will now only print once".
      last (Nil).
    }.

Keep in mind that the starred forms of the loops are slightly less
efficient than their simpler equivalents. It shouldn't make much of a
difference, but it is ideal to omit the star unless the additional
functionality is actually needed.

## Summary

We've discussed the basic looping and conditional structures in
Latitude. In the next chapter, we'll discuss iterable collections and
several of the looping constructs available on them.

[[up](.)]
<br/>[[prev - Variables](vars.md)]
<br/>[[next - Collections](arrays.md)]

<hr/>

<a name="footnote-01f"
href="#user-content-footnote-01a"><sup>1</sup></a> If you happen to
run `Parents hierarchy (Nil)` or `Nil parent`, you will likely notice
that there is in fact a third object in the inheritance hierarchy.
`Nil` is in fact cloned from `Object`; that is still true. The third
object you see there is a mixin object, which will be discussed later.

<a name="footnote-02f"
href="#user-content-footnote-02a"><sup>2</sup></a> Sometimes, you will
see the conditional block enclosed in braces as a method as well, like
`if { conditional. } then ...`. This works fine as well, since `if`
follows the same rules as `or` and `and` with regard to methods. In
fact, the `trueCase` and `falseCase` don't even strictly have to be
methods, but then both would be evaluated and (in most cases) that
would defeat the purpose of the if-statement.
