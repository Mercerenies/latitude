
# The Root Object

    Object := #<the root object>.

This is the root object from which all traditional objects inherit. It
is its own parent and is created by the virtual machine during
startup.

## Simple Slots

    Object parent := Object.
    Object toBool := True.
    Object true? := False.
    Object false? := False.
    Object nil? := False.
    Object meta := meta.

## Methods

### `Object clone.`

This method returns a clone of the calling object.

Equivalent `Kernel` call:

    Kernel cloneObject: Object.

### `Object is? (target).`

Returns whether the calling object contains the target object in its
inheritance hierarchy. Note that this relation forms a preorder, not a
partial order as in many languages. That is, it is possible for `a`
and `b` to each fall within the other's inheritance hierarchy without
`a` and `b` being the same object.

[TODO: Equivalent Kernel call?]

### `Object slot (symbol).`

Returns the value of the given slot on the calling object. If the
result is a method, it is *not* called. Note that this method may
result in a call to `missing` if the slot is not found through the
standard full retrieval.

Equivalent `Slots` call:

    Slots hold: Object, symbol.

### `Object slot (symbol) = value.`

Sets the value of the given slot on the calling object. If the symbol
is a literal value, this is equivalent to simply using the
colon-equals assignment.

Equivalent `Slots` call:

    Slots put: Object, symbol, value.

### `Object slot? (symbol).`

Returns whether or not the slot exists on the calling
object. Specifically, this method attempts to access the slot using
`Object slot`. If the access is successful, the return value is
true. If the access attempt throws a `SlotError`, the return value is
false. Any other exceptions or thrown objects are propogated.

Equivalent `Slots` call:

    Slots has: Object, symbol.

### `Object dup.`

Returns a new object identical to `self` but distinct from it. This is
different from cloning an object, which returns a new object whose
*parent* is `self`. This copies the actual slots of `self` into a new
object which shares `self`'s parent.

Equivalent `Kernel` call:

    Kernel dupObject: self.

### `Object invoke (method).`

This method returns a procedure object. When invoked, the procedure
will call `method` with the arguments supplied to the procedure. The
caller at this time will be the object on which `invoke` was called.

Equivalent `Kernel` call:

    Kernel invokeOn: Object, method.

### `Object invokeSpecial (method, modifier).`

This method returns a procedure object. When invoked, the procedure
will call `method` with the arguments supplied to the procedure. The
caller at this time will be the object on which `invoke` was
called.

However, unlike `invoke`, this method adds an additional step to the
call. After the new lexical and dynamic scopes for the method call
have been created, `modifier` will be called with two arguments: the
lexical and dynamic scopes. The modifier method is free to modify
these scopes, usually to add variables which will be in-scope during
the method invocation.

The `invokeSpecial` method allows other methods to be called in
customizable contexts, which creates unique opportunities for
metaprogramming.

Equivalent `Kernel` call:

    Kernel invokeOnSpecial: Object, method, modifier.

### `Object tap (method).`

This method calls its argument, with the calling object as its
caller. It then returns the caller. Like the Ruby method of the same
name, `tap` is intended to capture the frequent idiom in which an
object is constructed, modified slightly, then returned. Compare

    tmp := Object clone.
    tmp someMethod := { ... }.
    tmp someOtherMethod := { ... }.
    tmp.

to

    Object clone tap {
        self someMethod := { ... }.
        self someOtherMethod := { ... }.
    }.

The temporary variable is eliminated.

### `Object throw.`

Throws the current object, triggering any exception handlers that are
in scope.

### `Object rethrow.`

This is equivalent to `throw`. However, this method is *not* overriden
in `Exception` and thus can be used to throw an exception object
without invoking any of the exception-specific behavior, such as
modifying the thrown object's stack trace.

### `Object printObject.`

Prints the object to the screen, by invoking the `toString` method on
it. This is equivalent to

    stdout println: Object.

### `Object dumpObject.`

Prints the object to the screen, as well as all slots that are
accessible from it. The exact format of this output is unspecified, so
it should be used for debugging and testing only. This is equivalent to

    stdout dump: Object.

### `Object me.`

Returns the caller, calling it with zero arguments if it is a
method. In the latter case, the caller of the method is itself.

### `Object === target.`

Returns whether the object is exactly the same as the target. This
method does not call either the caller or the target, even if they are
methods; it simply checks for pointer equality.

