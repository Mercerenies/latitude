
# The Global Object

    global := Object clone.

The global scope object is the eventual parent of all scopes in
Latitude, both lexical and dynamic. It also provides many globally
available functions for general use in Latitude. Obviously, all of the
globally available types and objects are available as slots on this
object. Objects which are linked from the main page are omitted from
this page for brevity.

## Simple Slots

    global ... := Ellipsis.

## Methods

### `global this.`

By default, `this` returns the value `self` of the calling scope
object, effectively making `this` equivalent to `self`. `this` can be
overriden, most frequently by `localize`.

### `global localize.`

When `localize` is called, the calling scope object has its `this`
slot redefined to be equal to its `self` slot. This makes it easier to
keep track of the "current" object being worked on, even when nested
several layers deep.

### `global $*.`

Returns an [`ArgList`](arglist.md) object which contains the arguments
on `self`, treating `self` as an argument-like object. Equivalent to
`ArgList clone fillWith (self)`.

### `global escapable (cont).`

Defines a `return` slot on `self` which contains a method. When called
with an argument, this method will invoke the continuation `cont`,
jumping to the point of the continuation and passing its argument
onward.

### `global local (name) = (value).`

Defines a new variable in the current scope. The variable's name is
`name` and its initial value is `value` (not evaluated). Additionally,
an accessor is defined for the name, as though by `self assignable
(name)`.

### `global local (name).`

Defines a new variable in the current scope with a value of `Nil`.
Equivalent to `self local (name) = Nil`.

### `global $moduleLoader.`

This slot contains the current [`ModuleLoader`](moduleloader.md). It
is dynamically scoped, so that the module loader can be locally
overriden for mocking and testing purposes.

### `global $whereAmI.`

This slot could have one of several values, informing the programmer
of what sort of code is being run. It's intended use is similar to
that of the Python `if __name__ == "__main__"` idiom.

 * If the current file is being loaded as a module, then `$whereAmI`
   will be equal to the provided module object.
 * If the current file is being run as an independent Latitude script,
   then `$whereAmI` will be a string equal to the file's name. Note
   that this *does not include* cases where the current file was run
   by means of `Kernel evalFile`, which does not change `$whereAmI`.
 * If execution is currently in the REPL, then `$whereAmI` will be
   equal to the `REPL` object.

### `global caller.`

This slot will be bound, at every method call, to be the caller's
lexical scope. As such, it constructs a chain of callers which leads
back to `global`. `global`'s own `caller` is always itself.

### `global lexical.`

Returns `self`, the current lexical scope.

### `global $dynamic.`

Returns `self`, the current dynamic scope.

### `global self.`

This name is bound, at every method call, to be the calling
object. `global`'s `self` is equal to `global`.

### `global again.`

This name is bound, at every method call, to be the currently
executing method.

### `global here.`

Returns, without evaluating, `self again`. This is equivalent to
`#'(self again)`.

### `global toString.`

If the calling object is equal to `global`, `"global"` is
returned. Otherwise, `#<Scope>` is returned.

## Static Methods

### `global takes (args).`

This metaprogramming method binds the ordinary dynamically scoped
arguments (`$1`, `$2`, etc) to lexically scoped names. The `args`
argument should be a list of symbols (usually but not necessarily
provided as a literal list). The caller's dynamic scope will be
queried for arguments, and the caller's lexical scope will have the
slots assigned. Effectively, the first element of `args` will be the
name of the slot which receives the value of `$1`, the second element
of `args` will receive `$2`, and so on. If there are more arguments
than symbols, the later arguments are not bound to any lexical
symbol. If there are more symbols than arguments, the later symbols
will be given the value `Nil`.

### `global - arg.`

Returns the additive inverse of `arg`. Equivalent to `0 - arg`.

### `global / arg.`

Returns the multiplicative inverse of `arg`. Equivalent to `1 / arg`.

### `global cons (a, b).`

Constructs a [`Cons`](cons.md) object whose `car` is `a` and whose
`cdr` is `b`. `a` and `b` can be evaluating objects and, in that case,
will not be evaluated.

### `global callCC (mthd).`

Calls `mthd` with one argument. The single argument is
a [`Cont`](cont.md) object which, when invoked, will return execution
to the point just *after* the `callCC` call. The argument to the
`Cont` invocation will be used as the return value of `callCC`. If the
method is exited normally, as opposed to via a continuation jump, the
return value of `mthd` is used as the return value of
`callCC`. Remember that Latitude supports first-class continuations,
so it is possible to *return* the continuation object and call it
later to jump *back* to the point in the code at which it was made.

### `global if (obj) then (block1) else (block2).`

