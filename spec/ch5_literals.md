
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

/////
