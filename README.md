![Cassette](https://cassette-lang.com/assets/banner-wide.svg)

Cassette is a small, Lisp-like language. It looks like this:

```cassette
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
Please direct any comments, questions, and insults [here](mailto:cassette@zjm.me).
</aside>

## [Building](#building)

This project requires a C build toolchain. You may need to install `make` and `clang`. The source code can be found [here](https://git.sr.ht/~zjm/Cassette).

- Clone the repo with `git clone https://git.sr.ht/~zjm/Cassette`.
- Run `make` to build the project. This creates the executable `cassette`, which can be called with a file to execute, or a manifest file to run a collection of modules.

## [Syntax](#syntax)

- Numbers: `3.14`, `-5`
- Symbols: `:error`, `:file_not_found`
- Binaries: `"Hello world!"`, `"He said \"no\""`
- Lists: `[1 2 3]`, `nil` (the empty list)
- Tuples: `{1 2 3}`
- Lambdas: `(x y) -> x + y`
- Operators:
  - `-x`, `not x`, `#list` (length)
  - `x * y`, `x / y`, `x % y` (remainder)
  - `x + y`, `x - y`
  - `x | y` (cons)
  - `x in list`
  - `x < y`, `x > y`, `x <= y`, `x >= y`
  - `x == y`, `x != y`
  - `x and y`, `x or y`
- Assignment: `let x = 1, y = 2`, `def (foo x) x + 1`
- Do, if, and cond blocks

Here's the full grammar:

```
script          → module_def imports stmts | imports stmts
module_def      → "module" ID
imports         → import_stmt imports | ε
import_stmt     → "import" ID "\n"

stmts           → stmts stmt | ε
stmt            → let_stmt | def_stmt | call_stmt
let_stmt        → "let" assigns "\n"
assigns         → assigns "," assign | assign
assign          → ID "=" call
def_stmt        → "def" "(" ID args ")" call "\n"
args            → args arg | ε
call_stmt       → call "\n"
call            → args arg

arg             → lambda
lambda          → group "->" arg | ID "->" arg
logic           → equality | logic "and" equality | logic "or" equality
equality        → compare | equality "==" compare | equality "!=" compare
compare         → member
                  | compare "<" member
                  | compare ">" member
                  | compare "<=" member
                  | compare ">=" member
member          → pair | member "in" pair
pair            → sum | sum "|" pair
sum             → product | sum "+" product | sum "-" product
product         → unary | product "*" unary | product "/" unary | product "%" unary
unary           → primary | "-" primary | "not" primary | "#" primary

primary         → group | block | obj | value | ID

group           → "(" call ")"
block           → do_block | if_block | cond_block
do_block        → "do" stmts "end"
if_block        → "if" stmts "else" stmts "end" | "if" stmts "end"
cond_block      → "cond" clauses "end"
clauses         → clauses clause | ε
clause          → logic "->" call

obj             → list | tuple | map
list            → "[" list_tail
list_tail       → "]" | call "]" | call "," list_tail
tuple           → "{" tuple_tail
tuple_tail      → "}" | call "}" | call "," tuple_tail

value           → "true" | "false" | "nil" | SYM | NUM | STR
```

## [Modules](#modules)

A script is a module if it begins with a module declaration, e.g. `module Foo`. The beginning of any script can also have import statements, e.g. `import Bar`. This makes all top-level definitions in `Bar` in scope in the importing module.

A Cassette project is made up of multiple modules. Cassette can be called with the `-p` flag to give it a project file, which is just a list of module files, one per line. The first listed file is the entry-point.

## [Memory Representation](#memory-representation)

Cassette is a garbage-collected language. The managed memory system proved useful not only in the runtime, but also in the compiler for easily representing data structures.

Values are represented as 32-bit NaN-boxed floats. Any floating point number represents itself, except when it's NaN, the unused 23 bits encode a type and value.

```
         31      24       16       8       0  Bitmask
         ┌┴┬──────┴─┬──────┴───────┴───────┴┐
   Float │S│ <expnt>│       <mantissa>      │
     NaN │-│11111111│1----------------------│ 0x7FC00000
         ├─┼────────┼───────────────────────┤
 Integer │0│11111111│100 <signed int value> │ 0x7FC00000
  Symbol │0│11111111│101   <symbol hash>    │ 0x7FD00000
    Pair │0│11111111│110    <heap index>    │ 0x7FE00000
  Object │0│11111111│111    <heap index>    │ 0x7FF00000
   Tuple │1│11111111│100    <num items>     │ 0xFFC00000
  Binary │1│11111111│101    <num bytes>     │ 0xFFD00000
Reserved │1│11111111│110--------------------│ 0xFFE00000
Reserved │1│11111111│111--------------------│ 0xFFF00000
         └─┴────────┴───────────────────────┘
```

When the exponent is all 1s and any mantissa bit (usually the high bit) is 1, the value represents NaN, and the remaining 23 bits are undefined. Cassette uses the sign bit and bits 20 and 21 to encode a type, and bits 0–19 to encode a raw value. Integers and symbols are represented immediately, but pairs and other objects store an index into the heap where the object data is.

Symbol values are a 20-bit hash of the symbol name, which is stored in a separate symbol table.

A pair points to the first of two values in memory. An object points to an object header in memory (a tuple or binary), which is followed by the object's data. A tuple header stores the number of items it holds, while a binary stores the number of bytes. A binary's data is padded to the nearest 4-byte boundary.
