
# Chapter 2 - Lexical Structure

## Encoding

Latitude programs are assumed to be encoded in UTF-8.

## Whitespace

The following characters are considered to be whitespace: space
(`U+0020`), horizontal tab (`U+0009`), carriage return (`U+000D`), and
line feed (`U+000A`). In general, whitespace is used to separate
tokens, so unless otherwise stated, multiple whitespace characters
will be treated as equivalent to a single character of whitespace.

[TODO: Should some non-ASCII characters be whitespace?]


## Character Sets

Aside from whitespace characters, there are three other classes of
characters in the Latitude parser: special, semi-special, and normal.

 * Whitespace characters, as discussed in the section on whitespace,
   are generally ignored and are used to separate tokens.
 * Special characters are treated specially when they appear in the
   program, except in specific circumstances such as the inside of a
   string. They are used as parts of syntactic constructs and can
   never appear in an identifier. These are the dot (`U+002E`), comma
   (`U+002C`), colon (`U+003A`), semicolon (`U+003B`), parentheses
   (`U+0028` and `U+0029`), brackets (`U+005B` and `U+005D`), braces
   (`U+007B` and `U+007D`), single quotes (`U+0027`), and double
   quotes (`U+0022`).
 * Semi-special characters are treated specially when they appear
   following whitespace. As such, they cannot be the first character
   in an identifier, but they can appear in the middle or at the end
   of an identifier. These are the digits (`U+0030` to `U+0039`),
   tilde (`U+007E`), hash (`U+0023`), and at-sign (`U+0040`).
 * Normal characters can appear anywhere in an identifier. Any
   character that is not in any of the above classes is considered
   normal.

## Identifiers

An identifier is a nonempty string of characters, of which the first
must be a normal character and the remaining characters must be either
normal or semi-special.

Note that an identifier ending with an equal-sign (`=`) is often
called an assignment identifier, as there is a special syntax which
invokes methods with these names in a convenient way.

The following exceptions take precedence over the above rule.
 * Three consecutive dots (`...`) form a valid identifier, called the
   ellipsis identifier (and its corresponding variable is called the
   ellipsis variable). As a consequence of this, a single dot is
   considered a statement terminator, a pair of consecutive dots is
   almost always a syntax error, and three dots in a row is parsed as
   the ellipsis identifier.
 * Two colons in a row (`::`) form a valid identifier. Note that there
   is no assignment identifier corresponding to this, as `::=` is a
   special syntactic token and not a valid identifier.
 * A single isolated equal-sign (`=`) and a less-than sign followed by
   a dash (`<-`) are not valid identifiers, as both are treated
   specially by the language to allow custom assignment
   operations. However, identifiers *containing* equal-signs or a
   less-than sign followed by a dash are allowed. In particular,
   identifiers consisting only of two or more equal-signs (such as
   `==` or `===`) are allowed.
 * A plus sign (`+`) or minus sign (`-`) followed by at least one
   digit (`0` to `9`) is not a valid *start* to an identifier. That
   is, any string which begins with a plus sign or minus sign followed
   by at least one digit, followed by any other characters is not a
   valid identifier, as it will be parsed as a numerical literal.

## Code Structure

A source file consists of zero or more lines, as defined by the
following grammar.

```
<line> ::= <stmt> "."
<stmt> ::= <chain> <name> <rhs> |
           <literalish>
<rhs> ::= λ |
          <shortarglist> |
          ":=" <stmt> |
          "::=" <stmt> |
          ":" <arglist> |
          <shortarglist> "=" <stmt> |
          "<-" <stmt>
<arglist> ::= λ |
              <arg> <arglist1>
<arglist1> ::= λ |
               "," <arg> <arglist1>
<arg> ::= <nonemptychain>
<nonemptychain> ::= <chain> <name> |
                    <chain> <name> <shortarglist> |
                    <literalish>
<chain> ::= <nonemptychain> |
            λ
<shortarglist> ::= "(" <arglist> ")" |
                   "(" <chain> <name> ":" <arg> ")" |
                   <literal>
<literalish> ::= "~" <text> <literalish> |
                 "(" <stmt> ")" |
                 <literal>
<literal> ::= "{" <linelist> "}" |
              <number> |
              "\"" <text> "\"" |
              "'" <text> |
              "'(" <text> ")" |
              "~" <text> |
              "[" <arglist> "]" |
              "'[" <literallist> "]" |
              "#\"" <text> "\"#" |
              "#(" <text> ")" |
              "0" <letter> { <alphanum> } |
              "@{" <linelist> "}" |
              "@(" <number> "," <number> ")" |
              "#'" <text>
<linelist> ::= <line> <linelist> |
               λ
<literallist> ::= <listlist> <literallist1> |
                  λ
<literallist1> ::= "," <listlist> <literallist1> |
                   λ
<listlit> ::= "\"" <text> "\"" |
              "'" <text> |
              "~" <text> |
              <number> |
              "'(" <text> ")" |
              "[" <literallist> "]" |
              <name>
```

[TODO: What is `<text>`? And `<number>`? `<letter>`? `<name>`?]
