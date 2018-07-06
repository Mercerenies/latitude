
# Section II - Standard Library

This section describes the objects and methods available in the
standard library. Note that many of these objects are not required to
have any protection applied to them. However, except where otherwise
stated, the behavior of a program is undefined if a standard library
object is modified and then used later in the program.

Latitude distinguishes between the core library and the standard
modules, both of which fall under the umbrella of "standard
library". The core library is available to every Latitude program that
runs. It consists of essential objects such as the kernel and the root
object. Meanwhile, the standard modules, which are also included in
conforming Latitude mplementations, consist of ancillary objects and
methods which are useful but not essential in all programs.

## Core Library

What follows is the hierarchy of objects available in the core
library. Note that concrete methods and other specific instantiations
of literal objects are not listed here, for brevity. The name listed
here is the usual path from the global scope that can be used to
access the object, so `err SystemCallError` refers to the object at
slot `SystemCallError` within the object at slot `err` in the global
scope object.

    Object
     +-- $argv
     +-- ArgList
     +-- Array
     +-- Boolean
     |    +-- False
     |    +-- True
     +-- Chain
     +-- CollectionBuilder
     +-- Conditional
     +-- Cons
     +-- Dict
     +-- Ellipsis
     +-- Enumeration
     +-- Exception
     |    +-- SystemError
     |    |    +-- err ArgError
     |    |    +-- err BoundsError
     |    |    +-- err ContError
     |    |    +-- err IOError
     |    |    +-- err InputError
     |    |    +-- err IntegrityError
     |    |    |    +-- err UTF8IntegrityError
     |    |    +-- err LangError
     |    |    +-- err ModuleError
     |    |    +-- err NotSupportedError
     |    |    +-- err ParseError
     |    |    +-- err ReadOnlyError
     |    |    |    +-- err ProtectedError
     |    |    +-- err SlotError
     |    |    +-- err SystemArgError
     |    |    +-- err SystemCallError
     |    |    +-- err TypeError
     +-- FileHeader
     +-- FilePath
     +-- Iterator
     |    +-- ArgIterator
     |    +-- ArrayIterator
     |    +-- ChainIterator
     |    +-- DictIterator
     |    +-- NilIterator
     |    +-- RangeIterator
     |    +-- StringIterator
     +-- Kernel
     +-- Kernel GC
     +-- Mixin
     |    +-- Collection
     +-- Module
     +-- ModuleLoader
     +-- Nil
     +-- Number
     +-- Operator
     +-- Parents
     +-- Proc
     |    +-- Cached
     |    +-- Cont
     |    +-- Method
     +-- Process
     +-- Range
     +-- Slots
     +-- StackFrame
     +-- Stream
     +-- String
     +-- Symbol
     +-- err
     +-- global
     +-- meta
     +-- meta operators
     +-- meta sigil

 * [ArgList](arglist.md)
 * [Array](array.md)
 * [Booleans and Nil](boolnil.md)
 * [Cached](cached.md)
 * [Chain](chain.md)
 * [Collection](collection.md)
 * [CollectionBuilder and Builders](collbuilder.md)
 * [Conditional](conditional.md)
 * [Cons](cons.md)
 * [Cont](cont.md)
 * [Dict](dict.md)
 * [Ellipsis](ellipsis.md)
 * [Enumeration](enumeration.md)
 * [Exception and Exceptions](exception.md)
 * [FileHeader](fileheader.md)
 * [FilePath](filepath.md)
 * [Iterator and Iterators](iterator.md)
 * [Kernel](kernel.md)
 * [Method](method.md)
 * [Mixin](mixin.md)
 * [Module](module.md)
 * [ModuleLoader](moduleloader.md)
 * [Number](number.md)
 * [Object](object.md)
 * [Operator](operator.md)
 * [Parents](parents.md)
 * [Proc](proc.md)
 * [Process](process.md)
 * [Range](range.md)
 * [Slots](slots.md)
 * [StackFrame](stackframe.md)
 * [Stream](stream.md)
 * [String](string.md)
 * [Symbol](symbol.md)
 * [global](global.md)
 * [meta](meta.md)

## Standard Modules

In addition to the core library, which is imported by default, there
are several standard modules which can be imported explicitly into
scope. All of the modules listed here fall under the `'(latitude)`
package.

 * [cell](cell.md)
 * [format](format.md)
 * [os](os.md)
 * [random](random.md)
 * [repl](repl.md)
 * [sequence](sequence.md)
 * [time](time.md)
 * [unicode](unicode.md)
 * [unit-test](unit-test.md)

[[up](..)]
<br/>[[prev - Section I - Syntax and Semantics](../i_syntax_and_semantics/)]
<br/>[[next - Appendix](../appendix/)]