Equivalent `Kernel` call:

    Kernel eq: Object, target.

### `Object == target.`

Returns whether the object is exactly the same as the target. For the
root object, this method is equivalent to `===`. However, subobjects
are encouraged to override this method's behavior to provide better
notions of equality for specific types.

### `Object > target.`

This method simply delegates to the call `target < (Object)`.

### `Object >= target.`

This method checks whether the object is greater than the target *or*
the two are equal (according to `==`).

### `Object <= target.`

This method checks whether the object is less than the target *or* the
two are equal (according to `==`).

### `Object /= target.`

Returns the Boolean negation of the result of `Object == target`.

### `Object min (target).`

Returns the caller if it is strictly less than the target, or the
target otherwise.

### `Object max (target).`

Returns the caller if it is strictly greater than the target, or the
target otherwise.

### `Object toString.`

On `Object`, this method returns the constant value
`"Object"`. However, it is often overriden in subobjects to provide a
machine-friendly representation of the data or of some superobject of
the data. Note that `toString` should *always* return a string, and
implementations are free to make this assumption.

### `Object pretty.`

This method delegates to `toString` by default. Subobjects often
override this method to define a "pretty", or user-friendly,
representation of a type while `toString` prints a machine-readable
form of the data. Like `toString`, `pretty` should always return a
string.

### `Object stringify.`

This method simply delegates to the `toString` method on the
caller. However, it is overriden in the `String` object to simply
return itself. As such, `stringify` will return a string
representation of the current object, unless the object is already a
string, in which case it will simply return the object. Users are free
to override `stringify` in subobjects, but the method should always
return a string.

### `Object ++ target.`

This method invokes `stringify` on the caller and the target and
concatenates the resulting strings. As such, it is a convenient way to
join together strings and other values in preparation for printing.

    stdout println: "Good morning, " ++ userName ++ ". It is " ++ currentTime ++ " right now.".

### `Object missing (symbol).`

The default behavior of the `missing` method for traditional objects
is to throw a `SlotError` whose `slotName` is the symbol argument and
whose `objectInstance` is the caller.

### `Object :: (symbol).`

This method defines a `toString` on the caller which has value equal
to the name of the symbol. The caller is then returned. This method is
most frequently invoked using the special assignment operator

    NewObject ::= Object clone.

in which case the symbol argument is passed by the interpreter and
will be the target symbol name `NewObject`. However, it is perfectly
permissible and not uncommon to invoke this method directly.

    Object clone :: 'NewObject tap { ... }.

### `Object falsify.`

By default, newly-cloned subobjects of `Object` will be truthy, since
`Object toBool` evaluates to the true value. This method provides a
convenient way to make a value falsy. `falsify` takes the calling
object and redefines its `toBool` slot to be the false value,
effectively making the value falsy. `falsify` returns the original
object, so that it can be chained with other initialization
techniques. A frequent idiom to set up an appropriate `toString`
method and then perform some initialization might be

    Object clone :: 'NameOfObject tap { ... }.

To make this same object false, simply add `falsify` to the call chain.

    Object clone :: 'NameOfObject falsify tap { ... }.

### `Object ifTrue (method).`

If the caller is truthy, invokes the `method` argument. If the caller
is falsy, performs no action. In either case, the caller is
returned. Note that if the caller is a method, it will be evaluated
when checking for truthiness. If the argument is called, its caller
will be the same as the caller of `ifTrue`.

### `Object ifFalse (method).`

If the caller is falsy, invokes the `method` argument. If the caller
is truthy, performs no action. In either case, the caller is
returned. Note that if the caller is a method, it will be evaluated
when checking for truthiness. If the argument is called, its caller
will be the same as the caller of `ifFalse`.

### `Object and (target).`

If the caller (called as a method) is truthy, then the target (also
called) is returned. Otherwise, the false value is returned. The
target method will *only* be called if the caller is truthy, so by
passing methods to `and`, short-circuiting is enabled.

### `Object or (target).`

If the caller (called as a method) is truthy, then the caller is
returned. Otherwise, the target (also called) is returned. If the
caller is to be returned, it will be called exactly once. If the
caller is a method, the value returned in the truthy case will be the
*result* of the method call. The target method will be called *only*
if the caller is falsy, so by passing methods to `or`,
short-circuiting is enabled.

### `Object not.`

If the caller (called as a method) is truthy, then the false value is
returned. Otherwise, the true value is returned.
