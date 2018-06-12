
# Variables

At this point, we've talked about how to construct new objects and
define slots on them. Now we'll discuss how local variables and scopes
work in Latitude.

## Local Variables

We haven't explicitly talked about local variables yet. We already
know how to use them; we just haven't explicitly said so yet. This is
because local variables are no different than slots, so we can define
them using the same `:=` syntax.

    % theNumberOne := 1.
    1
    % theNumberOne.
    1

It's worth noting that Latitude is fairly liberal in what it allows as
a variable name. The following characters are never allowed in variable names.

    .,:;()[]{}'"`\

And the following characters cannot begin a variable name but can
appear in the middle of one.

    ~#@0123456789

Aside from these two rules, any nonempty string of printable,
non-whitespace Unicode characters is a valid identifier. All of the
following are valid variable names.

    myShinyNewObject
    isInteger?
    **
    foo/bar
    <<m=~m>>

So, although it is not recommended from a readability standpoint, the
following is a perfectly valid Latitude script.

    <<m=~m>> := "Hello, world!".
    putln (<<m=~m>>).

Now, while we could define all of our variables using `:=` and be
perfectly happy, it does create some difficulties when it comes to
inner scopes. Consider the following script.

    i := 1.
    addOne := {
      i := i + 1.
    }.
    println (i).
    addOne.
    println (i).

