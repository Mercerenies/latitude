
# Mixin Objects

Latitude is fundamentally a single-inheritance language. There is no
way for an object to have multiple parents. As we will see in the next
chapter, there are some ways to emulate slot lookups to make it *look*
like an object has multiple parents, but at the core a Latitude object
always has exactly one parent.

However, sometimes we would like an object to "inherit" the benefits
of multiple interfaces. For this reason, we package these reusable
interfaces as mixin objects and then inject them into target objects.

## Defining a Mixin

The global Latitude `Mixin` object is the parent of all mixin objects.
Defining a new mixin is done by cloning this object and defining new
behaviors on the new object. Mixins are useful when you have some
common behavior among objects which do not share any common ancestors
(aside from the obvious root object common ancestor).

Suppose we wanted to encapsulate the behavior of values which can be
meaningfully serialized. We might construct a mixin object which
expects an object to be serializable with `serialize` and then defines
a `serializeTo` which dumps the object to a file.

    ;; Expects implementors to have a `serialize` method returning a string
    Serializable ::= Mixin clone.
    Serializable interface := '[serializeTo].

    Serializable serializeTo := {
      takes '[stream].
      stream puts: self serialize.
    }.

Note that we explicitly declare the interface in the `interface` slot.
This slot defines which methods will be copied when the mixin is
injected. To inject the mixin, we make an object which has a
`serialize` method and call `inject`.

    StringWrapper ::= Object clone
    StringWrapper impl := "".
    StringWrapper make := {
      takes '[string].
      self clone tap {
        self impl := string.
      }.
    }.
    StringWrapper serialize := { self impl. }.
    Serializable inject: StringWrapper.

`StringWrapper` will be modified to have a `serializeTo` method. In
this way, we can define behavior, such as `Serializable`, which can be
reused across unrelated objects.

## Built-in Mixins

We have already seen the functionality of the most frequently-used
built-in mixin in Latitude: `Collection`. Any type which defines a
(correct) `iterator` can implement `Collection`, which provides many
convenient iteration functions. We've already seen the simplest of
these, `visit`.

    [1, 2, 3, 4] visit {
      println: $1.
    }.

The `visit` method was injected into the `Array` object's scope from
the `Collection` mixin. `Collection` also provides the usual
functional-style stream techniques: `map`, `foldl`, `foldr`, `filter`,
etc. Additionally, there are several convenience methods for common
types of folds: `sum`, `product`, `append`, `minimum`, `maximum`, etc.

A full list of the collection methods can be found
at [Collection](/spec/ii_standard_library/collection.md).

## Summary

Mixins are a specific type of object which can be easily injected into
other objects' inheritance hierarchy. In this way, we can provide
common behaviors to unrelated objects. In the next chapter, we'll
discuss some of the ways Latitude allows you to use reflection and
metaprogramming to modify the language itself.

[[up](.)]
<br/>[[prev - Modules](modules.md)]
<br/>[[next - Metaprogramming](meta.md)]
