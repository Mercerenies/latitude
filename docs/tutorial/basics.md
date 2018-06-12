
# The Basics

We'll start with the basics.

    putln "Hello, world!".

You can either copy this code into the REPL or put it in a standalone
file and run it from the command line. In either case, you should see
the text "Hello, world!" printed to the screen.

There's not a whole lot to dissect here. The global method `putln` is
being called with a single argument: a literal string. In Latitude,
statements always end in a dot `.`, and our `putln` statement is no
exception.

## Object Orientation

Now, Latitude is a prototype-oriented programming language. We'll get
to exactly what that means in a bit. But for now, just note that that
means Latitude is object-oriented, in the sense that program behavior
is determined by objects sending messages to other objects. Since
message passing is so fundamental to Latitude, it is denoted
syntactically by juxtaposition.

    Kernel GC total.

Here, we're asking the global Latitude kernel for its garbage
collector instance (`GC`) and then asking that garbage collector how
many total Latitude objects have been allocated for this process. In a
more mainstream language, the equivalent code would look something
like

    Kernel.getGC().total();

In Latitude, methods are automatically called, even if no argument
list is provided. This implies that an empty argument list `()` can be
omitted.

It's also important to note that Latitude is *purely* object-oriented.
This means that everything in Latitude is an object. There are no
"primitive" types. Even simple things like numbers and strings are
objects like any other and can have methods called on them. Let's put
that to the test with a few REPL calls.

    % 1 toString.
    "1"
    % 3.2 isInteger?.
    False
    % "Latitude is fun" length.
    15

In fact, this goes one step further. In Latitude, operators like `+`
and `*` are just methods that happen to have precedence rules.

    % 1 + 3 * 7.
    22

This is calling the `*` method on the `3` object, passing `7` as an
argument. Then the result of that call is being passed as an argument
to the `+` method on the `1` object.

In addition to numbers and strings, Latitude also defines a symbol
type. Symbols are similar to strings, except that they are interned in
a global symbol table to enable constant-time equality comparisons.
Additionally, symbols are used internally as the names of messages,
and when we discuss reflection later, we will learn how to use them to
customize that behavior. For now, though, a primer will suffice.
Symbols are denoted by placing a single-quote character `'` in front
of the name. So, for instance, `'abc`, `'+`, and `'foobar` are valid
symbolic literals. Symbols containing spaces or other "invalid"
characters can be constructed by wrapping the name in parentheses. So
`'(this symbol name is long and has spaces)` is a valid symbol.

## Prototypes

When people hear the phrase "object-oriented", they usually think of
classes. While classical inheritance is indeed the more common
paradigm, it is not the only approach to object orientation. Latitude
subscribes to a prototypical inheritance model. As such, Latitude has
no notion of classes. Every object is just that: an "object". And
rather than having classes inherit from classes, objects inherit from
other objects. An object's inheritance hierarchy can be queried with
the `Parents hierarchy` method (that is, the method `hierarchy` on the
global object `Parents`). Let's take a look at the hierarchy of a
numerical value.

    % Parents hierarchy (3).
    [3, 0.00, Object]

So `3` "inherits" from some sort of special numerical object that
displays as zero, which inherits from the root object `Object`. This
intermediate object, as it turns out, is the object `Number`, the
parent of all numerical literals, and it defines the behavior of all
the mathematical operators like addition and multiplication.

But simply using built-in objects is no fun. Let's make our own
object. New objects are constructed by cloning existing objects, which
creates a derived object. Since we're making a brand new object that
doesn't really behave like any other built-in type, we'll just clone
the root object.

    % foo := Object clone.
    Object

Our new object is called `foo`, but it still prints out as `Object`.
This is because it inherited the `Object toString` method. It is a
distinct object, though, and we can verify this.

    % foo == Object.
    False

Now let's define some slots on our object. Slots tell an object how to
respond to messages that is it given. Right now, our object is a fresh
clone of `Object`, so it will behave just like `Object` for the most
part. However, we can add new behavior or override existing behavior
by defining slots on our new object. This is done with `:=`.

    % foo bar := "It works!".
    "It works!"
    % foo bar.
    "It works!"

Let's override `toString` so that our `foo` object stops looking like
the root object.

    % foo toString := "foo".
    "foo"
    % foo.
    foo

So far, we've only defined slots that have a constant value. When a
slot has a constant value and an object receives the corresponding
message, that constant value is simply returned. However, frequently,
we want to define our own dynamic behavior when a message is received.
Let's define a method on `foo` which generates a fresh random number
whenever it is called. To do this, we'll need to import the `'random`
module.

    % use 'random.
    random

Now, methods in Latitude are denoted with curly braces `{}`. That's
it. There's no `def ...` or `lambda ...` syntax; it's just braces.

    % foo randomly := { (random nextInt) mod 10. }.
    Method

Now, when we send a message whose slot contains a method, that method
is called. A method's final statement always denotes its return value;
there is no explicit "return" keyword in Latitude (although, as we
will see later, there are ways to emulate such functionality if it
becomes necessary). Each time we call `foo randomly`, we should get a
new random value. Of course, we're generating random numbers, so your
output will likely be different from the sample output provided here.

    % foo randomly.
    7
    % foo randomly.
    2
    % foo randomly.
    0

As you can see, it definitely generates a new value each time.

As alluded to before, *everything* is Latitude is an object. This
includes methods. If we want to get the method object rather than
calling it, we use a special "hold" block denoted by `#'`.

    % #'(foo randomly).
    Method

There isn't a whole lot we can do with this method object right now.
But we'll learn some new neat things to do with it later.

## Respecting your Parents

We've glossed over the inheritance model a bit, but now we will go
over the exact algorithm being used. Every object has a `parent` slot.
This slot is, for the most part, just like any other, except that it
can never be deleted.

When a message is passed to an object, a slot with a matching name is
looked up on the target object. If such a slot exists, it will either
be returned directly or called, depending on whether or not it
contains a method. If it does not exist, then the lookup is performed
recursively on the object in the `parent` slot. If the lookup
eventually encounters an object which is its own parent, this object
is considered to be the "root" of the hierarchy, and the search stops.

This is why, on our `foo` object we created earlier, we can send
messages that either `foo` *or* `Object` knows how to respond to.

    % foo randomly.
    6
    % foo == foo.
    True

That second line works because there is a slot `==` on `Object`, and
when the Latitude system can't find a slot with that name on `foo`, it
looks on `Object`.

## Summary

Congratulations! You now know the basics of object orientation in
Latitude. In the next chapter, we'll talk about some basic control
flow structures.

[[up](.)]
<br/>[[prev - Installation](installing.md)]
