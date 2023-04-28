![Cassette Logo](https://zjm.me/content/uploads/2023/04/Cassette_banner-wide.svg)

# Cassette

Cassette is a small language intended for casual solo programming on simple
systems. It looks like this:

```
def (fizzbuzz n) do
  def (loop m) do
    cond do
      (rem m 3) == 0 and
          (rem m 5) == 0  :: print "FizzBuzz"
      (rem m 3) == 0      :: print "Fizz"
      (rem m 5) == 0      :: print "Buzz"
      true                :: print m
    end

    if m < n do
      loop m + 1
    end
  end

  loop 1
end

fizzbuzz 100
```

## To Do

- String support
- Module loading
- Compiler optimization
  - Open code primitives
  - Lexical variable lookup
- Garbage collection
- Better error handling for primitives
- VM optimization
- Standard library
  - Collections
  - Math
  - I/O ports
  - Graphics

## Building

Building Cassette requires SDL2—edit the makefile to ensure it can find SDL2's
library and headers.

Use `make` to build the project. This creates the executable `dist/cassette`,
which can be called with a file to execute, or with no arguments to start a
REPL.

## Values and Memory

The base of the runtime is the memory representation. Values are 32-bit NaN-boxed floats: any floating point number represents itself, except when it's NaN, the unused 23 bits encode a type and value.

```
         31      24       16       8       0
         ┌┴┬──────┴─┬──────┴───────┴───────┴┐
   Float │S│EEEEEEEE│MMMMMMMMMMMMMMMMMMMMMMM│
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

### Numbers

Numbers can be written in decimal or hexadecimal. Decimal numbers without a decimal point and hexadecimals become integers, and decimals with a decimal point become floats.

Four infix arithmetic operators are supported: `+`, `-`, `*`, `/`.  Addition, subtraction, and multiplication with integers results in an integer. Division or arithmetic involving floats results in a float.

```
3 + 1       ; => 4
3 + 1.0     ; => 4.0
3 * 5       ; => 15
15 / 3      ; => 5.0
```

### Symbols

Symbol values are a hash of the symbol name they represent. Since their purpose is to simply represent themselves, no extra storage is necessary, but the symbol names are stored in a separate hash table.

```
:foo == :foo    ; => true
:bar            ; => :bar
:bar == "bar"   ; => false
```

### Comparison & Equality

Eight infix operators for comparison, equality, and logic are supported: `<`, `<=`, `>=`, `>`, `==`, `!=`, `and`, `or`. These evaluate to the symbols `:true` or `:false` (usually written with the keywords `true` and `false`). The logical operators are short-circuiting. In logic and conditional expressions, only `false` (the symbol `:false`) and `nil` (the empty list `[]`) are considered false.

Comparison is only defined for numbers, which are compared by value. Equality is done by identity.

```
3 and true    ; => true
3 and false   ; => false
3 and nil     ; => false
3 and []      ; => false

:foo == :foo  ; => true
3 == 3.0      ; => true
[3] == [3]    ; => false
```

### Heap-Allocated Objects

A pair is stored in the heap with its head at the value index and the tail just after. Other objects stored in the heap begin with a header value that specifies the object type and other info. The top three bits of a header encode the object type, and the rest encode some data about the object, usually its length. The object's data follows the header.

Heap objects are persistent, so updates to them usually result in a new value, and references to the old value are unchanged. Some special functions are available that do mutate these structures, though.

```
         31     24      16       8       0
         ┌┴──────┴───────┴───────┴───────┴┐
   Tuple │000      <number of items>      │
    Dict │001     <number of entries>     │
  Binary │010      <number of bytes>      │
Reserved │011-----------------------------│
Reserved │100-----------------------------│
Reserved │101-----------------------------│
Reserved │110-----------------------------│
Reserved │111-----------------------------│
         └────────────────────────────────┘
```

### Lists

Lists are Lisp-style linked lists of head/tail pairs. A list of linked pairs can be created with the list syntax, with optional commas. The special pair `nil` points to the first value in memory at index 0, which should be a pair with `nil` as its head and tail. `nil` as the tail of a pair indicates the end of the list, thus `nil` is equivalent to the empty list `[]`. `nil` is one of two values, along with `false`, that evaluates to false in conditional expressions.

```
[1, 2, 3]
```

A pair can be created with the pair syntax. When the tail is a list, this extends the list with a new head.

```
let list = [3, 2, 1]
[4 | list]          ; => [4, 3, 2, 1]
[4 | [3, 2, 1]]     ; => [4, 3, 2, 1]
[:a | :b]           ; => [:a | :b] (an "improper" list, since the tail isn't a list)
[3 | nil]           ; => [3]
nil == []           ; => true
```

### Tuples

Tuples are fixed-size arrays. A tuple stores its values sequentially after the header, and can be accessed by their index. A tuple can be created with the tuple-literal syntax, with optional commas: `#[1, 2, 3]`.

### Dicts

Dicts are key-value dictionaries. A dict stores a tuple just after the header that points to the keys, followed by the associated values. A dict can be created with the dict-literal syntax, with optional commas. A dict can be updated with the dict-update syntax. A dict can be accessed with the `.` operator. A dict's keys can be any value, but only symbol keys are supported in the dict-literal and `.` syntaxes.

```
let d = {a: 1, b: 2}
d.a                   ; => 1
d.foo                 ; => nil
{c: 3 | d}            ; => {a: 1, b: 2, c: 3}
{a: 0 | d}            ; => {a: 0, b: 2}
```

### Binaries & Strings

Strings in Cassette are represented only as "binaries", a sequence of arbitrary bytes. Binaries can be created from UTF-8 strings with double quotes:

```
"Hi!"  ; bytes 0x48, 0x69, 0x21
"水"    ; bytes 0xE6, 0xB0, 0xB4
```

A binary stores its data just after the header, padded to four bytes. The number of 4-byte words a binary uses (excluding the header) can be calculated with `((length - 1) / 4) + 1`. Strings are represented by binaries.

### Lambdas

A lambda is represented as a list beginning with the symbol `λ`, followed by a list of parameter symbols, a reference to the function body, and a reference to the function environment. Lambdas can be created with the arrow syntax.

```
(a b c) -> a * b + c
params -> (length params)
```

### Garbage Collection

Garbage collection will be implemented in a future version.

## Program Structure

The top-level structure of a program is a list of expressions separated by newlines (a "block"). An expression can be a special expression or a function call.

### Blocks & Function Calls

A function call is a space-separated list of arguments. Nested calls can be disambiguated with parentheses. When a value is called with no arguments, it evaluates to itself. A function call cannot contain newlines, except after an infix operator or when inside parentheses.

```
foo 3 4 + 1     ; `foo` called with two arguments
foo 3 4 +
      1         ; line breaks ok after infix operators
foo 3 (bar 4
           5)   ; line breaks ok in parentheses
```

The `do` special form introduces a new block, and evaluates to the value of the last expression in the block.

```
foo 3 4 do
  print "Evaluating third argument"
  5
end

; Calls `foo` with arguments 3, 4, and 5
```

### Environments & Scope

Cassette uses normal lexical scoping and a standard Lisp-style environment model. Lambdas capture the environment in which they are defined, and applying a function extends the environment with a frame that binds the function's arguments to its parameters. The `let` keyword can define or redefine additional variables in the current frame. `let`-defined variables are in scope for the whole frame.

```
(a b) -> do
  let x = 1, b = 2
  x       ; => 1
  a       ; => <whatever was passed as a>
  b       ; => 2
  y       ; => :foo
  z       ; Error, undefined variable "z"

  let y = :foo
end
```

The `def` keyword is syntactic sugar for defining functions.

```
; These are equivalent:

def (foo a b) do
  a + b
end

let foo = (a b) -> do
  a + b
end
```

### Conditionals

The `if` and `cond` special expressions allow conditional control flow. `if` evaluates one of two blocks based on a condition. When the false branch is omitted, it returns `nil`.

```
if x == y do
  print "They're equal!"
  :ok
else
  print "Not equal"
  :error
end

if false do
  :impossible
end
; => nil

let x = if y == nil do 0 else y + 1 end
```

`cond` evaluates a list of conditions until one is true, then evaluates that condition's consequent.

```
cond do
  x == y    :: print "They're equal!"
  false     :: :impossible
  y != nil  :: y + 1
  else      :: 0
  1 == 1    :: :impossible    ; since the `else` clause is always true
end
```

### Import

The `import` special expression can import definitions from another file. The file is evaluated in a separate top-level environment, and the definitions there are either defined in the importing environment or defined in a dict, which is then defined in the importing environment.

```
; "foo"
def (foo a b) do a + b end

; "bar"
def (bar a b) do a - b end

; "baz"
import "foo"          ; => all definitions in "foo" are defined here, too
import "bar" as Bar   ; => definitions in "bar" are available in the dict Bar

foo 3 4               ; => 7
bar 3 4               ; Error, undefined variable "bar"
Bar.bar 3 4           ; => -1
```

## Input & Output

Cassette uses the "port" concept for I/O. The `open` function opens a port of a specified type. Then the `send` function can send a message to that port. The structure of the message depends on the type of port. Currently supported port types are `console`, `file`, and `window`. The `receive` can be called to take an incoming message from a port. If no message is waiting, `receive` returns `nil`.

A future version will support concurrent threads that can communicate via ports.

### File

A `:file` port represents a file for reading/writing. File ports are opened by filename, which is created if the file doesn't exist. Data sent to a file port is written to the file. Calling `receive` on a file port returns the next byte in the file, or `:eof` when the end of file is reached.

```
let f = open :file "test.txt"
send f "Hello world!"           ; writes a string to a file
```

### Console

A special port of type `:console` represents a text console. Only one `:console` port exists. The console behaves similarly to a file port, but reads and writes data to and from the console. The `print` function is a convenience for writing to the console.

```
let c = open :console
send c "Hello, World!"

; an easier way of doing the same thing
print "Hello, World!"
```

### Window

A `:window` port represents a graphics window. Window ports are opened by name, which becomes the title of the window. A dict of options can be passed when opening a window to set its size and position.

A window can be sent drawing commands. Currently, only the `:line` command is supported.

When a window is opened or gains focus, it becomes the "top window". The top window can be found with the `top-window` function. Some functions implicitly send to and receive from the top window.

Events can be received from a window. The format of an event message is not yet defined.

```
let win = open :window "Canvas" {width: 400 height: 400}
send win #[:line 20 30 300 100]

let event = receive win
; ¯\_(ツ)_/¯
```
