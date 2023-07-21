![Cassette](https://cassette-lang.com/banner.svg)

Cassette is a small language for solo programming, inpired by Elixir and Scheme. It looks like this:

```
def (fizzbuzz n) do
  def (loop m) do
    cond do
      m % 3 == 0 and m % 5 == 0   -> print "FizzBuzz"
      m % 3 == 0                  -> print "Fizz"
      m % 5 == 0                  -> print "Buzz"
      true                        -> print m
    end

    if m < n do
      loop m + 1
    end
  end

  loop 1
end

fizzbuzz 100
```

<aside>
Feedback is welcome — if anything is unclear or you spot any mistakes, please [drop me a note](mailto:cassette@zjm.me).
</aside>

## [To-Do](#to-do)

Cassette is in early stages of development. Not everything described here has been implemented, and everything is subject to change.

- HAMT-based large maps
- Lexical addressing
- Source context on runtime errors
- Standard library
- Profile VM
- Documentation

## [Building](#building)

This project requires a C build toolchain. You may need to install `make` and `clang`.

The source code can be found [here](https://git.sr.ht/~zjm/Cassette). This project depends on the [univ](https://git.sr.ht/~zjm/univ) library.

- Clone the repo with `git clone https://git.sr.ht/~zjm/Cassette`.
- Run `make deps`. This clones, builds, and copies the univ library into this project. (Alternatively, build that library separately and copy the files into "lib" and "include".)
- Run `make` to build the project. This creates the executable `cassette`, which can be called with a file to execute, or launch a REPL. `make test` will run the file `test/test.csst`.

## [Syntax](#syntax)

### [Values](#values)

- Numbers: `3.14`, `-5`
- Symbols: `:error`, `:file-not-found`
  - Symbols and variables can use any characters except spaces and these reserved characters: `# * / , . | : ( ) [ ] { }`
  - The symbols `:true` and `:false` represent boolean values, and can be written as `true` and `false`
- Binaries: `"Hello world!"`, `"He said \"no\""`
  - Strings are stored as UTF-8 encoded binaries, but binaries are generally encoding-agnostic
- Lists: `[1, 2, 3]`
  - The empty list can be written as `nil`
- Tuples: `{1, 2, 3}`
- Maps: `{x: 24.1, y: 36.8}`
  - Only symbol keys are supported with this syntax, but maps can have any value keys using functions like `Map.put`

### [Argument Expressions](#argument-expressions)

An argument expression is a value or an argument expression combined with an operator. Parentheses allow grouping expressions and function calls. These are the supported operators, in order of precedence (low to high):

- Lambda: `(x) -> x`
- Logic: `x and y`, `x or y`
  - The values `nil` and `false` are falsey. All other values are truthy.
- Equality: `x == y`, `x != y`
- Comparison: `x > y`, `x < y`, `x >= y`, `x <= y`
- Membership: `x in y`
  - Only collections on the right
- Pair creation: `x | y`
  - Right-associative
- Add/subtract: `x + y`, `x - y`
  - Only numbers
- Multiply/divide/remainder: `x * y`, `x / y`, `x % y`
  - Only numbers. For remainder, only integers.
- Exponent: `x ^ y`
  - Only numbers
- Logical not: `not x`
- Negation: `-x`
  - Only numbers
- Length: `#x`
  - Only collections
- Map access: `x.y`
  - Only maps on the left. Only an identifier on the right, which is treated as a symbol key.

`do`, `if`, and `cond` expressions also count as argument expressions.

### [Call Expressions](#call-expressions)

A list of argument expressions is treated as a function call: `foo 3 4 2*x`. If there is only one argument expression (e.g. a function with no parameters), the function is not called, unless it's in parentheses.

```
send-alert                          ; not called
(send-alert)                        ; called
send-alert-to "admin@example.com"   ; called
(send-alert-to "admin@exaple.com")  ; called
```

When a call is in parentheses and has only one expression, it is called if it's a function. Otherwise, it's simply evaluated.

```
  x * (3 + y) - (foo) / (bar x y)   ; `(3 + y)` is not called as a function. `(foo)` is called, if `foo` is a function. `bar` is expected to be a function.
```

Collections (lists, tuples, binaries, and maps) can be called as functions with one parameter. This will access the collection by integer index (for lists, tuples, and binaries) or by key (for maps).

```
  let my-list = [1, 2, 3],
      my-tup = {4, 5, 6},
      my-map = {a: 7, b: 8}

  (my-list 2)     ; => 3
  (my-tup 1)      ; => 5
  (my-map :b)     ; => 8
  my-map.b        ; => 8

  let my-map = (Map.put my-map "c" 9)

  (my-map "c")    ; => 9
```

### [Blocks](#blocks)

A block is a sequence of multiple call expressions, separated by newlines or commas. The top level of a Cassette script is a block.

A `do` expression allows a block as an argument expression. It evaluates to its last call expression.

```
do
  print "ok"
  (send-alert)    ; needs parentheses to be called
  send-alert      ; not called. `send-alert` (presumably a function) is the value of the `do` expression.
end
```

An `if` expression allows simple conditionals. The `else` block is optional, and defaults to `nil`.

```
if b != 0 do
  a / b
else
  {:error, :div-by-zero}
end

if not (check-system) do
  (send-alert)
end
```

A `cond` expression allows multiple conditional clauses. The predicate must be an argument expression (except for lambdas). If no predicate is true, the result is `nil`.

```
cond do
  x % 3 == 0  -> "Fizz"
  x % 5 == 0  -> "Buzz"
  true        -> x
end
```

### [Definitions](#definitions)

Variables may be defined in a block with a `let` statement.

```
let a = 1
let b = 2 * a
let c = (foo b),
    d = 4, e = 5
```

Functions can be defined with `let` and a lambda, or using `def`.

```
let foo = (x) -> do
  x * 2
end

def (foo x) do
  x * 2
end
```

### [Modules](#modules)

A Cassette script is a module if it contains a module statement. A module statement can appear anywhere in the script, but it's recommended to put it at the beginning. Only one module statement is allowed in a script. Module names must be dot-separated identifiers.

```
module String.UTF8

; ...
```

A script can import a module with `import`. All top-level definitions in the module are imported. The imported definitions are imported as the module name by default, but a different name can be specified.

```
import String.UTF8
import Some.Long.Nested.Name.Helpers as Helpers

String.UTF8.valid? "some string"
Helpers.validate "some string"
```

A module's definitions can also be imported directly into a script with this special syntax:

```
import Math.Matrix as *

let m = {{1 2 3} {4 5 6} {7 8 9}}

inverse m     ; presumably imported from Math.Matrix
```

<aside>
When a Cassette script is executed, if it imports any modules, then the directory tree containing the script is scanned for potential modules. Then each import is recursively included into the AST before being compiled.
</aside>

## [Internals](#internals)

### [Values & Types](#values-types)

Cassette is dynamically typed. The basic data types in Cassette are:

- Numbers
- Symbols
- Pairs (used to form lists)
- Tuples
- Binaries (used to represent strings and binary data)
- Maps

Numbers are either 32-bit signed integers or 32-bit IEEE 754 floating point numbers. Constants written with a decimal, like `31.0`, are floats; without a decimal, like `31`, are integers. Integers can also be written in hexadecimal, like `0x5A4D`. Numbers are converted between floating point and integer based on the operations applied to them.

A symbol is a unique value that simply represent itself. A symbol is written like this: `:ok`, `:error`, `:symbols_can_be_really_long_but_maybe_shouldn't_be`. Two special symbols, `:true` and `:false`, represent boolean values, and can be written simply as `true` and `false`.

A pair is a value that contains two values, a head and a tail. A pair can contain any two values. Pairs are created with the pipe operator: `1 | 2`. A special value, `nil`, is the empty list, used as the tail of the last pair in the list. `nil` is also a pair, with `nil` as its head and `nil` as its tail.

A tuple is a fixed-size set of items, which can be accessed in constant time: `{1 2 3}`.

A binary is a sequence of bytes. Binaries are used to represent strings and other binary data. Cassette is encoding-agnostic, making no assumptions about the contents of a binary. Binaries can be created with the string syntax, like `"hello, world!"`, or by converting a list of integers. Binaries, unlike pairs, tuples, and maps, are not persistent: the contents of a binary can be changed in-place.

A map is a key-value store, known elsewhere as an _associative array_ or a _dictionary_. Maps can be written like this: `{foo: 1, bar: "ok"}`. In this case, the keys are the symbols `:foo` and `:bar`. When creating maps like this, only symbols can be used as keys, but any value can be used as a key when using map functions like `Map.put`. Maps, like tuples and pairs, are immutable.

Pairs, tuples, binaries, and maps are _object types_: their data is stored in memory on the heap, and the values are references to that data.

### [Memory Representation](#memory-representation)

Values are represented as 32-bit NaN-boxed floats. Any floating point number represents itself, except when it's NaN, the unused 23 bits encode a type and value.

```
         31      24       16       8       0
         ┌┴┬──────┴─┬──────┴───────┴───────┴┐
   Float │S│ <expnt>│       <mantissa>      │
     NaN │-│11111111│1----------------------│ 0x7FC00000
         ├─┼────────┼───────────────────────┤
 Integer │0│11111111│100 <signed int value> │ 0x7FC00000
  Symbol │0│11111111│101   <symbol hash>    │ 0x7FD00000
    Pair │0│11111111│110    <heap index>    │ 0x7FE00000
  Object │0│11111111│111    <heap index>    │ 0x7FF00000
   Tuple │1│11111111│100    <num items>     │
  Binary │1│11111111│101    <num bytes>     │
Reserved │1│11111111│110--------------------│
Reserved │1│11111111│111--------------------│
         └─┴────────┴───────────────────────┘
```

When the exponent is all 1s and any mantissa bit (usually the high bit) is 1, the value represents NaN, and the remaining 23 bits are undefined. Cassette uses the sign bit and bits 20 and 21 to encode a type, and bits 0–19 to encode a raw value. Integers and symbols are represented immediately, but pairs and other objects store an index into the heap where the object data is.

Symbol values are a 20-bit hash of the symbol name, which is stored in a separate symbol table.

Pairs are stored in the heap with the head at the value index and the tail just after. Other objects stored in the heap begin with a header value that specifies the object type and other info. The top three bits of a header encode the object type, and the rest encode some data about the object, usually its length. The object's data follows the header.

```
         31     24      16       8       0
         ┌┴──────┴───────┴───────┴───────┴┐
   Tuple │000      <number of items>      │
  Binary │001      <number of bytes>      │
Reserved │010-----------------------------│
Reserved │011-----------------------------│
Reserved │100-----------------------------│
Reserved │101-----------------------------│
Reserved │110-----------------------------│
Reserved │111-----------------------------│
         └────────────────────────────────┘
```

Tuples store their values sequentially after the header. Binaries store their data just after the header, padded to four bytes. The number of 4-byte words a binary uses (excluding the header) can be calculated with `((length - 1) / 4) + 1`. Strings are represented by binaries. String literals in programs are stored in the symbol table, and created on the heap at runtime.
