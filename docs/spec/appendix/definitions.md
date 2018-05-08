
# Definitions

 * Evaluating object (defined in [Objects](../i_syntax_and_semantics/ch3_object.md#objects))) -
   An object whose primitive field contains a method is an evaluating
   object.
 * Non-traditional object (defined
   in [Objects](../i_syntax_and_semantics/ch3_object.md#objects)) -
   Any object which is not a *traditional object* is a non-traditional
   object.
 * Traditional object (defined
   in [Objects](../i_syntax_and_semantics/ch3_object.md#objects)) - A
   traditional object has the
   root [`Object`](../ii_standard_library/object.md) object as the root of its object hierarchy.
 * Argument-like object (defined
   in [`ArgList`](../ii_standard_library/arglist.md#arglist-fillwith)) -
   An argument-like object is an object which has slots of the form
   `$1`, `$2` ....
 * Dictionary object - An object, usually a non-traditional object,
   whose primary purpose is storing arbitrary key-value data, as a
   dictionary data structure would.
 * Structure object - An object whose primary purpose is storing
   structured data. Structure objects usually have very few, if any,
   methods defined on them, and many simple slots which hold values.
