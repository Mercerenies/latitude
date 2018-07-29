
# The Unit Testing Module

    unit-test.lat

This module provides basic testing capabilities. Modules which are
designed to be used as unit tests should implement the mixin
`TestModule` and provide the desired unit tests with `addTest`.

## Module Methods

### `truthy (block).`

Runs the block. If the block produces a truthy result, that result is
returned. Otherwise, a `FailedTest` object with an appropriate message
is returned.

### `eq (a, b).`

Compares (without evaluating) `a` and `b` for `==` equality. If they
are equal, `True` is returned. Otherwise, a `FailedTest` object with
an appropriate message is returned.

### `throws (exc) do (block).`

Runs the block. If the block throws an exception of the type `exc`,
then `True` is returned. Otherwise, an appropriate `FailedTest` object
is returned.

## The Test Module Mixin Object

    TestModule := Mixin clone.

A test module is a module which provides unit tests. Test modules
still function like regular modules, simply with the added feature of
having a collection of runnable tests.

### Simple Slots

    TestModule interface := '[clone, addTest, importTestsFrom, runTests].

### Methods

#### `TestModule unitTests.`

Returns the array of unit tests the module provides.

#### `TestModule clone.`

Cloning a test module also duplicates its `unitTests` slot.

#### `TestModule inject (target).`

Injecting `TestModule` into a target object has the added side effect
that the target object receives a `unitTests` slot whose value is a
newly allocated array. Additionally, the injection provides the target
object with a `toString` method whose behavior is as follows: If
`toString` is called on the injector object itself, then
`"TestModule"` is returned; otherwise, the parent `toString` method is
called. This latter behavior is to ensure that `Parents hierarchy`
will continue to look neat for `TestModule`-injected objects. Consider
the behavior of `hierarchy` on an average test module.

    Parents hierarchy: use 'some-test-module.
    ;; In current versions of Latitude
    ==> [some-test-module, TestModule, Module, Object]
    ;; Without the specialized toString behavior
    ==> [TestModule, TestModule, Module, Object]

This is because the usual `toString` for a module is inherited from
`Module`, which would get overriden by the `TestModule` injection.

#### `TestModule addTest (name) do (block).`

Adds a new test to the current module. The name should be a symbol,
and the block should be a method. Two distinct unit tests on the same
module should not share a name.

#### `TestModule importTestsFrom (module).`

Adds all of the tests belonging to the test module `module` to `self`.

#### `TestModule runTests.`

Runs all of the unit tests contained within this module. Each unit
test in `self unitTests` is run via its `call` method. If an uncaught
exception escapes the unit test scope, then the test is considered to
have failed. Otherwise, the test has passed if and only if it returns
a truthy value. In the case of a failed test, the falsy result is
printed out. At the end of running all of the tests, a brief summary
is printed out and a `TestSummary` object is returned.

## The Unit Test Object

    UnitTest := Proc clone.

A `UnitTest` object encapsulates an individual unit test as a callable
procedure. They are usually constructed via `UnitTest make` or
`TestModule addTest ...`.

### Simple Slots

    UnitTest toString := "UnitTest".

### Methods

#### `UnitTest name.`

Returns the unit test's name, which defaults to `'()`. This slot can be
(and usually is) overridden in subobjects.

#### `UnitTest pretty.`

Returns the unit test's name, as though by `self name`.

#### `UnitTest call.`

Runs the unit test. If this method returns truthy, then the unit test
has passed. If this method returns falsy, then the unit test has
failed. In the latter case, the falsy return value will usually
contain some additional information about what has failed. The default
implementation on the `UnitTest` object simply returns the constant
`True`.

### Static Methods

#### `UnitTest make (block).`

Given an evaluating block object, this method constructs a new unit
test whose `call` method performs the given block, with `self` bound
to the appropriate unit test object.

## The Test Summary Object

    TestSummary := Object clone.

This structure object stores information about the results after
running a collection of unit tests.

### Simple Slots

    TestSummary toString := "TestSummary".
    TestSummary passes := 0.
    TestSummary fails := 0.

## The Failed Test Object

    FailedTest := Object clone.

This object represents a unit test which has failed. Any falsy object
returned from a unit test's `call` method will be regarded as a failed
test, but `FailedTest` objects specifically provide useful additional
information about the test which has failed.

### Simple Slots

    FailedTest toString := "FailedTest".
    FailedTest toBool := False.
    FailedTest message := "FailedTest".

### Methods

#### `FailedTest pretty.`

Returns a string representation of the failed test, similar to that of
an exception object, consisting of the name of the failed test
(`toString`), followed by its message (`message`).

### Static Methods

#### `FailedTest make (message).`

Constructs a new `FailedTest` object with the given message.
