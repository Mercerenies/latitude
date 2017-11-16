
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

## Literals

A method literal consists of any number of statements, all terminated
by dots, enclosed in curly braces. A method literal results in a clone
of the method object. The method object is the initial value of the
global `Method` slot. The cloned object will have a primitive field
containing the method's inner code.

A numerical literal will result in a clone of the number object, which
is the initial value of the global `Number` slot. The number object
has a primitive field containing the floating-point value positive
zero. The resulting cloned object will have a primitive field
containing the numerical value of the literal. Note that the at-brace
syntax `@(..., ...)` is also a numerical literal, which results in a
complex number whose real part is listed first and whose imaginary
part is listed second.

A string literal will result in a clone of the string object, which is
the initial value of the global `String` slot. The string object has a
primitive field containing the empty string. The resulting cloned
object will have a primitive field containing the string value
enclosed in the literal. This applies both to raw strings and ordinary
strings, but DSL strings are treated differently, as detailed [TODO:
This].

A symbol literal that begins with a single-quote (`'`) or a
quote-paren (`'(`) will result in a clone of the symbol object. The
symbol object is the initial value of the global `Symbol` slot. The
symbol object has a primitive field containing the empty symbol. The
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

/////

[TODO: We refer to the global version as *the* method object and a
subobject as *a* method object. This is confusing; we need better
terminology for these literals.]
