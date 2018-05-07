
# The Ellipsis Object

    Ellipsis := Object clone.

The singleton `Ellipsis` object, usually accessed by the `...` name,
will always return truthy when `=~` is called on it, making it a
"catch-all" pattern.

## Simple Slots

    Ellipsis toString := "Ellipsis".
    Ellipsis =~ := True.