![Cassette](https://cassette-lang.com/banner.svg)

Cassette is a small language for solo programming, inpired by Lua, Elixir, and Scheme. It looks like this:

```
def (fizzbuzz n) do
  def (loop m) do
    cond do
      (rem m 3) == 0 and
          (rem m 5) == 0  -> print "FizzBuzz"
      (rem m 3) == 0      -> print "Fizz"
      (rem m 5) == 0      -> print "Buzz"
      true                -> print m
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
Comments and feedback are welcome, especially about this documentation. If anything is unclear or you spot any mistakes, please [drop me a note](mailto:cassette@zjm.me).
</aside>

## [To-Do](#to-do)

Cassette is in early stages of development. Not everything described here has been implemented, and everything is subject to change.

- Module loading
- Garbage collection
- VM optimization
- Standard library
  - Collections
  - Math
  - I/O ports
  - Graphics

## [Building](#building)

This project requires a C build toolchain. You may need to install `make` and `clang`.

The source code can be found [here](https://git.sr.ht/~zjm/Cassette). This project depends on the [univ](https://git.sr.ht/~zjm/univ) library.

- Clone the repo with `git clone https://git.sr.ht/~zjm/Cassette`.
- Run `make deps`. This clones, builds, and copies the univ library into this project. (Alternatively, build that library separately and copy the files into "lib" and "include".)
- Run `make` to build the project. This creates the executable `cassette`, which can be called with a file to execute. `make test` will run the file `test/test.csst`

## [Syntax, Types & Values](#syntax-types-values)

Cassette is dynamically typed. There are seven data types: floating-point numbers, integers, symbols, pairs, tuples, maps, and binaries. Numbers and symbols are immediate values, while the other values are pointers to heap objects.

<aside>
Syntactically, these forms are considered "primary" expressions: Numbers, variables, symbols, strings, lists, tuples, maps, map accesses, and grouped expressions. Primary expressions can be used with infix operators. An "Argument" expression is any primary expression, infix expression, or a `do`, `if`, or `cond` expression. A `call` expression is a sequence of one or more argument expressions, separated by spaces (but not newlines).
</aside>

### [Numbers](#numbers)

Numbers can be written in decimal or hexadecimal. Numbers without a decimal point and hexadecimals are integers, and decimals with a decimal point become floats.

Four infix arithmetic operators are supported: `+`, `-`, `*`, `/`.  Addition, subtraction, and multiplication with two integers results in an integer. Arithmetic with floats results in a float. Division always results in a float.

```
0x1F        ; => 31
3 + 1       ; => 4
3 - 1.0     ; => 2.0
3 * -5      ; => -15
15 / 3      ; => 5.0
```

### [Symbols](#symbols)

Symbols are identifier values that only represent themselves, similar to symbols in Ruby or atoms in Elixir. Two special symbols, `:true` and `:false`, represent boolean values and can be written with the keywords `true` and `false`. 

There are eight infix operators for comparison, equality, and logic: `<`, `<=`, `>=`, `>`, `==`, `!=`, `and`, `or`.

```
:foo == :foo    ; => true
:bar            ; => :bar
:bar == "bar"   ; => false
```

### [Comparison & Equality](#comparison-equality)

These evaluate to the symbols `true` or `false` (usually written with the keywords `true` and `false`). The logical operators are short-circuiting. In logic and conditional expressions, only `false` (the symbol `:false`) and `nil` (the empty list `[]`) are considered false.

Comparison is only defined for numbers, which are compared by value. Equality is tested by identity, so pairs, tuples, maps, and binaries are only equal if they  point to the same heap object. Integers and floats are compared by numeric value.

```
3 and true    ; => true
3 and false   ; => false
3 and nil     ; => false
3 and []      ; => false

:foo == :foo  ; => true
3 == 3.0      ; => true
[3] == [3]    ; => false (two different lists are created)
```

### [Pairs & Lists](#pairs-lists)

Pairs are typical Lisp-style head/tail pairs, which can be linked to form lists. A list can be created with this syntax: `[1, 2, 3]` (commas optional). The keyword `nil` is the empty list, `[]`, and marks the end of a list.

A list can be extended with the update syntax: `[1, 2, 3 | some-list]`. This prepends the given values onto the head of the list. This can be used with any value to create a single pair: `[ :a | :b ]`. This pair syntax is essentially the same as Lisp's `cons` operator.

```
let list = [3, 2, 1]
[5, 4 | list]       ; => [5, 4, 3, 2, 1]
[4 | [3, 2, 1]]     ; => [4, 3, 2, 1]
[:a | :b]           ; => [:a | :b] 
[3 | nil]           ; => [3]
nil == []           ; => true
```

### [Tuples](#tuples)

Tuples are fixed-size arrays. Tuple elements can be accessed by their index in constant time by calling the tuple as a function. A tuple can be created with this syntax: `#[1, 2, 3]` (commas optional).

```
let tup = #[1, 2, 3]
(tup 1)             ; => 2
(tup 100)           ; Out of bounds error
```

### [Maps](#maps)

Maps are key-value dictionaries. A map can be created with this syntax: `{a: 1, b: 2}` (commas optional). This creates a map with the keys `:a` and `:b`, and the values `1` and `2`. Map keys can be any value, but only symbol keys are supported in the literal syntax. For symbol keys, a map can be accessed like this: `foo.x`.

A map can be updated with a syntax similar to lists: `{c: 3, d: 4 | some-map}`. This returns a new map that is a copy of the original with the updated entries.

```
let d = {a: 1, b: 2}
d.a                   ; => 1
d.foo                 ; => nil
{c: 3 | d}            ; => {a: 1, b: 2, c: 3}
{a: 0, d: d.a | d}    ; => {a: 0, b: 2, d: 1}
```

### [Binaries & Strings](#binaries-strings)

Strings in Cassette are represented only as "binaries", a sequence of arbitrary bytes. Binaries can be created from UTF-8 strings with double quotes:

```
"Hi!"  ; bytes 0x48, 0x69, 0x21
"水"    ; bytes 0xE6, 0xB0, 0xB4
```

### [Defining Values](#defining-values)

The `let` syntax allows you to define variables in the current block. `let` is only allowed within blocks.

```
let x = 1, 
    y = 2
print x             ; => prints "1"
print z             ; Undefined variable error
```

### [Functions & Lambdas](#functions-lambdas)

A function object is represented as a list beginning with the symbol `λ`, followed by data defining the function. Functions can be created with the lambda syntax: `(x y) -> x * y`. Variadic functions can be created by specifying a variable for the parameter list, like this: `params -> (print params)`. A variadic function receives its arguments as a list bound to that variable.

```
(a b c) -> a * b + c
params -> length params
```

Functions can be defined with the `let` or `def` syntax. `def` is syntactic sugar for `let`:

```
; These are equivalent:

let foo = (x y) -> do
  x * y
end

def (foo x y) do
  x * y
end
```

Functions are called by name by listing their arguments afterwards, without separators: `foo 1 2 3 4`. Precedence can be enforced with parentheses: `foo (bar x) y`. Infix expressions are considered a single argument. To call a function with no arguments, you must use parentheses: `(do-stuff)`.

```
def (combine x y z) do
  x * y + x * z
end

def (inc x)
  x + 1
end

def (val) do
  4
end

let x = 8
combine (5 + 1) * 2 (inc x) (val)   ; `combine` called with arguments 12, 9, 4
combine 2 (inc x) val               ; `combine` called with arguments 2, 9, [λ val]
combine                             ; not called, just the value [λ combine]
```

### [Foreign Functions](#foreign-functions)

A foreign-function interface allows Cassette code to call native code. An identifier beginning with `@` represents a foreign function, and can be called with arguments like a regular function.

The set of available foreign functions must be defined by the runtime before execution. A foreign function is registered in the C API with a name and a pointer to an implementation function. The C API is not yet fully defined.

```
def (print x) do
  @print x                  ; calls foreign function "print" with arguments [x]
end

print "Hello, world!"       ; prints "Hello, world!"
```

### [Blocks & Conditionals](#blocks-conditionals)

The top level of a file is a block. A block is a sequence of call expressions, separated by newlines. Function calls in a block can't have newlines between arguments, unless called within parentheses. Newlines are also allowed after an infix operator and within list, tuple, and map literals.

A block can be created with a `do` expression, which can be used anywhere an expression is allowed. A `do` expression evaluates to the last statement in the block.

```
let f = (x y) -> do
  print "Called f!"
  x * y
end

(f 2 3)             ; prints "Called f!", and evaluates to 6
```

The `cond` and `if` expressions allow conditional flow. `if` expressions have a predicate argument followed by a `do` expression and an optional `else` expression. An `if` without an `else` returns `nil` when the predicate is false.

```
def (second list) do
  if (length list) > 2 do
    head (tail list)
  end
end

second [3]             ; => nil
second [3 5 7 9]       ; => 5

if (try-something 0 10) do
  :ok
else
  :error
end
```

A `cond` expression allows multiple predicate clauses. The predicates are evaluated in order until one is true, then that predicate's consequent is evaluated. `true` can be used as a fallback predicate.

Clauses are separated by newlines. Any argument expression can be a clause predicate except lambdas. Any call expression can be a consequent.

```
cond do
  x == y      -> foo x y
  (test x y)  -> :ok
  true        -> :error
end
```

## [Modules & Import](#modules-import)

The `import` keyword allows inclusion of another module. When used without `as` (e.g. `import "foo.csst"`), any definitions in the top-level of the other module become defined in the current environment. With the `as` keyword, the imported definitions are keys in a map bound to the identifier.

```
import "helpers.csst"       ; all defs in "helpers.csst" are now in scope
import "math.csst" as Math  ; defs in "math.csst" are keys in the map `Math`
```

## [Input & Output](#input-output)

Cassette uses the "port" concept for I/O. The `open` function opens a port of a specified type. Then the `send` function can send a message to that port. The structure of the message depends on the type of port. The `receive` can be called to take an incoming message from a port. If no message is waiting, `receive` returns `nil`.

Planned port types are `file`, for normal file I/O, and `window`, for graphics and input events.

## [Memory Representation](#memory-representation)

The base of the runtime is the memory representation. Values are 32-bit NaN-boxed floats: any floating point number represents itself, except when it's NaN, the unused 23 bits encode a type and value.

```
         31      24       16       8       0
         ┌┴┬──────┴─┬──────┴───────┴───────┴┐
   Float │S│ <expnt>│       <mantissa>      │
     NaN │-│11111111│1----------------------│
         ├─┼────────┼───────────────────────┤
 Integer │0│11111111│100 <signed int value> │
  Symbol │0│11111111│101   <symbol hash>    │
    Pair │0│11111111│110    <heap index>    │
  Object │0│11111111│111    <heap index>    │
Reserved │1│11111111│100--------------------│
Reserved │1│11111111│101--------------------│
Reserved │1│11111111│110--------------------│
Reserved │1│11111111│111--------------------│
         └─┴────────┴───────────────────────┘
```

When the exponent is all 1s and any mantissa bit (usually the high bit) is 1, the value represents NaN, and the remaining 23 bits are undefined. Cassette uses the sign bit and bits 20 and 21 to encode a type, and bits 0–19 to encode a raw value. Integers and symbols are represented immediately, but pairs and other objects store an index into the heap where the object data is.

Symbol values are a 20-bit hash of the symbol name, which is stored in a hash table.

### [Heap-Allocated Objects](#heap-allocated-objects)

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
