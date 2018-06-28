
# Metaprogramming

One of the primary design goals of Latitude is to provide a strong
metaprogramming toolset. In this chapter, we'll be discussing several
of the tools for reflection and metaprogramming that Latitude gives
programmers access to.

## Tapping Objects

The first tool we'll discuss is minor but very handy. The Latitude
root object provides a `tap` method which takes a block. When an
object receives the `tap` message, it runs the block with itself as
the receiver, then returns itself. It is primarily used to initialize
new objects. This is best demonstrated with an example.

    myObject := Object clone {

      self toString := "myObject".

      self pretty := "My shiny object :)".

      self someMethod := {
        ;; ...
      }.

    }.

In this way, we can construct a new object and initialize all of its
behaviors in one statement, organizing all of the functionality into
one nice, indented block.

## Runtime Slot Access

Latitude provides several methods on the root object to modify slots
based on runtime information. `slot` can be used to access a slot
based on a runtime value, `slot=` can be used to assign, and `slot?`
can be used to check for slot existence. If you recall, variable
scopes in Latitude are objects like any other, so we can use these
methods to dynamically create variables.

    define := {
      lexical caller slot ($1) = Nil.
    }.

    define 'var.
    println: var. ; ==> Nil

Here, we've created a method `define` which, when called, defines a
new variable in the caller's scope. On its own, `define` is not
terribly useful, but we can expand it to include an accessor.<sup><a
name="footnote-01a" href="#user-content-footnote-01f">1</a></sup>

    define= := {
      ;; Define the slot variableName and the accessor variableName=
      target := self.
      var := $1.
      target slot (var) = $2.
      accessor := (var asText ++ "=") intern.
      target slot (accessor) = { target slot (var) = $1. }.
    }.
    define := {
      self define ($1) = Nil.
    }.

First, rather than using `lexical caller`, we simply refer to `self`,
since `define` (and, in this case, `define=`) is a message being
passed to the lexical scope anyway. Then, in addition to making the
variable, we make an accessor whose name ends in an `=`. So

    define 'var = 0.

will define a variable named `var` in the current lexical scope and
also a method `var=` which assigns to that variable. This particular
syntax may look familiar to you; that's because it's exactly the
`local` syntax for declaring local variables. In fact, the code shown
above is very close to the way `local` and `local=` are actually
implemented in Latitude.

Likewise, we can define the built-in `localize` method using the same
techniques. Recall that `localize` binds the value `this` to be the
*current* value of `self`.

    myLocalize := {
      self this := #'(self self).
    }.

Inside of `myLocalize`, `self` is the lexical scope which received the
message, so `self self` is the caller's `self` value.

## Argument List Manipulation

We have already seen that you can use `$*` to get the current argument
list as an iterable collection. However, argument lists can be
manipulated more directly and even mutated. Argument lists can be
constructed with `ArgList clone` and filled with `fillWith`.

    ;; The following are equivalent.
    args := $*.
    args := ArgList clone fillWith ($dynamic).

Once you have the argument list object (either by `fillWith` or `$*`),
you can manipulate it by shifting values on or off it. Mutating an
argument list only affects the scope it was created in, so you don't
have to worry about messing up your caller's dynamic scope. `shift`
will rotate the first argument onto the end of the argument list and
return the rotated value. `unshift` will rotate the final argument
onto the front of the argument list and return the rotated value.

    printArgs := {
      $* visit {
        println: $1.
      }.
    }.
    shiftFirstArg := {
      $* shift.
      printArgs. ; Forward the rotated argument list
    }.
    shiftFirstArg: 1, 2, 3, 4, 5. ; ==> Prints 2, 3, 4, 5, 1

Similarly, you can push values onto front of the argument list with
`unshiftOnto`, which takes an argument and places it at the front of
the argument list.

The inverse operation of `fillWith` is `dropInto`, which takes an
argument list object and places its arguments into the given scope.

    printStoredArgs := {
      ;; Takes an argument list and drops it into the current scope
      $1 dropInto ($dynamic).
      $* visit {
        println: $1.
      }.
    }.
    storeArgs := {
      $*.
    }.
    argList := storeArgs (1, 2, 3, 4, 5).
    argList map! { $1 * 10. }. ; Let's modify the argument list object in-place
    printStoredArgs (argList). ; ==> Prints 10, 20, 30, 40, 50

