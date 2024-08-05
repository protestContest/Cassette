![Cassette](https://cassette-lang.com/banner.png)

[Cassette](https://cassette-lang.com) is a small, Lisp-like programming language. It looks like this:

```cassette
import List, Math, Canvas, Time

let width = 800,
    height = 480,
    canvas = Canvas.new(width, height)

Math.seed(Time.now())

def rand_line(i) do
  let x0 = i * width / 100,
      y0 = Math.random_between(20, height / 10),
      x1 = Math.random_between(0, width),
      y1 = Math.random_between(20, height)
  Canvas.line({x0, y0}, {x1, y1}, canvas)
end

List.map(\i -> rand_line(i), List.range(0, 100))
```

## Press _Play_

I made Cassette as a simple language for personal programming. It's designed for solo programmers working on non-enterprise projects. It's DIY, roll your own, batteries-not-included. It's for fun. Try writing a generative art program or a hash-array-mapped trie.

Cassette aspires to be as simple as possible while still including the features I want. These are the primary features I included:

- Functional
- Immutable values
- Efficient tail-call recursion
- Garbage collected
- Modules

## Getting Started

This project requires a C build toolchain and SDL2.

1. Get the project's dependencies
  - On macOS with Homebrew, run `brew install llvm git sdl2 sdl2_ttf`
  - On Debian, run `apt install build-essential clang git libsdl2-dev libsdl2-ttf-dev libfontconfig-dev`
2. Build the utility library, Univ
  - Clone with `git clone https://github.com/protestContest/Univ` (and then `cd Univ`)
  - Run `make` to build the library
  - Run `make install` to install the library to `~/.local/`. (You can change this in the makefile.) Make sure the install location is somewhere your build system can find it.
3. Build Cassette
  - Clone Cassette with `git clone https://git.sr.ht/~zjm/Cassette` (and then `cd Cassette`)
  - Run `make` to build the project. This creates the executable `cassette`.
4. Try the example with `./bin/cassette test/test.ct`.

## Syntax

<div class="columns">
Cassette has two number types, integers and floats. Integers can be written in decimal, hexadecimal, or as a character. The normal infix arithmetic operations work on numbers, and bitwise operations work on integers.

```cassette
-1

1                 ; decimal integer
0x1F              ; hex integer
$a                ; => 0x61

-4                ; => -4
1 + 2             ; => 3
5 * 5             ; => 25
10 / 2            ; => 5.0
12 % 10           ; => 2

0xF0 >> 4         ; => 0x0F (shift right)
0x55 << 1         ; => 0xAA (shift left)
0x37 & 0x0F       ; => 0x07 (bitwise and)
20 ^ 1            ; => 21   (bitwise or)
~4                ; => 3    (bitwise not)
```
</div>

<div class="columns reverse">
Cassette has symbols, which represent an arbitrary value. Comparison operators only work on numbers, but equality operators work on any type. The `and` and `or` operators are short-circuiting. Equality is tested by identity, not structurally!

```cassette
:ok
:not_found

3 > 1             ; => true
3 < 3             ; => false
5 <= 4 + 1        ; => true
5 >= 4 + 1        ; => true
:ok == :ok        ; => true
:ok != :ok        ; => false
```
</div>

<div class="columns">
Cassette has three keyword values: `true`, `false`, and `nil`. `true` and `false` are `1` and `0`, respectively, while `nil` is the empty list, `[]`. In boolean operations, all values are truthy except `false` and `nil`. (This means `0` and `[]` are also falsey.)

```cassette
3 > 1 and 4 == 5  ; => false
3 > 1 or 4 == 5   ; => true
3 >= 0 and 3 < 5  ; => true
nil and :ok       ; => false
:error and :ok    ; => true
```
</div>

<div class="columns">
Strings are UTF-8 encoded binaries. You can find the length of a binary, concatenate two binaries, and slice a binary. Binaries also represent other arbitrary byte sequences, such as the contents of a file.

```cassette
"Hello!"
#"Hello!"         ; => 6
#""               ; => 0

"Hi " <> "there"  ; => "Hi there"
"foo" <> "bar"    ; => "foobar"

"ABCD"[1]         ; => 66 (ASCII "B")
"ABCD"[1:2]       ; => "B"
```
</div>

<div class="columns reverse">
Cassette has Lisp-style cons pairs, which form linked lists. Pairs are formed with the `|` operator, and can be used to easily prepend values to a list. `nil` is the empty list. You can find the length of a list, concatenate two lists, and slice a list.

```cassette
[1, 2, 3]         ; list
[1, 2, 3][2]      ; => 3
[1, 2, 3][8]      ; error: out of bounds
[:a, x, 42]
nil == []         ; => true

1 | [2, 3, 4]     ; => [1, 2, 3, 4]
3 | nil           ; => [3]
1 | 2 | 3 | nil   ; => [1, 2, 3]

#[1, 2, 3]        ; => 3
#nil              ; => 0
[1, 2] <> [3, 4]  ; => [1, 2, 3, 4]
[1, 2, 3][0:2]    ; => [1, 2]
```
</div>

<div class="columns">
Cassette has tuples, which are fixed-length arrays of values. Tuples are less flexible than lists, but use less memory and are a little faster to access. You can find the length of a tuple, concatenate two tuples, and slice a tuple.

```cassette
{1, 2, 3}         ; tuple
{1, 2, 3}[2]      ; => 3
{1, 2, 3}[8]      ; error: out of bounds
#{1, 2, 3}        ; => 3
#{}               ; => 0
{1, 2} <> {3, 4}  ; => {1, 2, 3, 4}
{1, 2, 3}[1:]     ; => {2, 3}
```
</div>

<div class="columns">
Variables are defined with `let`. A `do` block can introduce a new scope, and can be used to combine a group of expressions into one. Cassette has lexical scoping.

```cassette
let x = 1, y = 2

do
  let y = 3, z = 4

  x       ; => 1 (from the parent scope)
  y       ; => 3 (shadows the parent `y`)
  z       ; => 4
end

x         ; => 1
y         ; => 2
z         ; error: undefined variable
```
</div>

<div class="columns reverse">
Cassette has `if`/`else` blocks and `cond` blocks for conditionals. A `cond` block will evaluate each predicate until one is true, then evaluate that clause.

```cassette
if x == 0 do
  IO.print("Uh oh!")
  :error
else
  :ok
end

cond do
  x > 10    -> :ten
  x > 1     -> :one
  true      -> :less  ; default
end
```
</div>

<div class="columns">
Lambdas can be created with a backslash, argument list, and an arrow. The `def` syntax is syntactic sugar for defining a lambda as a variable, with the distinction that `def`-declared functions have block scope, so they can be called recursively. Functions are called with parentheses.

```cassette
let foo = \x -> x + 1
foo(3)            ; => 4

; equivalent, except for scope:
let foo = \x -> x + 1
def foo(x) x + 1

; these produce an error, since `b` isn't defined when the body of `a` is compiled
let a = \x -> (b x * 3),
    b = \x -> (a x / 2)

; these are ok, since `a` and `b` are in scope from the beginning of the block
def a(x) b(x * 3)
def b(x) a(x / 2)
```
</div>

<div class="columns reverse">
Cassette programs can be split up into different modules, one per file. Any top-level `def`-defined functions can be exported. Imported modules can be aliased.

<div>
```cassette
; file "foo.ct"
module Foo
export bar

def bar(x) x + 1
```

```cassette
; file "main.ct"
import Foo        ; imported as a map called `Foo`

Foo.bar(3)        ; => 4
```

```cassette
; alternative "main.ct"
import Foo as F   ; imported as `F`

def bar(x) x + 8

bar(3)            ; => 11
F.bar(3)          ; => 4
```
</div>
</div>

## More Info

For more information about Cassette, check out some of these other documents. Stay tuned for future articles.

- [Function Reference](https://cassette-lang.com/reference.html): A reference of built-in modules and functions
