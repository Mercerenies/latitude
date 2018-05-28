
# Chapter 6 - Control Flow

## Continuations

Continuations are the primary means of control flow in Latitude. A
continuation is a special value which can be stored in the primitive
field of an object. A continuation value contains a local interpreter
state. It does *not* store the value of any other memory data. In
particular, the object pool, global symbol table, and read-only
register tables are *not* stored in any continuations.

There are two primary operations that can be performed on
continuations: construction and loading. Constructing a continuation
is done simply by taking the current interpreter state and copying it
into a fresh continuation value. Loading a continuation, sometimes
referred to as jumping to a continuation, involves taking the
continuation's interpreter state and replacing the current VM's
registers with the values from the continuation's registers.

At the user level, continuation values are accessible through special
continuation objects. The parent of these objects is the globally
defined continuation object, which has no primitive field and is bound
to the global name `Cont`.

Constructing a new continuation is done using the global method
`callCC`. `callCC` takes a method as its argument. That method will be
invoked with a single argument, which is a newly constructed
continuation. This new continuation contains the register memory that
the VM should have at the end of the `callCC` call. If the method
exits normally, the value returned from the method is used as the
return value for the `callCC` call. However, if the continuation is
invoked (using its `call` method) with exactly one argument, the
continuation's value is loaded into memory, and the program proceeds
as though the `callCC` call had just returned. In this case, the
argument with which the continuation was invoked is used as the return
value of the `callCC` call.

Note that this is only a small fraction of the power of continuations
in Latitude. Many languages support what is called "one-shot"
continuations in the form of try-catch blocks or similar
constructs. However, Latitude supports full first-class
continuations. This means that continuations can be invoked from
anywhere in the code, *not just* inside the original `callCC`
block. Consider the following code block.

    callCC {
      $1 call: Nil.
      "Hello, world!" println.
    }.

This continuation will exit before the print statement is
performed. In this way, `callCC` can be used to implement
return-statements, break-statements, and throw-statements from other
languages. However, continuations can also be invoked from outside the
original block. Consider the following code.

    a := Nil.
    callCC {
      parent a := $1.
    }.
    "Hello, world!" println.
    a call: Nil.

This will print the string, and then the continuation call will move
*backward* to the point in the code just after `callCC`, resulting in
an infinite loop. In this way, continuations can be used to replicate
loops and arbitrary jumps in the code.

Note that while implementations are free to implement standard library
looping constructs using continuations, implementations may also
choose to implement these looping constructs using more efficient,
low-level techniques. As such, while the code sample shown above is
demonstrative, it is also inefficient, and built-in loops should be
preferred when they are powerful enough to express the desired
behavior.

## Hard Kills

A hard kill is a foolproof way to quickly exit the VM. Hard kills can
be performed using the built-in `kill` method on the `Kernel` object,
which takes no arguments. Hard kills are also performed by the VM
itself under some circumstances, as will be detailed in the following
sections.

A VM-level panic is like a hard kill. However, a panic does not exit
silently. It will print various debugging information before
terminating.

## Conditionals and Loops

There is an object defined in global scope with the name
`Conditional`. This object has no special methods defined on
it. However, it is used as the target of many control flow methods in
Latitude.

The standard way to determine whether an object in Latitude is
"truthy" is to call its `toBool` method with no arguments. `toBool` is
defined on all traditional objects, so care must be taken when using
non-traditional objects as the condition in an if-statement or a
similar construct. `toBool` is expected to return either the global
true value or the global false value. The truthiness of an object is
unspecified if `toBool` returns any value other than these two, and
the behavior of the program is undefined if an object which lacks a
`toBool` method altogether is used as a condition.

The most basic built-in control flow method is, appropriately, the
if-statement. The name `if` is globally bound to a method. This method
takes a single argument, which is called. The truthiness of the result
of this call is stored, and a new "then" object is returned. The
"then" object has a method called `then` which takes a single
argument. This argument, the true case, is stored, and a new "else"
object is returned. The "else" object, likewise, has a method called
`else`. When the `else` method is invoked, its argument, the false
case, is stored. Then either the true case or the false case,
depending on the truthiness of the condition value, is called with no
arguments. The chosen case is invoked on the global `Conditional`
object. This results in the convenient syntax

    if { someCondition. } then {
      trueCase.
    } else {
      falseCase.
    }.

The return value of the if-statement is the return value of the case
which was invoked. Note that it is not required that the condition
object be a method, since it will only be invoked once. However, it is
generally good practice to get into this habit, as the loop constructs
which may invoke their condition multiple times *must* be given
methods or the program will behave in unexpected ways.

If a condition is being invoked for side effects and the return value
is unimportant, there is another convenient way to branch. The methods
`ifTrue` and `ifFalse` defined on the root object each take a single
argument. These two methods check whether the caller is truthy or
falsy (respectively) and, if so, call the argument method. If not, no
method is invoked. In either case, the caller is returned.

