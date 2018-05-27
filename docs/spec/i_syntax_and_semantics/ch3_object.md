
# Chapter 3 - Object Model

## Philosophy

Almost everything in Latitude is an object. This includes data types,
primitive values, variable scopes, object methods, and
continuations. Since Latitude is prototype oriented, objects have a
"type" in a trivial sense, in that there is only one type, to which
all objects belong. Latitude has no notion of classes, interfaces, or
traits that other object-oriented languages have, but the benefits of
these features can be reaped easily using prototype inheritance.

## Memory Space

The Latitude memory model is organized into four parts.

 * An object pool consisting of all of the objects that are reachable
   from a Latitude program, as well as zero or more uninitialized
   objects which may be used in the future.
 * An interpreter state, sometimes referred to as register memory,
   consisting of special internal registers available to the VM which
   contain the current continuation and program state.
 * A global symbol table consisting of a bidirectional map between
   integers and names.
 * A collection of read-only registers, often function tables, which
   are initialized at the start of execution and do not change over
   the course of the program.

## Symbol Table

Each Latitude process maintains a global symbol table. There are three
types of symbols, each of which is stored in the symbol table in a
slightly different way. The three types of symbols are: standard,
generated, and natural.

 * A standard symbol is any symbol which does not satisfy the
   requirements of either a generated symbol or a natural symbol.
   Standard symbols are stored in the symbol table with both the index
   and the name as keys, so that looking up a standard symbol with the
   same name will always yield the same symbol.
 * A generated symbol is a symbol whose name begins with a tilde
   (`~`). Generated symbols are stored in the symbol table with only
   the index as a key and the name as a value. Thus, looking up the
   same name for a generated symbol will produce a *distinct* symbol
   each time it is looked up, but looking up a generated symbol by
   index will produce the same name.
 * A natural symbol is a symbol whose name is nonempty and consists
   only of ASCII digits (`0` through `9`). Natural symbols correspond
   to positive integers and are not stored in the symbol table at all.
   Looking up a natural symbol by name or by index will always produce
   the same natural symbol, computed via a simple but
   implementation-dependent mapping. Natural symbols are frequently
   used for indexing in data structures such as arrays, since symbols
   can be used as slot keys but integers cannot.

## Objects

A Latitude object consists of an associative array mapping symbols to
slots, as well as a primitive field. A slot consists of an object
pointer and a protection mask.

The primitive field can contain a value having any of several
types. At the very least, the primitive field must be able to be
empty, and it must also at least be able to contain the following: a
number, a string, a symbol, a continuation, and a
method. Implementations are free to allow primitive fields to contain
other values as needed.

An object with a primitive field containing a method is called an
evaluating object.

Every object must always have a `parent` slot. An object's `parent`
slot is delete-protected but can be reassigned freely. The value of
the `parent` slot is considered to be the object's parent. An object's
inheritance hierarchy, often referred to simply as the object's
hierarchy, is a list calculated as follows

 * The first object in an object's hierarchy is itself.
 * Each successive object is the previous object's parent.
 * The list continues until a duplicate element is encountered.

`Object` is the root object and, as a consequence, is its own
parent. It is possible to construct objects that do not inherit from
`Object`. These objects are called non-traditional objects, as they
have an object hierarchy that is not of the traditional
form. Conversely, an object that has `Object` in its inheritance
hierarchy is called a traditional object.

Many standard library functions expect objects to have several other
slots, which traditional objects have by default. Thus, care must be
taken when passing non-traditional objects to standard library
functions. Unless otherwise specified, standard library functions
expect only traditional objects to be passed to them. As a convention,
non-traditional objects are often bound to variables beginning with an
ampersand (`&`) to clearly denote them.

## Protection

Every slot can have different protections applied to it. A slot with
no protection can be assigned and deleted by the user freely. A slot
with assignment protection cannot be written to. A slot with deletion
protection cannot be deleted. An implementation is free to provide
more types of protection or to provide more fine-grained versions of
the existing ones, as long as the required functionality is
implemented.

## Cloning Objects

A fundamental part of any prototype-oriented programming language is
the ability to clone objects. When an object is cloned, a new,
distinct object is created from the object pool. The new object is
given only one slot: the `parent` slot. The new object's parent is set
to be the old object. Additionally, the parent object's primitive
field is copied into the new object's primitive field.

[[up](.)]
<br/>[[prev - Chapter 2 - Lexical Structure](ch2_lexical.md)]
<br/>[[next - Chapter 4 - Evaluation](ch4_evaluation.md)]