Now we have the machinery to implement the `takes` syntax. Recall that
`takes` binds dynamic arguments to given lexical names.

    myTakes := {
      ;; Get the caller's dynamic scope
      args := ArgList clone fillWith ($dynamic parent).
      ;; We are defining names in the caller's lexical scope
      target := lexical caller.
      ;; Pair each of the names given with an argument
      $1 clone zip! (args) map! {
        ;; For each pair (given by zip! as a cons cell),
        ;; assign the name
        target slot ($1 car) = $1 cdr.
      }.
    }.

    ;; Sample usage
    myMethod := {
      myTakes '[a, b, c].
      a + b + c.
    }.
    println: myMethod (1, 2, 3). ; ==> Prints 6

## Parent Methods

Often, we would like to override a method while also preserving (but
slightly augmenting) the parent method's behavior, similar to calling
`super()` in some other languages. Latitude provides this
functionality via `Parents above`.

    myObject := Object clone tap {
      self clone := {
        Parents above (myObject, 'clone) call tap {
          ;; Customize the new object
          self someValue := 0.
        }.
      }.
    }.

`Parents above (myObject, 'clone)` returns a procedure object which
will invoke the `clone` method on `myObject`'s parent<sup><a
name="footnote-02a" href="#user-content-footnote-02f">2</a></sup>. We
call that procedure, then "tap" the resulting object to customize it.

Note that we explicitly pass `myObject` to `Parents above` rather than
`self`. We do not want the method above `self` (who could feasibly be
an indirect subobject of some other subobject); we want the particular
implementation of `clone` which falls above `myObject`. If we had
passed `self` and someone else along the inheritance hierarchy had
also passed `self`, the `Parents above` calls would get stuck in a
loop calling the method above `self` over and over again. Thus, to
make this behavior sustainable in a larger inheritance hierarchy, we
pass the object we want to target explicitly.

## Missing Methods

What happens if we try to access a slot which does not exist on an
object? Surely, you've already run into this, either accidentally or
intentionally.

    % Object clone thisSlotDoesNotExist.
    ***** EXCEPTION *****
    SlotError - Could not find slot 'thisSlotDoesNotExist
    <stack trace omitted>

Appropriately, we get an error. However, it is possible to override
this behavior. If an object does not know how to respond to a message,
the Latitude VM will fall back to sending the `missing` message, with
the symbol that was originally intended as an argument. The value
returned from the `missing` message will be used as the slot's value
for this instance only. So we can effectively override the "lookup
failed" behavior.

    mySafeObject := Object clone tap {
      self missing := Nil.
    }.
    println: mySafeObject thisSlotDoesNotExist. ; Prints Nil

