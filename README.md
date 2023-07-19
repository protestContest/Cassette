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
Feedback is welcome — if anything is unclear or you spot any mistakes, please [drop me a note](mailto:cassette@zjm.me).
</aside>

## [To-Do](#to-do)

Cassette is in early stages of development. Not everything described here has been implemented, and everything is subject to change.

- Source context on runtime errors
- Module loading (90%)
- Compiler optimization
- VM optimization
- Standard library
  - Collections
  - Math
  - I/O ports
  - Graphics
- Documentation

## [Building](#building)

This project requires a C build toolchain. You may need to install `make` and `clang`.

The source code can be found [here](https://git.sr.ht/~zjm/Cassette). This project depends on the [univ](https://git.sr.ht/~zjm/univ) library.

- Clone the repo with `git clone https://git.sr.ht/~zjm/Cassette`.
- Run `make deps`. This clones, builds, and copies the univ library into this project. (Alternatively, build that library separately and copy the files into "lib" and "include".)
- Run `make` to build the project. This creates the executable `cassette`, which can be called with a file to execute, or launch a REPL. `make test` will run the file `test/test.csst`.

## [Values & Types](#values-types)

Cassette is dynamically typed. The basic data types in Cassette are:

- Numbers
- Symbols
- Pairs (which can be linked to form lists)
- Tuples
- Binaries (also used to represent strings)
- Maps

Numbers are either 32-bit signed integers or 32-bit IEEE 754 floating point numbers. Constants written with a decimal, like `31.0`, are floats; without a decimal, like `31`, are integers. Integers can also be written in hexadecimal, like `0x5A4D`. Numbers are converted between floating point and integer based on the operations applied to them.

A _symbol_ is a unique value that simply represent itself. A symbol is written like this: `:ok`, `:error`, `:symbols_can_be_really_long_but_maybe_shouldn't_be`. Two special symbols, `:true` and `:false`, represent boolean values, and can be written simply as `true` and `false`.

A _pair_ is a value that contains two values, a _head_ and a _tail_. A pair can contain any two values. Pairs are created with the pipe operator: `1 | 2`. Conventionally, pairs are used to form linked lists: a list is a pair with the first item as the head and the rest of the list (another pair) as the tail. A special value, `nil`, is used as the tail of the last pair in the list. `nil` is also a pair, with `nil` as its head and `nil` as its tail. `nil` is considered to be an empty list. Lists can be constructed from pairs, like this: `1 | 2 | 3 | 4 | nil`, but it's easier to use the list syntax: `[1, 2, 3, 4]` (commas optional).

A _tuple_ is a fixed-size set of items. Tuples are a _persistent data structure_, which means when you change a tuple, the old version still exists (in case some other code is still using it). A tuple is similar to a list, but is more efficient to access items from. On the other hand, it's less efficient to change a tuple. Tuples can be created like this: `#[1, 2, 3, 4]` (commas optional).

A _binary_ is a sequence of bytes. Binaries are used to represent strings and other binary data. Cassette is _encoding-agnostic_, making no assumptions about the contents of a binary. Binaries can be created with the string syntax, like `"hello, world!"`, or by converting a list of integers. Binaries, unlike pairs, tuples, and maps, are not persistent: the contents of a binary can be changed in-place.

A _map_ is a key-value map, known elsewhere as an _associative array_ or a _dictionary_. Maps can be written like this: `{foo: 1, bar: "ok"}` (commas optional). In this case, the keys are the symbols `:foo` and `:bar`. When creating maps like this, only symbols can be used as keys, but any value can be used as a key when using map functions like `Map.put`. Maps, like tuples and pairs, are persistent data structures.

Pairs, tuples, binaries, and maps are _object types_: their data is stored in memory on the heap, and the values are references to that data.

## [Syntax](#syntax)

Cassette supports these infix operators:

- Arithmetic: `+`, `-`, `*`, `/`
- Exponentiation: `^`
- Comparison: `>`, `<`, `>=`, `<=`, `==`, `!=`
- Membership: `in` (e.g. `3 in my_list`)
- Pair construction: `x | y`
- Logic: `and`, `or` (short-circuiting)

and these prefix operators:

- Negative: `-`
- Logic: `not`
- Length: `#` (e.g. #my_list)

Collections can be created like these:

- Lists: `[1, 2, 3]` or `[1 2 3]`
- Tuples: `#[1, 2, 3]` or `#[1 2 3]`
- Maps: `{foo: 3, bar: 4}`
- Binaries: `"I am a collection of bytes"`

Collections can be accessed by calling them as functions:

```
let list = [1 2 3]
(list 1)            ; => 2

let tup = #[4 5 6]
(tup 2)             ; => 6

let bin = "hello!"
(bin 0)             ; => 104 (ASCII code for 'h')

let map = {x: 3, y: 4}
(map :y)            ; => 4
```

Symbols look like this: `:foo`, `:bar`.

Variables can be defined like this: `let x = 1, y = 2`.

Anonymous functions look like this: `(x y) -> x + y`.

Functions can be defined with `let` and an anonymous function, or like this:

`def (foo x y) x + y`

```
def (bar x y) do
  print x
  x + y
end
```

A `do` block can execute multiple expressions, returning the last one.

```
do
  print x
  print y
  :ok
end
```

A function is called with parentheses, like this: `(foo 2 3)`. In `do` blocks and `if` blocks, the parentheses are optional if there is at least one argument.

Conditionals can be an `if` block or a `cond` block:

```
if x == 0 do
  :error
else
  y / x
end

if not (test x) do
  log "Failed!"
end

cond do
  x > 1024*1024 -> "M"
  x > 1024      -> "K"
  true          -> "B"
end
```

## [Reference](#reference)

Here are some intended standard functions:

- `(List.from list n)`: returns the list starting from the nth element
- `(List.take list n)`: returns the first n elements of a list
- `(List.slice list begin end)`: returns a list between `begin` (inclusive) and `end` (exclusive)
- `(List.concat a b)`: concatenates two lists
- `(List.append a b)`: appends `b` to the end of list `a`
- `(List.flatten list)`: flattens a list
- `(List.to_binary list)`: converts an IO list to a binary
- `(List.to_tuple list)`: converts a list to a tuple
- `(List.to_map list)`: converts a list of pairs to a map
- `(List.insert list n value)`: returns a list with `value` inserted at position `n`
- `(List.zip a b)`: returns a list of pairs taken from the lists `a` and `b`
- `(List.unzip list)`: given a list of pairs, returns a pair of lists; the first containing the heads, the second containing the tails

- `(Map.get map key)`: gets `key` from a map, or nil
- `(Map.put map key value)`: returns a map with `key` set to `value`
- `(Map.delete map key)`: returns a map without `key`
- `(Map.keys map)`: returns a tuple of map keys
- `(Map.values map)`: returns a tuple of map values
- `(Map.to_list map)`: returns a list of (key, value) pairs

- `(Tuple.to_list tuple)`

- `(Math.ceil n)`
- `(Math.floor n)`
- `(Math.round n)`
- `(Math.abs n)`
- `(Math.sqrt n)`
- `(Math.log n)`
- `(Math.exp n)`
- `(Math.E)`
- `(Math.sin n)`
- `(Math.cos n)`
- `(Math.tan n)`
- `(Math.Pi)`

- `(Vec.add a b)`: vector addition
- `(Vec.sub a b)`: vector subtraction
- `(Vec.dot a b)`: dot product
- `(Vec.cross a b)`: cross product
- `(Matrix.id n)`
- `(Matrix.mul a b)`
- `(Matrix.inverse m)`
- `(Matrix.determinate m)`

- `(Graphics.line a b)`
- `(Graphics.curve a c1 c2 b)`: cubic bezier
- `(Graphics.width)`
- `(Graphics.height)`

## [Memory Representation](#memory-representation)

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

Symbol values are a 20-bit hash of the symbol name, which is stored in a hash table.

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