*(Aside: We use `println` rather than `putln` here. `putln` always
prints a string, verbatim, whereas `println` calls `toString`. We
can't do `putln` because `i` is a number, not a string)*

`addOne` is a method which, we would hope, adds one to the variable
`i`. Intuitively, one might think that this would print `1` and then
`2`. However, actually running this script produces the initially
counterintuitive result

    $ latitude add-one.lats
    1
    1

As it turns out, `:=` always assigns a variable in the *current*
scope. So when we do `i := i + 1` inside `addOne`, we are making a
*new* local variable inside that scope whose value is `i + 1`. We're
not modifying the original local variable.

We can get around this by declaring the variable to be a local
variable, which also defines an accessor. Notice the `local`
declaration on the first line, and the change from `:=` to `=` on the
third.

    local 'i = 1.
    addOne := {
      i = i + 1.
    }.
    println (i).
    addOne.
    println (i).

Now the script behaves the way we'd like it to.

    $ latitude add-one.lats
    1
    2

`local` defines a variable, just like `:=`, but it also defines an
accessor which can be used to mutate that variable via `=`. We'll have
the tools to talk about this more specifically soon, but for now just
remember to annotate all your local variables with `local`.

## Lexical vs Dynamic Scoping

We need to make a brief aside here about lexical and dynamic scoping.
The difference is best exemplified with a snippet of pseudo-code.

    x <- "Lexical"

    def foo()
      print x
    end

    def bar()
      x <- "Dynamic"
      foo()
    end

    bar()

The question of lexical vs dynamic scoping is what determines whether
this pseudo-program outputs, appropriately, "Lexical" or "Dynamic".

In a lexically scoped language, the `x` in `foo()` would bind to the
global declaration of `x` whose value is `"Lexical"`, which is the
narrowest enclosing scope containing the variable. In a
dynamically scoped language, the `x` in `foo()` would bind to
`bar()`'s declaration of `x` whose value is `"Dynamic"`, which is the
caller's scope.

That is, a lexically scoped language always looks for variables in the
physically enclosing scope, according to the placement of the source
code, while a dynamically scoped language traces up the call stack
looking for variables.

These days, most modern languages subscribe to lexical scoping, as it
is more readable, more intuitive, and easier to compile. However,
dynamic scoping does have its merits in specific situations. For this
reason, Latitude primarily uses lexical scoping but also *supports*
dynamic scoping.

By default, variables in Latitude are lexically scoped. Let's
translate that pseudo-code into Latitude and verify this.

    local 'x = "Lexical".

    foo := {
      putln (x).
    }.

    bar := {
      local 'x = "Dynamic".
      foo.
    }.

    bar.

As expected, running this script will print "Lexical" to the screen.
To enable dynamic scoping, we prefix the variable name with a `$`. Any
variable whose name begins with a `$` is dynamically scoped in
Latitude. Additionally, since dynamic variables are seldom mutated
in-place, the `local` method will never make a dynamic variable, so we
need to switch back to the `:=` syntax.

    $x := "Lexical".

    foo := {
      putln ($x).
    }.

    bar := {
      $x := "Dynamic".
      foo.
    }.

    bar.

Running this modified script will correctly print "Dynamic" to the
screen.

## Scoping Rules

Consider the defining behavior of scopes.

First, consider a lexically scoped language. When a variable is
accessed, the current scope is checked. If the variable does not
exist, the enclosing lexical scope is recursively checked. This
continues until either the variable is located or the global scope is
reached.

Likewise, consider a dynamically scoped language. When a variable is
accessed, the current scope is again checked. If the variable does not
exist, the caller's dynamic scope is recursively checked. This
continues until either the variable is located or the global scope is
reached.

Both of these processes seem superficially similar to Latitude's
inheritance lookup algorithm. In Latitude, this similarity is
codified, as scopes *are* Latitude objects. You can access the current
lexical and dynamic scope objects by, respectively, `lexical` and
`$dynamic`.

    % lexical.
    #<Scope>
    % lexical foo := 1.
    1
    % foo.
    1
    % $dynamic $bar := 2.
    2
    % $bar.
    2.

Being able to treat variable scopes as objects opens up an entirely
new dimension of metaprogramming possibilities. The `parent` of a
lexical scope is the enclosing lexical scope, and the `parent` of a
dynamic scope is the caller's dynamic scope. Additionally, the
`caller` method on a lexical scope object will return the caller's
lexical scope, allowing methods to define variables lexically within
their caller.

## Method Arguments

Up until this point, the only methods we have written have taken zero
arguments. Naturally, with message passing being the primary means of
communication in Latitude, we would like to be able to pass arguments
with a message.

The first "argument" to a method is called `self`, and it always
refers to the recipient of the message.

    foo := Object clone.
    foo bar := { self. }.
    foo bar == foo. ; ==> True

There are several syntaxes for passing other arguments. We've already
used most of them without calling attention to it. Arguments can be
enclosed in parentheses or separated from the method name by a colon.
The following are equivalent.

    foo bar (1, 2, 3, 4).
    foo bar: 1, 2, 3, 4.

The latter form is most often used when a statement consists of a
single expression which has side effects. For example, so far we've
been using `println` with the parenthesized form, as that is more
familiar to users coming from other languages, but it is more common
to see `println` used with a colon to indicate the side effect of
printing.

    println: someValue.

The primary benefit of this is that `:` has a very low syntactic
precedence, so operators like string concatenation (`++`) can be used
without causing issues.

    println: "The value of someValue is " ++ someValue.

Additionally, as an exception to the above rule, if there are no
arguments to a method, the parentheses may be omitted entirely.
Finally, if there is a single argument and it is a literal, such as
`1` or `"ABC"`, it need not be enclosed in parentheses or separated by
a colon. Parentheses may also be omitted if the method's name is an
operator, such as `+` or `*`.

Below is a summary of the rules for how to pass arguments. It is not
expected that you memorize these rules; they're primarily built to
maximize the intuitiveness of the call syntax.

 * Arguments are listed after the method name, either wrapped in
   parentheses or preceded by a colon.
 * If there are no arguments, then neither a colon nor parentheses are
   necessary.
 * If there is exactly one argument and it is a literal, the
   parentheses can be omitted.
 * If there is exactly one argument and the method is an operator, the
   parentheses can be omitted.

Now that we know how to pass arguments, the next question is how to
receive them on the method side. Any arguments passed are stored in
the *dynamic* variables `$1`, `$2`, `$3`, etc.

    addTwoNumbers := { $1 + $2. }.
    addTwoNumbers (3, 4). ; ==> 7

An important side effect of this is that arguments, as dynamic
variables, are always forwarded down the call stack. This makes it
very easy to write a delegator function.

    foo := { $1 + $2. }.
    bar := { foo. }.
    bar (3, 4). ; ==> bar

This especially comes in handy when designing an extension object,
which forwards most of its methods calls to an inner object. It is
never necessary to explicitly forward the arguments, as they are
always implicitly passed onward.

## Argument Renaming

Unless your method is exceedingly short (like our `addTwoNumbers`
above), you will likely be giving lexical names to these dynamic
variables, to make them self-documenting and also to prevent them from
getting corrupted in inner scopes. This pattern is so common that
Latitude provides automatic techniques for doing so.

First, note that all of the basic control flow techniques in Latitude
take a method as an argument. We will discuss these techniques in
detail in the next chapter, but it is worth noting that Latitude has
no explicit notion of a "block", so any time a control structure or
similar method requires user-customizable behavior, that method will
take another method as an argument. As such, it is fairly common to
end up nested several methods deep.

First, since the name `self` is bound *every* time a method is called,
it is difficult to access the `self` value from several layers out.

    someMethod := {
      if (blah) then {
        someBehavior {
          ;; Would like to access someMethod's self
          parent parent self. ; Ugly, but works
        }.
      } else {
        doNothing.
      }.
    }.

Since it is reasonably common to have several nested methods within
one another and only care about one of the `self` values, Latitude
offers the `localize` method. `localize` is a method called on the
lexical scope which, when called, sets `this` equal to `self`. `this`
is not rebound at every scope, so if it is used in an inner scope, it
will still have the same value it did before.

    someMethod := {
      localize.
      if (blah) then {
        someBehavior {
          ;; Would like to access someMethod's self
          this. ; self at the time of the localize call
        }.
      } else {
        doNothing.
      }.
    }.

As for non-`self` arguments, the primary concern with those is that,
being dynamically scoped, they can very easily be corrupted by inner
scopes.

    someMethod := {
      if (blah) then {
        $1 toString.
      } else {
        doNothing.
      }.
    }.

The `$1` in the if-statement does not refer to the first argument of
`someMethod`. It likely refers to the `{ $1 toString. }` method
itself, the first argument of `then`, but this is unspecified. As
such, for any nontrivial methods, it is desirable to convert these
dynamic argument variables into lexical names. This is done with the
`takes` primitive.

    someMethod := {
      takes '[n].
      if (blah) then {
        n toString.
      } else {
        doNothing.
      }.
    }.

`takes` takes an array of symbols and binds each of the arguments to
the corresponding (lexical) symbol. We haven't discussed arrays or the
"literal array" syntax used in the example, but for now that syntax
can be regarded as a "special" part of the `takes` call, and we can
learn what it actually is later.

    takes '[var1, var2, var3].

It is relatively common, at the beginning of any substantial Latitude
method, to see a `takes` call and a `localize` call. By convention,
`localize` and `takes` should always be used at the very beginning of
a method, and, if both are present, `localize` should be listed first.
This makes it easy to read off, at a glance, the number and names of
the formal arguments expected by the method, by looking for a `takes`
call at the top of the method.

## Summary

Now you have a good idea of the way variables, both dynamic and
lexical, work in Latitude, as well as how to pass and forward
arguments to methods. In the next chapter, we'll discuss some basic
flow control tools.

[[up](.)]
<br/>[[prev - The Basics](basics.md)]
