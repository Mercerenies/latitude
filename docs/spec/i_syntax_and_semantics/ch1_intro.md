
# Chapter 1 - Introduction

## Motivation

Latitude is a dynamically-typed prototype-oriented programming
language. Latitude aims to promote metaprogramming and to allow its
users the maximum control possible over the behavior of their
programs. As such, it defines a simplistic but extremely extensible
object model centered around prototyping by delegation, meaning newly
created objects maintain a reference to their parent which allows them
to retain all the behavior of the parent object until told to do
otherwise.

## Program Structure

A Latitude program consists of zero or more statements, each
terminated by a dot `.`. These statements typically define methods,
construct objects, and load other files to create a complete
environment for a running program.

When a new Latitude process is started, the process constructs the
necessary objects to form the Latitude core library. Control is then
passed to either an interactive interpreter or to a user-specified
execution file.

## Terms

A Latitude program is a file that is currently being parsed, compiled,
or evaluated. A virtual machine, or VM, is a running Latitude process.

[[up](.)]
