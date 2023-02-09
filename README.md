# Rye

Rye is a personal scripting language inspired by Elixir and Scheme. It's a Lisp
with infix operators and optional parentheses.

## Syntax

### Comments

Comments start with a semicolon and end at the end of a line:

```
; this is a comment
```

### Reserved Words

Rye has a few reserved words:

```
and cond def do else end false if nil or not true
```

The operators (see below) are also reserved.

### Identifiers

Identifiers are fairly general. An identifier can use any printable characters,
except for these: `( ) { } [ ] " : ; . ,`. Additionally, identifiers cannot
begin with a digit.

```
hello
fourtwenty69
thread-case
snake_case
camelCase
#variable
set!
```

### Expressions

Everything in Rye is an expression. Expressions can be combined with operators.

```
; Expressions:
foo
x + 3
-2 * (3 + 1)
x = 0 or x = nil
```

The operators in Rye are, in order of precedence:

<table>
  <tr><td><code>and</td><td>logical and
  <tr><td><code>or </td><td>logical or
  <tr><td><code>=  </td><td>equal
  <tr><td><code>!= </td><td>not equal
  <tr><td><code>&gt;  </td><td>greater than
  <tr><td><code>&lt;  </td><td>less than
  <tr><td><code>&gt;= </td><td>greater than or equal
  <tr><td><code>&lt;= </td><td>less than or equal
  <tr><td><code>|  </td><td>pair construction
  <tr><td><code>+  </td><td>add
  <tr><td><code>-  </td><td>subtract
  <tr><td><code>*  </td><td>multiply
  <tr><td><code>/  </td><td>divide
  <tr><td><code>-  </td><td>unary negative
  <tr><td><code>not</td><td>logical not
  <tr><td><code>.  </td><td>dict access
</table>

### Function Calls

A function call is a sequence of space-separated expressions:

```
move-to 300 100
line-to (with-margin width) (with-margin height)
```

A function without arguments is not called, unless it's surrounded by parens:

```
send-alert        // not called
send-alert :now   // called
(send-alert)      // called
(send-alert :now) // called
```

### Blocks

Calls can be sequenced in a `do` expression. A `do` expression evaluates to the
last call in the sequence. Calls in a `do` expression are separated by newlines.

```
do
  send-alert "admin@example.com"
  (halt-system)

  ; multi-line call is ok in parens
  (log (current-time)
       "Error"
       error)

  :ok
end
```

## Values

### Numbers

Numbers are either integers or floats. Addition, subtraction, and multiplication
with integers will result in an integer; any other arithmetic results in a
float. Floats can be cast to integers with some functions, like `floor` and
`ceil`. Underscores in numbers are ignored.

```
2       ; int
3 * 4   ; int
3 + 0.1 ; float
12 / 3  ; float
3.14    ; float
2.0     ; float
```

### Strings

Strings are a sequence of bytes. It's convenient when strings are valid UTF-8.

```
"Hello world!"
```

### Symbols

A symbol is a value that's equal to itself. Symbols are useful for making unique
human-readable values.

```
:ok
:error
:some-really-long-symbol
```

### Booleans

Two symbols, `:true` and `:false`, can be written without colons: `true`,
`false`. The symbol `:false` is one of only two values (along with `nil`) that
evaluates to false in boolean expressions.

### Nil

The special value `nil` represents an empty list. `nil` is a list, whose head is
`nil` and tail is `nil`. `nil` evaluates to false in boolean expressions.

## Lists

Lists in Rye are linked lists of pairs, similar to Lisp. A pair can be created
with the `|` operator:

```
x | y     ; creates a pair with a head x and tail y
```

A list is a pair whose head is the first value and tail is the rest of the list.
A list can be created with square brackets, and a sequence of comma-separated
expressions.

```
[a, b, 34, :foo]    ; creates a list
1 | [2, 3, 4]       ; creates a list [1, 2, 3, 4] by pairing a new head
```

List elements can be accessed by calling the list as a function:

```
; my-list is [1, 2, 3, 4]
(my-list 0)   ; 1
(my-list 3)   ; 4
(my-list -1)  ; 4
(my-list 4)   ; nil
```

## Maps

A map is a key-value store. Keys must be symbols. A map can be
created with braces:

```
{x: 100, y: 200}
```

A map element can be accessed by calling it as a function:

```
(my-map :x)    ; 100
(my-map :foo)  ; nil
```

A map can also be accessed with the `.` operator:

```
my-map.x    ; 100
my-map.foo  ; nil
```

## Special Forms

### Closures

Anonymous functions can be created with the arrow syntax:

```
(x y) -> x * y      ; function of two arguments
() -> :ok           ; function of no arguments
```

Anonymous functions create a new scope for their arguments, and return a
closure that remembers their lexical scope.

### Define

Variables can be defined in the current scope with `def`:

```
def x 100                 ; x becomes 100
def foo (x y) -> x * y    ; foo becomes a function of two arguments
```

A special syntax makes defining functions easier:

```
def (foo x y) -> x * y    ; foo becomes a function of two arguments
```

### Conditionals

Conditional control flow can be done with `if`:

```
; if with a consequent and alternative
if x = 0 do
  :ok
else
  :error
end

; if with no alternative (implicitly nil)
if y > 0 do
  y / 2
end

; short if form (implicit nil alternative)
if (valid? x) :ok
```

### Modules

A script can define modules with `module`:

```
module Math do
  def (lerp a b t) do
    t * (b - a) + a
  end
end
```

A module expression returns a map with all definitions in it. It also registers
the module so it can be imported later. Modules are imported with `import`:

```
import Math

Math.lerp 50 100 0.5    ; 75
```

When a script is executed, it's directory tree is searched for ".rye" files. For
each file, the top-level module expressions are evaluated and saved in a module
list. Later, when an import expression is evaluated, it defines that module in
the current environment.
