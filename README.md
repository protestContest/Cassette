![Cassette](https://cassette-lang.com/assets/banner-wide.svg)

Cassette is a small, Lisp-like language. It is a [home-cooked meal](https://www.robinsloan.com/notes/home-cooked-app/). It looks like this:

```cassette
def (take list n) do
  if n <= 0 or list == nil do
    nil
  else
    let cur = head list,
        rest = take (tail list) n-1
    cur | rest
  end
end

def (drop list n) do
  if n == 0 or list == nil do
    list
  else
    drop (tail list) n-1
  end
end

def (chunk list chunk_size) do
  if #list < chunk_size do
    [list]
  else
    (take list chunk_size) | (chunk (drop list chunk_size) chunk_size)
  end
end
```

<aside>
Please direct any comments, questions, and insults [here](mailto:cassette@zjm.me).
</aside>

## [To Do](#to-do)

- Maps (hash array mapped tries)
- Destructured assignment
- Graphics primitives

## [Building](#building)

This project requires a C build toolchain. You may need to install `make` and `clang`. The source code can be found [here](https://git.sr.ht/~zjm/Cassette).

- Clone the repo with `git clone https://git.sr.ht/~zjm/Cassette`.
- Run `make` to build the project. This creates the executable `cassette`.
- Run the examples with `./cassette test/entry.ct test/lib.ct` or `./cassette -p test/project.txt`

## [Syntax](#syntax)

- Numbers: `3.14`, `-5`
- Symbols: `:error`, `:file_not_found`
- Binaries: `"Hello world!"`, `"He said \"no\""`
- Lists: `[1 2 3]`, `nil` (the empty list)
- Tuples: `{1 2 3}`
- Lambdas: `(x y) -> x + y`
- Operators (highest precedence first):
  - `-x`, `not x`, `#list` (length), `~x` (bitwise complement)
  - `x * y`, `x / y`, `x % y`
  - `x + y`, `x - y`
  - `x << y`, `x >> y`
  - `x & y` (bitwise and), `x ^ y` (bitwise or)
  - `x | y` (cons)
  - `x <> y` (concatenate)
  - `x in list`
  - `x < y`, `x > y`, `x <= y`, `x >= y`
  - `x == y`, `x != y`
  - `x and y`, `x or y` (short-circuiting)
- Assignment: `let x = 1, y = 2`, `def (foo x) x + 1`
- Do, if, and cond blocks
- Modules, imports

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
member          → concat | member "in" concat
concat          → pair | conat "<>" pair
pair            → sum | sum "|" pair
bitwise         → shift | bitwise "&" shift | bitwise "^" shift
shift           → sum | shift ">>" sum | shift "<<" sum
sum             → product | sum "+" product | sum "-" product
product         → unary | product "*" unary | product "/" unary | product "%" unary
unary           → access | "-" access | "not" access | "#" access | "~" access
access          → primary | access "." primary

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
