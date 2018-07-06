
# Collections

Latitude provides several collection data types, as well as a
centralized interface for dealing with them.

## Cons Cells

First, cons cells deserve a brief mention. Latitude has a `Cons`
object which behaves like a cons cell in Lisp-like languages. Cons
cells have a `car` and a `cdr` slot. Assignments to these two slots is
done with `=` rather than `:=`.<sup><a name="footnote-01a"
href="#user-content-footnote-01f">1</a></sup> Cons cells can be
explicitly made with `Cons clone` but are usually constructed with
`cons`.

    first := Cons clone.
    putln: first car ++ " " ++ first cdr.
    ;; Prints Nil Nil
    second := cons ("A", "B").
    putln: second car ++ " " ++ second cdr.
    ;; Prints A B

Containers, as well as cons cells, can be used to store methods in a
sanitary way. Generally, when storing a method in a slot, future
access to that slot will evaluate the method unless explicitly
blocked. However, storing methods in a cons cell will allow later
retrievals to get the method back automatically.

    ;; An ordinary object
    first := Object clone.
    first foo := { "Test". }.
    println: first foo. ; Prints "Test"
    ;; A cons cell
    second := cons (Nil, Nil).
    second car = { "Test". }.
    println: second car. ; Prints "Method"

## Arrays

Latitude arrays are cloned off the `Array` object. You can clone from
the `Array` object directly, but it is more common to enclose elements
in square brackets, which will implicitly construct an array.

    ;; A new, empty array
    Array clone.
    ;; Another new, empty array
    [].
    ;; An array containing some elements
    [1, 2, 3, 4, 5, 6].

Arrays can be read from and written to in constant time, via `nth` and
`nth=`. Negative indices will count from the back of the array.

    arr := ["A", "B", "C", "D"].
    println: arr nth (2). ;; Prints "C"
    arr nth (2) = 99.
    println: arr nth (2). ;; Prints 99
    println: arr nth (-2). ;; Prints 99
    println: arr. ;; Prints ["A", "B", 99, "D"]

Additionally, elements can be added or removed from the front or back
of the array in constant time.

    arr := [1, 2, 3].
    arr popBack.
    arr pushBack: 4.
    arr pushBack: 5.
    arr popFront.
    arr pushFront: 0.
    println: arr. ; [0, 2, 4, 5]

Insertion and removal at arbitrary positions in the array is
`O(n)`.<sup><a name="footnote-02a"
href="#user-content-footnote-02f">2</a></sup>

    arr := [1, 2, 3, 4, 5].
    arr insert: 1, 999.
    arr removeOnce! { $1 == 3. }.
    println: arr. ; [1, 999, 2, 4, 5]

## Dictionaries

Dictionaries in Latitude consist of key-value pairs, where the keys
are symbols and the values are Latitude objects. Dictionaries are
cloned from the `Dict` object. As with arrays, you can clone the
`Dict` object directly, but it is mor ecommon to see the dictionary
syntax used. Dictionary syntax is like array syntax except that the
key/value pairs are separated by an arrow `=>`. An empty dictionary
can be constructed with `[=>]`, since `[]` is an empty array.

    ;; A new, empty dictionary
    Dict clone.
    ;; Another new, empty dictionary
    [=>].
    ;; A dictionary containing some elements
    [ 'one => 1, 'two => 2, 'three => 3 ].

Dictionaries are unordered, so the key-value pairs may not display in
the same order that you insert them. Accessing dictionary elements is
done via `get` and `get=`, and checking whether a key exists is done
via `has?`.

    foo := [ 'one => 1 ].
    println: foo get 'one. ; 1
    println: foo has? 'two. ; False
    foo get 'one = -1.
    foo get 'two = 2.
    println: foo has? 'two. ; True
    println: foo. ; [ 'two => 2, 'one => -1 ] (order may vary)

## Array and Dictionary Literals

Latitude provides quite a few metaprogramming facilities, and when
using these facilities, it is often helpful to provide a list of
names, such as variable names. We've already been doing this, with
`takes`.

    foo := {
      takes '[a, b, c].
      ;; ...
    }.

If a single quote precedes an array or dictionary expression, you can
think of the quote on the outside as "distributing" onto any names on
the inside. Any literals which are not names (or which are already
quoted) do not receive the quote.

    '[a, b, c, d] == ['a, 'b, 'c, 'd].
    '[1, 2, foo, 'bar] == [1, 2, 'foo, 'bar].
    '[a => 1, b => 2] == ['b => 2, 'a => 1].
    '[[[[latitude]]]] == [[[['latitude]]]]