Equivalent to [`Conditional if`](conditional.md#if), begins an
if-statement.

### `global loop (block).`

Loops infinitely, calling `block` at every iteration of the
loop. `mthd` will be called on the `Conditional` object.

### `global loop* (block).`

Equivalent to `loop` except that it supports
the [standard loop macros](../appendix/terms.md#loop-macros).

### `global while (cond) do (block).`

Executes a while-loop statement. `while` returns an intermediate
object, and `do` executes the statement. In one iteration, first
`cond` is checked for truthiness. If it is true, then `block` will be
run and these two steps repeated. If it is false, then the loop is
exited. Both the block and the conditional methods will be executed
with `Conditional` as the caller. The return value of the loop will be
the return value of `block` on its last iteration. If `block` never
executed, then `Nil` is returned.

Remember that `cond` should probably be a method, for if it is not
then its truthiness will be evaluated immediately and never re-checked
during later iterations.

### `global while* (cond) do (block).`

Equivalent to `while` except that it supports
the [standard loop macros](../appendix/terms.md#loop-macros).

### `global loopCall (obj).`

By default, this method returns `obj` without doing any work. It is
useful in looping macros, where it may be overriden locally.
See [`~star`](meta.md#star-method) for details.

### `global cond (body).`

Runs `body` in a modified environment. Within this environment, two
special methods are defined: `when` and `else`.

A statement of the form `when (obj) do (block)` will check whether
`obj` is truthy. If it is, then `block` will be run on `Conditional`
and then the `cond` block will be exited. Otherwise, nothing will
happen. In the former case, `cond` will return the value returned by
`block`.

A statement of the form `else (block)` is equivalent to `when (True)
do (block)`.

If no case triggers, `Nil` is returned.

Example usage:

    cond {
      when (x > 10) do { $stdout putln: "X is large". }.
      when (x > 0) do { $stdout putln: "X is positive". }.
      else { $stdout putln: "X is not positive". }.
    }.

### `global case (expr) do (body).`

Evaluates `expr` (if it is a method), then runs `body` in a modified
environment. In this environment, two special methods are defined:
`when` and `else`.

A statement of the form `when (obj) do (block)` will check whether
`obj =~ expr` is truthy. If it is, then `block` will be run on
`Conditional` and then the `case` block will be exited. Otherwise,
nothing will happen. In the former case, `case` will return the value
returned by `block`.

A statement of the form `else (block)` is equivalent to `when (...) do
(block)`.

If no case triggers, `Nil` is returned. In any case, `expr` is always
evaluated exactly once.

Example usage:

    case (x) do {
      when 1 do { $stdout putln: "one". }.
      when 2 do { $stdout putln: "two". }.
      when 3 do { $stdout putln: "three". }.
      else { $stdout putln: "What is this strange number?". }.
    }.

### `global use (name).`

Imports a module with the given name. `use` calls `resolveImport` on
the current `$moduleLoader` (with `name` as an argument) to determine
what path name to search for. It then reads the header (`Kernel
readHeader`) of the file with that name. If the file's header lacks a
required field (`MODULE` or `PACKAGE`) then a `ModuleError` is
raised.

Otherwise, the *canonical name* of the module is determined, which is
the package name, followed by a single space, followed by the module
name. This canonical name is looked up on `$moduleLoader`'s `&loaded`
dictionary. If a module is found in that dictionary, that module is
returned. Otherwise, a new module object is constructed and assigned
to the dynamic `$whereAmI` variable, and the file with the chosen path
is evaluated as a Latitude script. The returned value from the file
(usually, but not necessarily the constructed module) is placed in
`&loaded` for later access and returned.

If multiple candidate modules have the name `name`, a `ModuleError`
will be raised. In this case, use the full `fromPackage ...` form.

### `global fromPackage (pkg) use (name).`

This is the full form of the `use` method, which imports the module
with the given name from a known package. `pkg` and `name` should both
be symbols. The lookup for the name is done via `resolveImport`, with
the exception that only modules whose package name is equal to `pkg`
will be considered. This is used to disambiguate modules from
different packages which happen to share a name.

Additionally, if several modules are needed from the same package,
`fromPackage` can also take a `do` block, which contains multiple
`use` statements. Within the `do` block, these `use` statements will
only consider modules with a matching name.

Example usage:

    ;; Short form
    fromPackage '(com.example.project) use 'addition.
    ;; Long form
    fromPackage '(com.example.project) do {
      use 'addition.
      use 'multiplication.
    }.

### `global thunk (before, during, after).`

Runs `before`, then `during`, then `after`. `during` is run in a
protected environment, so that even in the case of continuation jumps,
`before` and `after` are run appropriately. For details on the exact
nature of this function,
see [Thunks](../i_syntax_and_semantics/ch6_controlflow.md#thunks).

### `global operator (prec, assoc).`

Constructs an [`Operator`](operator.md) object with the given
precedence and associativity.

### `global proc (block).`

Constructs a [`Proc`](proc.md) object which, when called, executes
`block`.

### `global memo (block).`

Constructs a [`Cached`](cached.md) object which, when called for the
first time, executes `block` and caches the result.

### `global id (arg).`

The identity function. Returns `arg`, without evaluating.

### `global scopeOf (lex, dyn, symbol).`

If `symbol` begins with a `$`, returns `dyn`. Otherwise, returns
`lex`. This method is used to identify, given an arbitrary variable
name, whether lexical or dynamic scope should be used to look up the
given name.

### `global do (block).`

Returns `block`, evaluating it if it is a method.

### `global currentStackTrace.`

Returns a [`StackFrame`](stackframe.md) object representing the point
of execution at which this method was called.

### `global puts (str).`

Delegates directly to `$stdout puts (str)`.

### `global putln (str).`

Delegates directly to `$stdout putln (str)`.

### `global print (str).`

Delegates directly to `$stdout print (str)`.

### `global println (str).`

Delegates directly to `$stdout println (str)`.

## Inner Object Methods

### `global $argv.`

This argument-like object contains the command line arguments passed
to the program, excluding those absorbed by the Latitude VM.

[[up](.)]
<br/>[[prev - The Symbol Object](symbol.md)]
<br/>[[next - The Meta Object](meta.md)]
