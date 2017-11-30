
# Chapter 4 - Evaluation

## Object Scope

At any point in the program's execution, there is a lexical and a
dynamic scope. Both of the scopes are first-class objects in
Latitude. The scope objects are stored in register memory.

A lexical scope object's parent is the enclosing lexical scope. In
most cases, this is the lexical scope of the method or module in which
the current scope was defined initially. A dynamic scope object's
parent is the caller's dynamic scope. A scope object's parent can be
modified freely. Doing so will *not* change the internal call stack,
so, for example, changing a dynamic scope object's parent does not
change the caller's dynamic scope; it merely changes what dynamic
variables are inherited into the current scope.

Scope objects should always be traditional objects. The consequences
are undefined if a scope object is made to be part of an inheritance
hierarchy that does not contain `Object`.

A lexical and dynamic scope are said to be paired if the lexical scope
contains a slot called `dynamic` whose value is the dynamic scope, and
the dynamic scope has a slot called `$lexical` whose value is the
lexical scope.

At the top-level, there is a global scope object. This global scope
object has a `global` slot that contains itself.

## Slot Lookup

The act of looking up a slot in the associative array of an object is
simply referred to as directly retrieving a slot from the object.

When an identifier is preceded by an expression, the expression is
evaluated, and the symbol corresponding to the identifier is fully
retrieved from the result of the expression. If the identifier is not
preceded by an expression, the object on which the lookup is performed
is one of the two scope objects, depending on the identifier.

 * If the identifier starts with a dollar-sign (`$`), then the lookup
   is performed on the current dynamic scope object.
 * Otherwise, the lookup is performed on the current lexical scope
   object.

## The Meta Object

There is a slot on every traditional object that is named `meta`. This
is referred to as that object's meta. An object's meta defines several
properties about that object's behavior. For most ordinary
programming, the meta object should not need to be modified, but
changing certain meta properties may be advantageous for those who are
interested in changing properties of the language. Slots on the meta
object can be modified to change these behaviors, but new slots should
not be added by the user.

Meta objects should always be traditional objects, and they should
always have exactly the slots defined on them that `Object`'s meta has
defined. As such, if a user wishes to modify a particular object's
meta, it makes logical sense to clone the parent's meta object and
then modify the clone.

Currently, only the current lexical scope's meta object is
used. However, the meta object may be expanded in the future to have
more effects.

## Full Lookup

A full retrieval, or full lookup, of a slot is a more sophisticated
action than a direct retrieval. A full retrieval is often simply
called a retrieval, as it is the more common case than direct
retrieval.

When fully retrieving a slot corresponding to an identifier from an
object, that object is first checked for direct retrieval. If a slot
corresponding to the given identifier exists in the current object,
then that slot is the result of the full lookup. If it does not exist,
the same identifier is sought on the parent object, and then on its
parent, and so on until the slot is found or a cycle in parents is
detected. If the slot is found in this process, then that slot is the
result of the full lookup.

If seeking the slot by looking in successive parents fails, then a
slot with the name `missing` is looked up, using the same
rules. First, `missing` is sought on the current object, then on the
parent, then the parent's parent, and so on until the slot is found or
a cycle is detected. If the `missing` slot is found, it is called with
the name of the original slot as an argument. The result of this call
is the result of the full lookup.

If neither the slot nor `missing` exists, then the current lexical
scope's meta object is used. A slot called `missed` is sought on the
lexical scope's meta object, then on its parent, and so on until the
slot is found or a cycle is detected. If such a slot is found, it is
called with no arguments, and its result is used as the result of the
full lookup.

`missed` on the current lexical meta object should always exist. If
all of the above attempts fail, then the behavior is undefined.

## Method Calls

Objects, usually but not necessarily methods, can be called. A call is
always performed with a caller and a called object, as well as a
possibly empty argument list. When an object is called, several steps
are performed. First, the arguments are evaluated in order and placed
in register memory.

Next, the slot by the name of `closure` is directly retrieved from the
called object. If `closure` does not exist or if the object is not an
evaluating object, then the called object is the return value. If the
object is an evaluating object and has `closure`, the call continues.

The current dynamic scope is cloned to create a new dynamic scope for
the call. The closure (obtained from the `closure` slot of the
evaluating object) is cloned to create a new lexical scope for the
call. Then several variables are bound in the new scopes.

 * `self` is bound to the caller. `again` is bound to the called
   object. `lexical` and `dynamic` are bound to the newly created
   lexical and dynamic scope objects, respectively. These bindings all
   occur in the newly created lexical scope object.
 * `$lexical` and `$dynamic` are bound to the newly created lexical
   and dynamic scope objects, respectively. These bindings occur in
   the newly created dynamic scope object.

