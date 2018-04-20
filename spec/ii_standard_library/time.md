
# The Time Module

    time.lat

The time module provides methods for accessing the current time and
date, as well as for manipulating date-time objects.

## Module Methods

### `now.`

Returns the current time and date, in the system's timezone, as a
`DateTime` object.

### `utc.`

Returns the current time and date, in coordinated universal time
(UTC), as a `DateTime` object.

## The Date-Time Object

    DateTime := Object clone.

This object stores a date and time, according to the modern Gregorian
calendar.

### Methods

#### `DateTime second.`

Returns the second, from `0` to `59`, represented by this object.

#### `DateTime minute.`

Returns the minute, from `0` to `59`, represented by this object.

#### `DateTime hour.`

Returns the hour, from `0` to `23`, represented by this object.

#### `DateTime day.`

Returns the day of the month represented by this object.

#### `DateTime year.`

Returns the year represented by this object.

#### `DateTime yearDay.`

Returns the day of the year represented by this object.

#### `DateTime month.`

Returns the month, as an instance of `Month`, represented by this
object.

#### `DateTime weekday.`

Returns the day of the week, as an instance of `Weekday`, represented
by this object.

#### `DateTime dst?.`

Returns whether the current time and date is subject to active
daylight savings time.

## The Month Object

    Month := Enumeration clone.

`Month` is an [enumeration object](enum.md) with the following
values: `January`, `February`, `March`, `April`, `May`, `June`,
`July`, `August`, `September`, `October`, `November`, `December`. It
also defines a `toString` which evaluates to `"Month"`.

## The Weekday Object

    Weekday := Enumeration clone.

`Weekday` is an [enumeration object](enum.md) with the following
values: `Sunday`, `Monday`, `Tuesday`, `Wednesday`, `Thursday`,
`Friday`, `Saturday`. It also defines a `toString` which evaluates to
`"Weekday"`. Note that the week begins on Sunday, not Monday.
