
# Booleans and the Nil Object

    Boolean := Object clone.
    True := Boolean clone.
    False := Boolean clone.
    Nil := Object clone.

The Boolean object models values which represent truth-hood or
falsehood. Its two subobjects, the true object and the false object,
represent these two concepts, respectively, of truth-hood and
falsehood. They are primarily used in Boolean statements such as
if-statements [TODO: Link] and loop conditionals [TODO: Link].

The nil object models the concept of emptiness.

## Simple Slots

    Boolean toString := "Boolean".

    False toString := "False".
    False toBool := False.
    False false? := True.

    True toString := "True".
    True true? := True.

    Nil toString := "Nil".
    Nil toBool := False.
    Nil nil? := True.