A `missing` method which does not wish to emulate the slot should call
the parent's `missing` method explicitly, rather than raising an
exception.

    mySafeObject := Object clone tap {
      self missing := {
        takes '[sym].
        if (sym == 'foo) then {
          Nil.
        } else {
          Parents above (mySafeObject, 'missing) call (sym).
        }.
      }.
    }.

For a useful, real-world example of `missing`, we need to look no
further than Latitude's module system. When a module's names are
imported with `importAll`, a new object is injected into the local
scope, and that new object has a `missing` method which forwards to
the module.

    myImportAll := {
      takes '[module].

      ;; Make a new object in the lexical scope hierarchy
      scope := lexical caller.
      scope parent := scope parent clone.
      target := scope parent.

      target missing := {
        takes '[sym].
        {
          module slot (sym).
        } catch (err SlotError) do {
          Parents above (target, 'missing) call (sym).
        }.
      }.

    }.

To import all of the names into scope, we define a new object in the
inheritance hierarchy. That new object has a `missing` method which
explicitly delegates to the module.

    +----------------------+
    | Old lexical parent   |   +----------------------+              +----------------------+
    +----------------------+<--| New object           |------------->| Target module        |
               ^               +----------------------+ Delegates to +----------------------+
               X               ^
               X              /
    +----------------------+ /
    | Lexical scope        |/
    +----------------------+

## Sigils

We've already briefly mentioned sigils, when using the `~fmt` sigil to
create a format string. Now we'll discuss how to create custom sigils.

The `meta` slot on a lexical scope object stores the current meta
object. The current meta object is used for several purposes, not
least of which is sigil lookup.<sup><a name="footnote-03a"
href="#user-content-footnote-03f">3</a></sup> When an expression of
the form `~name (someExpr)` is encountered, it is converted to the
call

    meta sigil name (someExpr).

As with ordinary method calls, if the argument is a literal, the
parentheses may be omitted, so `~name 1` desugars to `meta sigil name
(1)`. Sigils desugar to ordinary method calls, so they are merely a
syntactic convenience. Aside from `~fmt` (in the `'format` module),
Latitude defines a few other built-in sigils. One of the most useful
is the `~l` sigil for lazy evaluation.

    value := ~l {
      putln: "Doing some expensive calculations... :)".
      42.
    }.
    putln: "Haven't done any work yet.".
    println: value. ; Doing some expensive calculations... :) 42
    println: value. ; 42

`~l` takes a block and returns a new method. The new method will call
the block and cache the result the first time it is invoked. When it
is later invoked, it will return the cached value. In effect, it
constructs a method which emulates lazy evaluation, and since Latitude
methods are invoked the same way variables are accessed, you can treat
it the same way you would treat a variable.

Sigils are stored in the `meta sigil` object. When importing a module,
the `meta` object is never imported, even with `importAll`. For this
reason, if you wish to provide sigils as part of a module, you must
store them in a different place. Every module object has a `sigils`
slot, to which you can attach arbitrary sigils.

    ;;* MODULE sigil-module
    ;;* PACKAGE com.example.latitude.tutorial

    sigil-module := $whereAmI.

    meta sigil mysigil := { ... }.
    sigil-module sigils mysigil := #'(meta sigil mysigil).

    sigil-module.

Then, when importing the module, you can use `importAllSigils` to
explicitly import sigils into the current meta.

    use 'sigil-module importAllSigils.

## Sending Messages

Normally, when we send a message to an object, we send a symbolic
name, and the object chooses how to react to the message. Thus, the
same name may behave differently depending on which object is
receiving the message. A prime example of this is `toString`. Clearly,
sending `toString` to a numerical object is going to have different
behavior than sending `toString` to a string object. However,
sometimes it can be useful to send an explicit set of instructions to
an object, rather than a name. This can be done with `send`. `send`
returns a procedure object which, when called, will invoke its block
with the appropriate caller bound as `self`.

    foo ::= Object clone.
    foo send {
      println: self. ; Prints foo
    } call.

`send` returns a procedure object so that you can explicitly pass
arguments to the constructed procedure if you wish.

    use 'format importAllSigils.

    foo ::= Object clone.
    foo send {
      $stdout printf: ~fmt "Self is ~S and args are (~S, ~S)", self, $1, $2. ; foo, 10, 20
    } call: 10, 20.

More commonly, you will not pass an explicit block to `send` but
rather a method passed in from a caller. For instance, `tap` is
implemented in terms of `send`.

    myTap := {
      takes '[obj, mthd].
      obj send #'(mthd) call.
      obj.
    }.

It can be handy to set up a variable scope before performing a call.
For instance, recall that `loop*` behaves like `loop` except that it
also defines `next` and `last` in the loop body's scope. This behavior
is also implemented in terms of `send`. By calling `by` on the
procedure object, we can supply a setup method. This setup method will
be called before the actual method is called, and it will take two
arguments: the future lexical scope and the future dynamic scope. We
can freely modify these arguments, and those changes will be reflected
in the method call.

For brevity, we'll only be implementing `last` in our loop, but `next`
would be similar.

    myLoop* := {
      takes '[block].
      callCC {
        escapable.
        loop {
          Conditional send #'(content) by {
            $1 last := { return. }.
          } call.
        }.
      }.
    }.

We use the `Conditional` object as the target of the method. There is
not any particular reason for this; we just need some object to be the
receiver, and a singleton flow-control-centric object would seem to be
a sensible candidate. Before running `content`, the call invokes our
setup method, which defines a lexically-scoped `last` method. Remember
that the setup method is passed two arguments: lexical scope and
dynamic scope. So `$1` is the lexical scope argument, on which we
define `last`.

## Summary

Latitude's strength comes in its ability to define new control
structures and influence the language in subtle ways. Now you've seen
many of the tools used to do so.

[[up](.)]
<br/>[[prev - Mixin Objects](mixins.md)]
<br/>[[next - Closing Thoughts](final.md)]

<hr/>

<a name="footnote-01f"
href="#user-content-footnote-01a"><sup>1</sup></a> `asText` converts a
symbol into a string representing the symbol's name. `intern` takes a
string and returns an appropriate symbol. These methods are not quite
inverses of one another, for reasons we haven't discussed yet. `arg
intern asText` will always return the same string originally passed
in, but `arg asText intern` may return a distinct symbol in certain
cases.

<a name="footnote-02f"
href="#user-content-footnote-02a"><sup>2</sup></a> Specifically,
`above` finds the slot with the given name on the object, ignores that
value, and accesses the slot on that object's parent, essentially
ignoring the first layer of lookup.

<a name="footnote-03f"
href="#user-content-footnote-03a"><sup>3</sup></a> The meta object is
also used for operator precedence resolution. Customizing the operator
table is not explicitly discussed in this tutorial, but you can read
about it in the Latitude specification
at
[Chapter 2 - Lexical Structure](/spec/i_syntax_and_semantics/ch2_lexical.md).
