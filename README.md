# Latitude
## The language that lets you think laterally

Latitude is a work-in-progress prototype-oriented programming language that sports a very dynamic type system, a reflexive runtime with plenty of metaprogramming opportunities, and first-class scoping objects.

## Benefits

 * Simple syntax borrowing from Ruby, Smalltalk, and Erlang
 * Object orientation through prototypes
 * First-class scopes
 * Overridable literals through the `meta` object
 * Automatic garbage collection
 * Flow control via single-use continuations
 * Ruby-style exception handling via `catch` blocks
 * Lazy evaluation through method rules and the `Cached` object
 * Arbitrary precision arithmetic support

## Prototype-Oriented

From Wikipedia:
> Prototype-based programming is a style of object-oriented programming in which behaviour
> reuse (known as inheritance) is performed via a process of cloning existing objects that
> serve as prototypes.

The fundamental idea behind a language like Latitude is that classes are too strict. In traditional class-based languages, it is easy for programmers to get caught up in a strict idea of what a specific object is, by matching it up to its type. In a prototype language, everybody is special. The notion that an object can't do something just because its class says it can't is not a notion that makes sense in a prototype language.

Rather than using inheritance, Latitude uses a special `clone` method to create new objects.

    myCat := Cat clone.

Now the object `myCat` is a new object whose parent object is `Cat`. If `myCat` doesn't know what to do when it receives a specific message, it will delegate automatically to its parent, but any changes made to `myCat` are individual to that object. That is, a programmer can override parent methods on individual objects, not just on whole classes at once.

## Dependencies

Latitude requires a C++14 compiler (tested with GCC 5.3.0), as well as the Boost C++ library, which can be acquired [here](http://www.boost.org/). Once you have these, you should be able to build Latitude.

     $ make BOOST='/path/to/boost/library'

## Usage

This language is still in its very early stages of development, so FUTURE CHANGES WILL BREAK YOUR CODE.
