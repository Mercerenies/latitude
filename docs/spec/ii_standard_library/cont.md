
# The Continuation Object

    Cont := Proc clone.

A `Cont` object represents a continuation, as constructed with
`callCC`. When a continuation is "called", the call never explicitly
returns, as it redirects control over the current instruction pointer
and shifts the position in the code. Details on the continuation
mechanism at the VM level can be found
at
[Continuations](../i_syntax_and_semantics/ch6_controlflow.md#continuations).

## Simple Slots

    Cont toString := "Cont".

## Methods

### `Cont call (arg).`

Invokes the continuation. Code execution is jumped to the point after
that which the continuation was created. The argument `arg` will be
returned from the `callCC` call which created the continuation. A call
to a continuation such as this will not return in the conventional
method return sense, as the flow of control will have shifted to a new
point in the code.

[[up](.)]
