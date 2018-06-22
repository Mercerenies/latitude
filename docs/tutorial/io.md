
# Input and Output

So far, we have seen two global methods for printing to the screen:
`putln` for strings and `println` for objects in general. These global
methods, as well as they're companions `puts` and `print` (which do
not end in a newline), all delegate to the standard output stream
object `$stdout`. Now we're going to discuss how stream objects work
and which ones come built in to Latitude.

## Stream Objects

Latitude comes with three built-in global stream objects: `$stdout`,
`$stdin`, and `$stderr`. All three of these names are dynamically
scoped to make it easy for programmers to locally override them.

    do {
      ;; Run someOperation, but block any textual output.
      $stdout := Stream null.
      $stderr := Stream null.
      someOperation.
    }.

Since the output streams are dynamic variables, `someOperation` will
see the new values for them: a null stream which ignores all text
written to it, like `/dev/null`.

Stream objects provide several ways of outputting information. We have
already discussed the four main output functions, but to summarize

| Name      | Accepts      | Newline? |
|-----------|--------------| -------- |
| `puts`    | Strings only | No       |
| `putln`   | Strings only | Yes      |
| `print`   | Any objects  | No       |
| `println` | Any objects  | Yes      |

Unlike output functions, input functions always operate on
strings.<sup><a name="footnote-01a"
href="#user-content-footnote-01f">1</a></sup>

| Name      | Returns | Reads         |
|-----------|---------| ------------- |
| `read`    | String  | One character |
| `readln`  | String  | Full line     |

To check if a stream is designated for input or output, send it the
`in?` or `out?` message. Streams can be but are not necessarily
designated for both; for instance, the null stream from the example
above is designated for both input and output. Of course, `$stdin` is
only for input, and `$stdout` and `$stderr` are only for output.

To check whether a stream has hit its end, send it the `eof?` message.
Streams are closed with the `close` message, although as we have seen
in the previous chapter, it is more common to use `closeAfter` to
protect the stream in the case of an error.

## Files

Constructing a file stream is done with the `Stream open` method.

    file := Stream open ("myfile.txt", "r") closeAfter {
      ;; Do some work ...
    }.

The first argument to `Stream open` is, of course, the file name. The
second argument specifies the mode. The mode should be either `r` or
`w`, respectively for read and write. You may append a `b` to the mode
to specify that it is a binary file; this may cause the operating
system to treat new lines differently. So the four valid mode strings
are: `r`, `w`, `rb`, `wb`.

## Formatted Printing

Output stream objects also provide a `printf` method, for formatted
printing. The stream `printf` method takes a formatter as its first
argument, followed by any arguments to be forwarded to the formatter.
The formatter can be any procedure-like object: that is, any object
which responds to the `call` message.<sup><a name="footnote-02a"
href="#user-content-footnote-02f">2</a></sup> However, most likely we
will be using the `FormatString` object from the `'format` module,
which was built for this purpose.

    use 'format importAllSigils.

    $stdout puts: "Name: ".
    yourName := $stdin readln.
    $stdout puts: "Age: ".
    yourAge := { $stdin readln toInt. } catch (err InputError) do { "<unknown>". }.
    $stdout printf: ~fmt "Hello, ~A! (age ~A)", yourName, yourAge.

The `~fmt` bit begins a sigil expression, which in this case is simply
a concise shortcut to constructing a `FormatString` object. The format
string object supports the following directives.

 * `~S` consumes one argument and calls `toString` on it.
 * `~A` consumes one argument and calls `pretty` on it.
 * `~~` prints a literal tilde.

In this case, we use the second variant. `pretty` delegates to
`toString` in most cases; for strings, `pretty` will return the string
itself, whereas `toString` will place quotes around it. In general,
`toString` should produce relatively machine-readable output, whereas
`pretty` is free to make its output more user-friendly.

Remember that `printf` simply sends the `call` message to the
formatting object. So if you wish to format a string but not print it,
you may send `call` directly to the object to get a formatted string
as the return value.

    use 'format importAllSigils.
    message := ~fmt "Hello, ~A!" call ("John Doe"). ; ==> Hello, John Doe!

## Summary

You have now had an introduction to most of the `Stream` object
functionality, as well as construction of formatted strings. In the
next chapter, we'll formally discuss modules and Latitude's import
system.

[[up](.)]
<br/>[[prev - Advanced Flow Control](cont.md)]

<hr/>

<a name="footnote-01f"
href="#user-content-footnote-01a"><sup>1</sup></a> Note that `read`
may behave strangely at the REPL. This is because the REPL shares the
same input stream and uses `readln` internally, so after you `read`
one character, the REPL will immediately consume the rest of the line
and try to run it as a command.

<a name="footnote-02f"
href="#user-content-footnote-02a"><sup>2</sup></a> By convention, the
parent of all procedure-like objects is the global `Proc` object. You
can construct these objects explicitly from a method with `proc { ...
}`. Additionally, methods themselves are (indirect) subobjects of
`Proc`. Likewise, continuation objects are also subobjects of `Proc`.
