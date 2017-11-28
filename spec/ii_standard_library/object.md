
# The Root Object

    Object

This is the root object from which all traditional objects inherit. It
is its own parent and is created by the virtual machine during
startup.

## Simple Slots

    Object parent := Object.
    Object toString := "Object".

## Methods

### `Object clone.`

This method returns a clone of the calling object.

### `Object is (target).`

Returns whether the calling object contains the target object in its
inheritance hierarchy. Note that this relation forms a preorder, not a
partial order as in many languages. That is, it is possible for `a`
and `b` to each fall within the other's inheritance hierarchy without
`a` and `b` being the same object.

### `Object slot (symbol).`

Returns the value of the given slot on the calling object. If the
result is a method, it is *not* called. Note that this method may
result in a call to `missing` if the slot is not found through the
standard full retrieval.

### `Object slot (symbol) = value.`

Sets the value of the given slot on the calling object. If the symbol
is a literal value, this is equivalent to simply using the
colon-equals assignment.

### `Object slot? (symbol).`

Returns whether or not the slot exists on the calling
object. Specifically, this method attempts to access the slot using
`Object slot`. If the access is successful, the return value is
true. If the access attempt throws a `SlotError`, the return value is
false. Any other exceptions or thrown objects are propogated.

### `Object invoke (method).`

This method returns a procedure object. When invoked, the procedure
will call `method` with the arguments supplied to the procedure. The
caller at this time will be the object on which `invoke` was called.

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

### `Object println.`

Prints the object to the screen, by invoking the `toString` method on
it. This is equivalent to

    stdout println: Object.

### `Object dump.`

Prints the object to the screen, as well as all slots that are
accessible from it. The exact format of this output is unspecified, so
it should be used for debugging and testing only. This is equivalent to

    stdout dump: Object.

### `Object me.`

Returns the caller, calling it with zero arguments if it is a method.

[TODO: Who is the caller when the method gets called?]

### `Object === (target).`

Returns whether the object is exactly the same as the target. This
method does not call either the caller or the target, even if they are
methods; it simply checks for pointer equality. This is equivalent to

    Kernel eq: Object, target.

### `Object == (target).`

Returns whether the object is exactly the same as the target. For the
root object, this method is equivalent to `===`. However, subobjects
are encouraged to override this method's behavior to provide better
notions of equality for specific types.

/////