## Iterators and Collection Methods

Collection types, such as arrays and dictionaries, implement an
`iterator` method which returns an iterator over the collection.
Iterators respond to `element`, returning the current element; `next`,
moving to the next element; and `end?`, checking whether the iterator
is at its end yet. Additionally, if the iterator is over a mutable
collection, `element=` will set the current element.

Here is a simple loop which prints out each element of the array.

    arr := [1, 2, 3, 4, 5].
    iter := arr iterator.
    while { iter end? not. } do {
      println: iter element.
      iter next.
    }.

However, we almost never operate directly with iterators, as
collections implement many higher-level iteration constructs built on
top of iterators. The simplest of these is `visit`, which calls its
block for each element in the collection. This code snippet behaves
equivalently to the above snippet.

    arr := [1, 2, 3, 4, 5].
    arr visit {
      println: $1.
    }.

Iterating over a dictionary will yield key-value pairs as cons cells.
The key is stored immutably in the car cell, and the value is stored
mutably in the cdr cell, so that changes to the cdr will be reflected
in the dictionary.

    dict := [ 'foo => 1, 'bar => 2 ].
    dict visit {
      $1 cdr = $1 cdr + 1.
    }.
    println: dict. ; [ 'foo => 2, 'bar => 3 ] (order may vary)

The usual functional stream operations, such as folding and mapping,
are available and work as expected.

    arr := [1, 2, 3, 4, 5].
    println: arr foldl (0, { $1 + $2. }). ; 15
    println: arr map { $1 * $1. }. ; [1, 4, 9, 16, 25]

`filter` returns an iterator, which can be converted back into an
array with `to`.

    arr := [1, 2, 3, 4, 5].
    filtered := arr filter { $1 mod 2 == 0. }.
    println: filtered. ; FilterIterator
    println: filtered to (Array). ; [2, 4]

A full list of the collection methods can be found
at [Collection](/spec/ii_standard_library/collection.md).

## Other Collections

In addition to arrays and dictionaries, several other Latitude objects
are iterable. For one thing, strings are iterable by character.
Latitude strings are always encoded in UTF-8 and will iterate
character-by-character.

    "Hello!" visit {
      putln: $1.
    }.
    "Unicode works too! αβγ" visit {
      putln: $1.
    }.

`Nil` is an iterable object and always acts as an empty collection.

`$*` is a method which returns an argument list object. An argument
list object is a collection which contains all of the arguments
currently in scope.

    test := {
      $* visit {
        println: $1.
      }.
    }.
    test: 1, 2, 3, 4, 5. ; Prints each argument on a separate line

Remember that arguments in Latitude are inherited from parent scopes,
so when iterating over `$*`, you may find that there are more
arguments than you expect. Generally, if you write a method which
expects N arguments, you should generally assume that `$*` might
contain additional arguments, and so you should only utilize the
first N.

Finally, if your intention is to iterate from some `n` to some `m`,
then Latitude provides a `Range` type for doing so. We have already
indirectly seen this `Range` type, used in looping statements, but it
is also a full-fledged collection and as such can have any collection
method applied to it.

    10 upto 15. ; Collection consisting of 10, 11, 12, 13, 14
    15 downto 10. ; Collection consisting of 15, 14, 13, 12, 11
    5 times. ; Collection consisting of 0, 1, 2, 3, 4

## Summary

This is not all of the collections in Latitude, but arrays and
dictionaries are among the most commonly used. In the next chapter, we
will discuss a very powerful control construct which can be used to
implement many others.

[[up](.)]
<br/>[[prev - Simple Flow Control](flow.md)]
<br/>[[next - Advanced Flow Control](cont.md)]

<hr/>

<a name="footnote-01f"
href="#user-content-footnote-01a"><sup>1</sup></a> Using `=` calls the
accessor methods `car=` or `cdr=`. By doing so, we update the actual
internal slot where the `car` and `cdr` values are stored, rather than
the `car` / `cdr` methods themselves. This behavior allows us to
safely store methods in a cons cell.

<a name="footnote-02f"
href="#user-content-footnote-02a"><sup>2</sup></a> The `!` in
`removeOnce!` distinguishes it from the method `removeOnce`, which has
the same behavior but returns a *new* array, whereas `removeOnce!`
mutates the array in-place.