There are two basic looping constructs that Latitude provides. The
simplest is the `loop` method, which takes a single argument. This
loop construct forms an infinite loop, calling its single argument
forever.

The second, more commonly used looping construct is the
while-loop. The globally-defined `while` method takes a single
argument, which should be a method. This method is the condition. A
new object is returned, on which a `do` method is defined. The `do`
method takes a single method as an argument, which will be the loop
body.

To evaluate a while-loop, the condition will be checked. If it is
truthy, the body will be executed once. Then the condition will be
checked again, and so on. Within the scope of the body method *and*
the condition method, the loop will define a dynamically-scoped
procedure `$break`. This method can be called with one argument, which
will result in the loop immediately ceasing execution. In this case,
the return value of the loop is the value of the argument to the
`$break` procedure.

The resulting while-loop syntax is as follows.

    while { someCondition. } do {
      loopBody.
    }.

Note that the following is most likely incorrect.

    while (someCondition) do {
      loopBody.
    }.

This will check the condition exactly once, and the resulting
condition value will be stored. On later iterations of the loop, the
condition will not be checked again, resulting in a loop that either
runs zero times or forever.

Latitude provides more sophisticated looping and conditional
constructs than these, but they are all built on these primitives. The
more sophisticated methods will be discussed in later chapters on the
standard library.

## Exceptions

Like many languages, Latitude provides an exception-handling
mechanism. Note that the more familiar constructs such as try-catch
blocks are available in the Latitude standard library. However, these
constructs are built upon the primitives discussed in this
section. This section will discuss only the primitive constructs and
the relevant VM details.

Methods can be set up as exception handlers. The interpreter state
stores a stack of exception handlers which is initially empty. The
`handle` method, defined on the literal method object, takes a single
argument. This method pushes the argument onto the handler stack, then
invokes the caller, then pops the argument back off the handler stack.

Any object can be thrown as an exception. When an object is thrown,
usually through the `throw` method on the root object, the exception
handler stack is checked. Then, in order from top to bottom, each
method on the exception handler stack is called. Each method is called
with one argument: the object that was thrown. After this, the VM
performs a panic.

The exception handlers are each called in their own constructed
lexical scope, based on their closure. The dynamic scope is a clone of
the dynamic scope at the time of the `throw` call. Every handler will
be executed in a scope in which all of the outer handlers still exist
on the exception handler stack, but the current handler and any inner
handlers do not exist.

In the ideal case, one of the exception handlers will perform a
continuation jump which escapes from the panic. This is how the
try-catch standard library methods are written to behave.

## Thunks

It is often desirable to protect a block of code in the case of
continuation jumps. A thunk is a generalization of the try-finally
blocks available in many languages. The global method `thunk` takes
three arguments, each of them methods. The first method is referred to
as the "before" method, the third is referred to as the "after"
method, and the second is the body of the thunk. When a thunk is
entered, the before and after methods are stored on a stack in
register memory. Then the before method is called, followed by the
body, then the after method. Each of these is called with no
arguments. Then the before and after methods are removed from register
memory.

Thunks are useful when there is the possibility of performing
continuation jumps which enter or exit the body of the thunk. If a
continuation ever moves control of the code from outside the thunk to
the body of the thunk, the before method will be called immediately
before doing so. Likewise, if a continuation ever moves control of the
code from the body of the thunk to a place outside the thunk, the
after method will be called immediately before doing so.

If there are multiple thunks which need to perform before or after
methods in one jump, they are performed in the following order. First,
all of the required after methods are called, starting with the
innermost thunk. Then all of the required before methods are called,
starting with the outermost thunk.

Note that before and after methods of a thunk are invoked in a special
context. In this context, the dynamic scope is a clone of the dynamic
scope of the `thunk` call. and **not** a clone of the dynamic scope at
the point of the continuation load. As a side effect, before and after
methods of a thunk can **never** perform continuation jumps that
escape their scope. Local continuation jumps that remain within the
current before or after method are allowed, but the behavior is
undefined if a continuation jump attempts to enter or exit a before or
after method of a thunk.

For example, the following is allowed and will behave as expected

    callCC {
      break := $1.
      thunk: { "I will print once" println. }, {
        break call: Nil.
      }, { "I will also print once" println. }.
    }.

However, the following will result in undefined behavior.

    callCC {
      break := $1.
      thunk: { break call: Nil. }, {
        "I may or may not print." println.
      }, { "I also may or may not print" println. }.
    }.

Note also that in the case of a hard kill, before and after methods
are not executed. In this case, the stack is simply ignored and the VM
exits immediately.

[[up](.)]
<br/>[[prev - Chapter 5 - Literals](ch5_literals.md)]
