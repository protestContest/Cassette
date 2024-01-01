![Cassette](https://cassette-lang.com/banner.gif)

Cassette is a small, Lisp-like programming language. It is a [home-cooked meal](https://www.robinsloan.com/notes/home-cooked-app/). It looks like this:

```cassette
import List
import Math
import Canvas
import System

let width = 800,
    height = 480,
    canvas = Canvas.new(width, height)

canvas.text("Lines!", {200, 2})

System.seed(System.time())

def rand-line(i) do
  let x0 = Math.floor(i * width / 100),
      y0 = Math.rand-int(20, height / 10),
      x1 = Math.rand-int(0, width),
      y1 = Math.rand-int(20, height)
  canvas.line({x0, y0}, {x1, y1})
end

List.map(\i -> rand-line(i), List.range(0, 100))
```

<aside>
Please direct any comments, questions, and insults [here](mailto:cassette@zjm.me).
</aside>

## [Press _Play_](#press-play)

I made Cassette as a simple language for "playful programming". Playful programming is writing something for the sake of writing it. It's making a software 3D renderer or a GIF reader, even though better implementations of those already exist. It's making generative art programs and drawing them with a pen plotter. Cassette itself is playful programming—there are certainly other scripting languages that may be better for personal projects like these, but this one is mine.

Here are some of the design goals of Cassette:

- Functional
- Immutable types
- Simplicity over efficiency
- Small implementation
- Few dependencies

In particular, I wanted Cassette to feel "essential", where each aspect of the language reflects some fundamental aspect of computing (from a functional language perspective, at least). For example, I consider garbage collection, lexical scopes, and immutable types essential. The result is a little boring, but I hope it's a good platform to play with other programming concepts.

Here's some future work for Cassette:

- Bignum support
- Generational garbage collection
- Compiler & VM optimization
- Support other backends (WebAssembly, LLVM)
- Destructuring assignment (v2?)
- Pattern-based function dispatch (v2?)

## [Getting Started](#getting-started)

This project requires a C build toolchain and SDL2. The source code can be found [here](https://git.sr.ht/~zjm/Cassette).

1. Clone the repo with `git clone https://git.sr.ht/~zjm/Cassette`.
2. Run `make` to build the project. This creates the executable `cassette`.
3. Optionally, run `make install` to install the Cassette executable. You can set the install folder in the Makefile.
4. Try the example with `./cassette test/test.ct`.
5. Write a little script and run it with `./cassette script.ct`.

## [Syntax](#syntax)

<div class="columns">
Cassette has two number types, integers and floats. Integers can be written in decimal, hexadecimal, or as a character. The normal infix arithmetic operations work on numbers, and bitwise operations work on integers.

```cassette
-1

1                 ; decimal integer
0x1F              ; hex integer
$a                ; => 0x61
1.0               ; float

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
Cassette has symbols, which represent an arbitrary value. `true` and `false` are symbols. In boolean operations, all values are truthy except `false` and `nil`. Comparison operators only work on numbers, but equality operators work on any type. The `and` and `or` operators are short-circuiting.

```cassette
:ok
:not_found

3 > 1             ; => true
3 < 3             ; => false
5 <= 4 + 1        ; => true
5 >= 4 + 1        ; => true
:ok == :ok        ; => true
:ok != :ok        ; => false
3 > 1 and 4 == 5  ; => false
3 > 1 or 4 == 5   ; => true
3 >= 0 and 3 < 5  ; => true
nil and :ok       ; => false
:error and :ok    ; => true
```
</div>

<div class="columns">
Strings are UTF-8 encoded binaries. You can find the length of a string, concatenate two binaries, and test if one string or byte is present in another string. Binaries also represent other arbitrary byte sequences, such as the contents of a file.

```cassette
"Hello!"
#"Hello!"         ; => 6
#""               ; => 0

"Hi " <> "there"  ; => "Hi there"
"foo" <> "bar"    ; => "foobar"

"ob" in "foobar"  ; => true
$r in "foobar"    ; => true
65 in "Alpha"     ; => true
1000 in "foobar"  ; error: bytes must be between 0–255
```
</div>

<div class="columns reverse">
Cassette has Lisp-style cons pairs, which form linked lists. Pairs are formed with the `|` operator, and can be used to easily prepend values to a list. `nil` is the empty list. You can find the length of a list, concatenate two lists, and test if a value is in a list. Lists can be accessed with an integer index.

```cassette
[1, 2, 3]         ; list
[1, 2, 3].2       ; => 3
[1, 2, 3].8       ; error: out of bounds
[:a, x, 42]
nil == []         ; => true

1 | [2, 3, 4]     ; => [1, 2, 3, 4]
3 | nil           ; => [3]
1 | 2 | 3 | nil   ; => [1, 2, 3]

#[1, 2, 3]        ; => 3
#nil              ; => 0
[1, 2] <> [3, 4]  ; => [1, 2, 3, 4]
:ok in [:ok, 3]   ; => true
```
</div>

<div class="columns">
Cassette has tuples, which are fixed-length arrays of values. Tuples are less flexible than lists, but use less memory and are a little faster to access. You can find the length of a tuple, concatenate two tuples, and test if a value is in a tuple. Tuples can be accessed with an integer index.

```cassette
{1, 2, 3}         ; tuple
{1, 2, 3}.2       ; => 3
{1, 2, 3}.8       ; error: out of bounds
#{1, 2, 3}        ; => 3
#{}               ; => 0
{1, 2} <> {3, 4}  ; => {1, 2, 3, 4}
"x" in {"y", "z"} ; => false
```
</div>

<div class="columns reverse">
Cassette has maps (a.k.a. dictionaries), which can be written like tuples with symbol keys. Map literals can only have symbol keys, but other functions can get and set keys of other types. You can find the number of key/value pairs in a map, merge two maps, and test if a map contains a key.

```cassette
{x: 3, y: 4}      ; a map with keys `:x` and `:y`
my_map.x          ; => 3
my_map.z          ; => nil

#{x: 1, y: 2}     ; => 2
{x: 1, z: 4} <> {x: 2, y: 3}
                  ; => {x: 2, y: 3, z: 4}
:x in {x: 1, y: 2}  ; => true
```
</div>

<div class="columns">
Variables are defined with `let`. A `do` block can introduce a new scope, and can be used to combine a group of expressions into one. Cassette has lexical scoping. A variable must start with a letter or underscore, but may contain any characters afterward except for whitespace and these reserved characters: `;,.:()[]{}`. This means that if you want to write an infix expression, you must often include space around the operator to disambiguate it from its operands.

```cassette
let x = 1, y = 2, x-y = 3

print(x - y)     ; prints "-1"
print(x-y)       ; prints "3"
print(x -y)      ; error: tries to call function "x" with argument "-y"

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
Cassette programs can be split up into different modules, one per file. A manifest file is used to list the files in a program, with the first file listed being the entry point. Modules can be imported directly into a file's scope, or aliased as a map.

<div>
```cassette
; file "foo.ct"
module Foo

let pi = 3.14
def bar(x) x + 1
```

```cassette
; file "main.ct"
import Foo        ; imported as a map called `Foo`

Foo.bar(3)        ; => 4
Foo.pi            ; => 3.14
```

```cassette
; alternative "main.ct"
import Foo as F   ; imported as a map called `F`

def bar(x) x + 8

bar(3)            ; => 11
F.bar(3)          ; => 4
```

```cassette
; alternative "main.ct"
import Foo as *   ; imported directly into current scope

bar(3)            ; => 4
pi                ; => 3.14
```
</div>
</div>

## [More Info](#more-info)

For more information about Cassette, check out some of these other documents. Stay tuned for future articles.

- [Function Reference](https://cassette-lang.com/reference.html): A reference of built-in modules and functions
