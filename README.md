# Latitude
## The language that lets you think laterally

Latitude is a work-in-progress prototype-oriented programming language
that sports a dynamic type system, a reflexive runtime with plenty of
metaprogramming opportunities, and first-class scoping objects.

*Note: For installation instructions, a "Getting Started" guide, and
the standard library documentation, refer
to [the documentation page](https://mercerenies.github.io/latitude/).*

## Benefits

 * Simple syntax borrowing from Ruby, Smalltalk, and Erlang
 * Object orientation through prototypes
 * First-class scopes
 * Automatic garbage collection
 * Full continuation support via `callCC`
 * Ruby-style exception handling via `catch` blocks
 * Support for lazy evaluation
 * Arbitrary precision arithmetic support

## Prototype-Oriented

From Wikipedia:

> Prototype-based programming is a style of object-oriented
> programming in which behaviour reuse (known as inheritance) is
> performed via a process of cloning existing objects that serve as
> prototypes.

In a class-based language, the programmer writes classes and then
constructs objects which belong to those classes. In contract, a
prototype-oriented language provides only objects, not classes. The
programmer writes objects and then constructs additional objects which
behave like the originals. This eliminates a dichotomy between classes
and objects and allows for greater flexibility in programming.

For example, suppose you wanted to have a random number generator.
Since Latitude has no notion of static classes, you might write a
singleton object to contain the random number functionality. Later on,
if you decide that you want to have multiple random number generators
(for thread safety, perhaps), it is trivial to convert this singleton
object into a "class-like" object, from which other objects can be
made.

## Code Samples

Several runnable code examples are available in the
repository
[`/latitude-examples`](https://github.com/Mercerenies/latitude-examples/).

## Dependencies

Latitude requires a C++14 compiler (tested with GCC 5.3.0), the Boost
C++ libraries, Perl, and GNU Flex/Bison. Details on installation can
be found
at
[Installation](https://mercerenies.github.io/latitude/tutorial/installing.html).

## Development Environments

There is an Emacs mode for Latitude in `misc/latitude-mode.el` which
should work out-of-the-box and will highlight Latitude syntax.
Improvements will be on the way to the Emacs mode in the future.

## License

Latitude is copyrighted software belonging to Silvio Mayolo
(Mercerenies). See `LICENSE.txt` for licensing information.