All of the bindings created at this time are assign-protected *and*
delete-protected. Next, the arguments are bound to variables. Note
that the arguments are *not* protected and can be reassigned or
deleted freely by the programmer. The first argument is bound to `$1`,
the second to `$2`, and so on. If there are ten or more arguments, the
latter arguments will be bound to multi-digit variable names, such as
`$10` or even `$100`.

[TODO: Perhaps implementations should be allowed to restrict the
argument count.]

Finally, the body of the method is evaluated in order, in the context
of the new lexical and dynamic scopes. The final statement in the
method determines its return value. If the method is empty, the return
value is the special nil object.

## Argument Evaluation

When arguments in an argument list are evaluated, they are evaluated
in applicative order. That is, the first argument is evaluated first,
then the second, and so on until all the arguments have been
evaluated. As they are evaluated, the accumulating results are stored
in register memory.

Note that a short argument list can either be an argument list
enclosed in parentheses, a single chain expression enclosed in
parentheses, or a literal expression. In the latter two cases, the
argument list is considered to contain one element: the chain
expression or the literal expression.

## Chain Evaluation

A chain consists of a series of terms which are either identifiers or
identifiers followed by argument lists. In some circumstances, a chain
is allowed to be empty, but in others it may be required to have at
least some contents.

Chains are always evaluated left-to-right. Each term of the chain has
access to the evaluation of the entire chain to the left of it.

The first term of a chain (and only the first term) is allowed to be a
quasi-literal expression. In this case, the first term is evaluated as
a quasi-literal expression, not according to the chain rules.

An empty chain evaluates to itself: an empty chain. Note that this is
a special syntactic construct that is not equivalent to any Latitude
object; in particular, an empty chain does *not* evaluate to the nil
object.

When a term of a chain is encountered, the name is fully retrieved as
a slot in the object returned by the left-hand-side chain. If the
left-hand-side chain is empty, the slot is looked up in an object
determined by the rules of [Slot Lookup](#slot-lookup) above. In
either case, the resulting slot's object is called with the argument
list supplied. If no argument list is supplied, the object is called
with an empty argument list. The caller is considered to be the
left-hand-side chain's object, or the appropriate scope object.

## Statement Evaluation

There are several kinds of statements, which will be enumerated
here. A statement that consists only of a quasi-literal expression
will be evaluated as a quasi-literal expression. The other statement
types are detailed below.

### Method Invocation

A method invocation consists of a chain followed by a name, followed
optionally by either a short argument list or a full argument list. In
the latter case, the full argument list must be preceded by a colon
(`:`). The chain is evaluated, then the name is fully retrieved as a
slot on the chain object. The result of the lookup is then called with
the argument list as arguments. The return value from the method is
the return value of the invocation.

During the course of evaluating a method invocation, the evaluated
chain object should be stored in register memory.

### Assignment Operation

An assignment operation consists of a chain, then a name, then either
a colon-equals (`:=`) or a double-colon-equals (`::=`), followed by
another statement. The chain is evaluated, then the right-hand-side
statement. Then the slot with the given name on the chain object is
set to the value indicated by the right-hand-side statement. If the
slot does not exist, it is created. If it exists in a parent object,
it is overriden in the child only. If it exists in the chain object
itself, it is modified to the new value.

In the colon-equals case, the return value of the assignment operation
is the return value of the right-hand-side, after being assigned to
the new slot. In the double-colon-equals case, after the assignment is
made, the slot with the special name `::` is fully retrieved from the
object returned by the right-hand-side statement. This slot is then
called, with the right-hand-side object as the caller and with the
name to which it was assigned as the only argument. The name will be
passed as a symbol object. The return value of the `::` call is the
return value of the assignment in this case.

During the course of evaluating an assignment operation, the evaluated
chain object should be stored in register memory.

### Assignment Method Invocation

An assignment method invocation consists of a chain, followed by a
name, then optionally a short argument list, then an equal-sign (`=`),
and then a single statement. First, the chain is evaluated. Then the
right-hand-side statement is appended to the end of the argument
list. Then the name has an equal-sign appended to it, and the slot
with the new name is fully retrieved from the chain object. That
object is then called with the modified argument list, using the chain
object as the caller. The return value of the statement will be result
of the call.

Specifically, a statement of the form

    className methodName (a, b, c) = d.

will behave equivalently to

    className methodName= (a, b, c, d).

During the course of evaluating an assignment operation, the evaluated
chain object should be stored in register memory.

### Bind Operation

A bind operation consists of a chain, followed by a name, then the
special token `<-`, and finally a single statement. First, the chain
is evaluated. Then the name (as a symbol object) and the
right-hand-side are combined to create a 2-element argument list. The
slot with the name `<-` is fully retrieved from the chain object. If
the chain is empty, then the slot is fully retrieved from the lexical
scope instead. The resulting object is called with the 2-element
argument list, using the chain object or the lexical scope as the
caller. The return value of the statement will be the result of the
call.
