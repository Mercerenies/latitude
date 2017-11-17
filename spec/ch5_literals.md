
# Chapter 5 - Literals

## Summary

There are several types of literals and quasi-literals in Latitude. As
Latitude is an object-oriented language, these literals all evaluate
to objects representing them.

## Quasi-Literals

A quasi-literal has one of three forms.

 * A quasi-literal could be simply a literal, in which case it is
   evaluated according to the rules of literals.
 * A quasi-literal could be a statement enclosed in parentheses. In
   this case, the inner statement is evaluated.
 * A quasi-literal could be an uninterned symbol name (a tilde
   followed by a name), followed by another quasi-literal. This form
   is called a sigil expression.

## Sigil Expressions

A sigil expression begins with a tilde (`~`), followed by a name and
then a quasi-literal. A sigil expression desugars to an invocation of
the object (usually a method object) with the given name. The name is
fully retrieved from the current lexical scope's meta object's `sigil`
slot and is invoked with the quasi-literal as an argument. That is, an
expression of the form

    ~name (someExpr).

will desugar to

    meta sigil name (someExpr).

## Literal Objects

There are several predefined objects which are related to
literals. All of these objects are stored in a table in read-only
memory, called the literal object table, or simply the literal
table. Many of these objects can also be accessed using specific names
from global scope.

 * The nil object, accessible from the global `Nil` slot, is a special
   object used to denote the general concept of emptiness.
 * The Boolean object and its two subobjects, the true and false
   objects, define, appropriately, the Boolean notions of true and
   false. They are accessible from, respectively, the global names
   `Boolean`, `True`, and `False`.
 * The string object, number object, and symbol object, denoted as
   `String`, `Number`, and `Symbol`, respectively, in the global
   scope, are used to represent string, numeral, and symbolic
   literals. The literal string object has a primitive field
   containing the empty string, the literal number object has a
   primitive field containing the floating-point value positive zero,
   and the literal symbol object has a primitive field containing the
   symbol with the empty string as its name.
 * The method object, denoted `Method` in the global scope, is used to
   represent method literals defined in the code. Note that, unlike
   string, number, and symbol objects, the method object has an empty
   primitive field and does *not* contain a method in its primitive
   field. This is to prevent the method object from accidentally
   evaluating, resulting in confusing and undesired results.
 * The stack frame object and file header object are used,
   respectively, in reporting stack traces and in loading source code
   files. These are accessible using the respective global names
   `StackFrame` and `FileHeader`. Their usage is discussed in more
   detail in [TODO: This].

Note that these objects will be referred to as "literal objects", or
as "objects from the literal table". This does not imply anything
about the object itself; literal objects are still ordinary objects in
the language. All it implies is that the object can be found in the
literal table. So, for instance, the literal method object is the
object that defines a method in the literal object table, and a method
object is any object that has the literal method object as a direct or
indirect parent.

Implementations are free to place additional entries in the literal
object table, but once the process has finished being started up and
is running user code, objects can neither be added nor removed from
the literal object table, as it is intended to be in read-only memory.

## Literals

A method literal consists of any number of statements, all terminated
by dots, enclosed in curly braces. A method literal results in a clone
of the literal method object. The cloned object will have a primitive
field containing the method's inner code.

A numerical literal will result in a clone of the number object. The
resulting cloned object will have a primitive field containing the
numerical value of the literal. Note that the at-brace syntax `@(...,
...)` is also a numerical literal, which results in a complex number
whose real part is listed first and whose imaginary part is listed
second.

A string literal will result in a clone of the string object. The
resulting cloned object will have a primitive field containing the
string value enclosed in the literal. This applies both to raw strings
and ordinary strings, but DSL strings are treated differently, as
detailed [TODO: This].

A symbol literal that begins with a single-quote (`'`) or a
quote-paren (`'(`) will result in a clone of the symbol object. The
resulting cloned object will have a primitive field containing the
symbol value enclosed in the literal. In the quote-paren case, the
enclosing parentheses are not included in the symbol's name. In either
case, the single-quote is not included in the symbol's name. A symbol
literal that begins with a tilde (`~`) also results in a clone of the
symbol object with an appropriately set primitive field, but in this
case the tilde *is* included in the symbol's name. As a consequence,
the literals `'~abc` and `~abc` evaluate to symbols with the same name
(although they will evaluate to different symbols as the names are
uninterned).

## Lists

[TODO: The way the list syntax is interpreted may change soon, for
efficiency reasons.]

The list syntax consists of an opening bracket (`[`), followed by an
argument list, followed by a closing bracket (`]`). When a list is
evaluated, the `brackets` slot on the current lexical meta object is
called with no arguments. The resulting object from this call is used
as a builder for the list. This object is expected to have at least
`next` and `finish` slots. The builder object's `next` slot is then
called successively, once for each argument in the list, with the
current list element as an argment each time. At the end, the
builder's `finish` slot is called with no arguments. The result of the
`finish` call is used as the result of the list expression. It is
implementation-defined whether the arguments are all evaluated at the
beginning and then `next` is called or each argument is evaluated
immediately before the corresponding `next` call, so long as the
arguments themselves are evaluated in order.

The literal list syntax consists of a single-quote then an opening
bracket (`'[`), followed by a special kind of argument list, then a
closing bracket (`]`). Literal lists are evaluated in the same way as
ordinary lists, with the exception that identifiers which appear in
the literal list are treated as symbol literals. So

    '[abc, 'def, 1, 2, 3].

is equivalent to

    ['abc, 'def, 1, 2, 3].

Notice that the `def` symbol does *not* get double quoted but that the
bare `abc` identifier becomes quoted.

/////
