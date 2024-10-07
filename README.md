![Cassette](https://cassette-lang.com/banner.png)

[Cassette](https://cassette-lang.com) is a small functional programming language. It looks like this:

```
import IO, Net, String (crlf), List

; keeps reading from a connection while there's any data
def read_resp(conn) do
  def loop(received)
    let
      chunk = IO.read_chunk(conn, 1024)   ; get the next chunk
    in
      if #chunk == 0, received    ; no more data
      else loop(received <> chunk)

  loop("")
end

let
  conn = Net.connect("cassette-lang.com", "80")   ; open a network connection
  req = List.join([                               ; form an HTTP request
    "GET / HTTP/1.0",
    "Host: cassette-lang.com",
    "",
    ""
  ], crlf)
in
  do
    IO.write(conn, req)         ; send the request
    IO.print(read_resp(conn))   ; read the response and print it
  end
```

I made Cassette as a simple language for personal programming. It's designed for solo programmers working on non-enterprise projects. It's DIY, roll your own, batteries-not-included. It's for fun.

Here are some features of Cassette:

- Functional
- Immutable values
- Garbage collected
- Efficient tail-call recursion
- Module based

## Getting Started
This project requires a C toolchain and SDL2.

1. Get the project's dependencies
    - On macOS with Homebrew, run `brew install llvm git sdl2`
    - On Debian, run `apt install build-essential clang git libsdl2-dev`
2. Build Cassette
    - Clone Cassette with `git clone https://github.com/protestContest/Cassette` (and then `cd Cassette`)
    - Run `make` to build the project. This creates the executable `bin/cassette`.
3. Try the example with `./bin/cassette test/test.ct`.

## Syntax

### Values

Cassette has only four value types: integers, pairs, tuples, and binaries.

Integers are signed, 30-bit numbers (-536,870,912 to 536,870,911). Integers can be written in decimal, hexadecimal, or a literal byte value. The keyword `true` is shorthand for `1` and the keyword `false` is shorthand for `0`.

```
1                 ; decimal integer
0x1F              ; hex integer
$a                ; => 0x61
true              ; => 1
false             ; => 0
```

Symbols are arbitrary values. (Some languages call them atoms.) At runtime, these become the integer hash value of the symbol name.

```
:hello
:ok
:not_found_error
```

Pairs are Lisp-style cons cells, which are used to create linked lists. The keyword `nil` is shorthand for a special pair, which is the empty list. The pair operator, `:`, is right-associative.

```
100 : 200         ; pair
nil               ; empty list
[1, 2, 3]         ; list, same as 1 : 2 : 3 : nil
```

Tuples are fixed-size arrays. They're less flexible than lists, but they use less memory and are more efficient to access. The maximum size of a tuple is the maximum integer size.

```
{1, 2, 3}
```

Binaries are byte vectors. Strings are represented as UTF-8 encoded binaries. The maximum size of a binary is the maximum integer size.

```
"Hello!"
```

### Operators

Cassette supports several built-in operators on values. Most operators only work on certain types.

Basic arithmetic and comparison operators work on integers.

```
-24               ; unary negation
73 + 4            ; addition
87 - 41           ; subtraction
43 * 12           ; multiplication
17 / 4            ; division (truncating)
400 % 12          ; modulus
256 >> 3          ; bit shift
1 << 27
0xAA | 1          ; bitwise or
1036 & 0xFF       ; bitwise and
~7                ; bitwise not
12 < 3            ; comparison
12 <= 3
12 > 3
12 >= 3
```

Some operators only work with pairs, tuples, or binaries.

```
; get the head of a pair
@(1 : 2)          ; => 1
@[1, 2, 3]        ; => 1

; get the tail of a pair
^(1 : 2)          ; => 2
^[1, 2, 3]        ; => [2, 3]

; join two tuples or binaries
{1, 2} <> {3, 4}  ; => {1, 2, 3, 4}
"ab" <> "cd"      ; => "abcd"

; get the length of a tuple or binary
#{:foo, :bar}     ; => 2
#"hello"          ; => 5

; get an element of a tuple or binary
{1, 2, 3}[0]      ; => 1
"test"[2]         ; => $s (an integer)

; slice a tuple or binary
{1, 2, 3, 4}[1,3] ; => {2, 3}
"hello"[1,4]      ; => "ell"
```

Logic and equality operators work on any type. Only the values `0` (a.k.a. `false`) and `nil` evaluate as false. Logic operators short-circuit and evaluate to one of their operands. Equality is compared structurally, and returns `true` or `false`.

```
false or :ok      ; => :ok
true and nil      ; => nil
not nil           ; => true
not {0, 0}        ; => false
3 == 3            ; => true
[1, 2] == [1, 2]  ; => true
```

### Conditionals

An `if` expression is a list of predicate/consequent pairs. It tests each predicate until one is true, then evaluates that predicate's consequent. If none are true, the `else` expression is evaluated.

```
if true, :ok else :error

if
  x >= 10,  :ten_plus
  x >= 1,   :one_plus
  else      :less_than_one
```

### Variables

A `let` expression is a list of assignments and a result expression. Each assigned variable is in scope in the subsequent assignments (but not in its own) and in the result expression.

```
let
  x = 3
  y = 2 + 1
in
  x - y

let
  result = try_this()
  result = try_that(result)   ; the argument refers to the previous assignment
in
  done(result)
```

An assignment in a `let` expression can have an `except` clause. If the `except` test evaluates true, the clause's alternative expression becomes the result of the `let` expression, and the rest of the `let` expression is ignored.

```
let
  conn = Net.connect(host, "80") except error?(result), result
  result = IO.write(conn, req) except error?(result), result
in
  IO.read(conn)
```

### Functions

Functions can be defined as lambdas. Function calls look similar to other languages.

```
let foo = \a, b -> a + b
in foo(1, 2)
```

### Blocks

A `do` block is a list of expressions. The result of the last expression is the result of the block.

```
do
  some_work()   ; executed, but ignored
  other_work()  ; block result
end
```

A `do` block also allows functions to be defined with `def`. `def` assigns a function to a variable, which is in scope for the whole block. Functions defined with `def` can refer to themselves recursively.

```
do
  inf_loop()    ; this is fine since the function is defined for the whole block

  def bar(x) {:bar, x}

  def inf_loop() do
    inf_loop()
  end
end
```

### Modules

Cassette programs are composed of modules. The body of the module is a `do` block, and can define functions with `def`. A module can export some of its defined functions and import other modules. A module can reference imported functions by qualifying them with the module name or alias. Module declaration, import, and export statements must appear first in a module (its "header").

```
module Foo
import Bar (bar_fn), LongModuleName as LMN
export foo, foo2

def foo(x)
  let y = Bar.parse(x)
  in LMN.run(y)

def foo2(x) :unimplemented

bar_fn(x)    ; no qualifier needed
```

### Primitives

Built-in functions are executed via the `trap` pseudo-function. These trap calls should usually be wrapped in a module function at runtime for convenience. A reference of currently-implemented traps is available [here](https://cassette-lang.com/primitives.html).
