
# The Random Module

    random.lat

The random module provides techniques for generating random numbers.

## Load Effects

When the random module is loaded, a new global dynamic variable
`$random` is defined. It stores the current random number generator.
This global variable can contain any traditional object which defines
a `next` method. The `next` method should return a random nonnegative
integer. The default value of this variable is the `NativeRandom`
singleton object.

To make the random module useful, `$random` should be overriden to be
a useful random number generator. `MersenneTwister` and `NativeRandom`
are generators provided by this module.

Additionally, the module defines a global `$mersenne` slot, which
should have type `MersenneConfig`. New `MersenneTwister` objects will
draw their configuration information from this object. This slot
should usually not need to be modified.

## Module Methods

### `nextInt.`

Uses the current random number generator `$random` to produce a random
number. Equivalent to `$random next`.

## The Native Random Object

This is the default random number generator, which simply delegates to
the operating system's random number function.

### Simple Slots

    NativeRandom toString := "NativeRandom".

### Methods

#### `NativeRandom next.`

Produces a random number from the generator.

## The Mersenne Twister Object

    MersenneTwister := Object clone.

The Mersenne Twister is a simple pseudorandom number generator
appropriate for use in `$random`. New Mersenne Twister objects can be
constructed using `make`.

### Simple Slots

    MersenneTwister toString := "MersenneTwister".

### Methods

#### `MersenneTwister config.`

Returns the object's configuration data, as a `MersenneConfig` object.

#### `MersenneTwister next.`

Produces the next random number from the generator.

### Static Methods

#### `MersenneTwister make (seed).`

Given a nonnegative integer seed, generates a new `MersenneTwister`
object. The object's configuration data is constructed by cloning
`$mersenne`.

## The Mersenne Configuration Object

    MersenneConfig := Object clone.

This object contains configuration details for a `MersenneTwister`
object. In general, programmers should not need to operate directly
with this object.

### Simple Slots

    MersenneConfig toString := "MersenneConfig".
    MersenneConfig w := #<???>.
    MersenneConfig n := #<???>.
    MersenneConfig m := #<???>.
    MersenneConfig r := #<???>.
    MersenneConfig a := #<???>.
    MersenneConfig u := #<???>.
    MersenneConfig d := #<???>.
    MersenneConfig s := #<???>.
    MersenneConfig b := #<???>.
    MersenneConfig t := #<???>.
    MersenneConfig c := #<???>.
    MersenneConfig l := #<???>.
    MersenneConfig f := #<???>.

### Methods

#### `MersenneConfig clone.`

Cloning a `MersenneConfig` object explicitly copies the numerical
parameters of the object.
