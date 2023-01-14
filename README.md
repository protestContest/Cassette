# Rye

Rye is a personal scripting language inspired by Elixir and Scheme. It's a Lisp
with infix operators and optional parentheses.

See the file ["test.rye"](https://git.sr.ht/~zjm/Rye/tree/master/item/test.rye) for a sample.

## Expressions

You can have numbers, strings, symbols, and variables. Comments begin with semicolon.

```
3.14            ; number
"hello there"   ; string
:error          ; symbol
width           ; variable
```

You can define variables with the `def` function. You can make anonymous functions with `->`, and assign them to variables. You can also use a lisp-style syntax to define functions.

```
def pi 3.14
def identity (x) -> x
def (identity x) x
```

Parentheses are optional for function calls, but can be used to determine precedence.

```
foo x y
(foo x y)
foo (x y)
```

The usual infix operators work, too.

```
1 + 2           ; => 3
5 / 12          ; => 2.5
3 + 2 * 5       ; => 13
(3 + 2) * 5     ; => 25
2 ** 5          ; => 32 (exponent)

3 > 5           ; => false
4 <= 2 + 2      ; => true
2 + 2 = 5       ; => false
7 != 12         ; => true

true and true   ; => true
false or true   ; => true
not true        ; => false
```

Infix expressions are automatically wrapped in parentheses, so they can be used as arguments.

```
foo 8 - 5     ; => (foo (8 - 5))
```

You can combine basic types into pairs, linked lists (of pairs), tuples, and dictionaries (dicts). You can make ranges with `..`. Dict keys are either strings or symbols. Elements are separated by newlines or commas. Elements are looked up by calling the structure as a function. Dicts can also look up symbolic keys with ".".

```
a | b               ; pair (infix operator)
[1, 2, "ok", 3]     ; list
3..10               ; [3, 4, 5, 6, 7, 8, 9]
10..3               ; [10, 9, 8, 7, 6, 5, 4]
3 | my-list         ; adds 3 to the front of my-list, e.g. [3, 1, 2, "ok", 3]
#[a, b, c]          ; tuple (fixed-size list)

{                   ; dict
  a: 1
  "b": 2
}

(my-list 3)         ; value at index 3 in my-list
(my-tuple 2)        ; value at index 2 in my-tuple
foo.a               ; value for :a in dict foo
(foo :a)            ; value for :a in dict foo
(foo "b")           ; value for "b" in dict foo
```

## Blocks

A block is a list of expressions separated by newlines or commas. An expression can span multiple lines when wrapped in parentheses. Blocks are delimited by `do`/`end`.

```
def (reduce list acc fn) do
  display "reduce"
  (reduce (tail list)
          (fn (head list))
          fn)
end

do display "error", :error end
```

You can do conditionals with case, cond, and if. Arrows in case and cond are clauses, not functions.

```
if true :ok        ; => :ok
if false :ok       ; => nil

if (done? x) do
  :ok
else
  (wait-for-x)
end

cond do
  ((rem n 3) = 0 and
   (rem n 5) = 0) -> display "fizzbuzz"
  (rem n 3) = 0 -> display "fizz"
  (rem n 5) = 0 -> display "buzz"
  true -> display n
end

case x do
  :error  -> display "oops"
  :ok -> (keep-going x)
end
```

You can define variables in a new lexical scope with `let`.

```
let {x: 3, y: 4} do
	display "X: " x "Y: " y
	x * y
end
```

The pipe operator, `|>`, passes the previous expression as the first argument in the next expression. The pipe operator works across newlines, and has lower precedence than expressions.

```
1..10            ; => [1, 2, 3, 4, 5, 6, 7, 8, 9]
|> filter even?  ; => [2, 4, 6, 8]
|> display       ; prints the list
```

## Modules

You can create modules to namespace variables.

```
module Math do
  module Trig do
    def (sin angle) (sin-table (round angle))
  end

  def (dist x1 y1 x2 y2) (sqrt (x2 - x1)**2 + (y2 - y1)**2)
end

module String do
	def (display-both str1 str2) do
		display str1
		display str2
	end
end

Math.dist 2 2 4 8
Math.Trig.sin 30
```

You can use modules defined in other files. The module becomes defined in the current file.

```
use String

String.display-both "hello" "world"
```

You can import modules defined in other files. All definitions in the module become defined in the current file.

```
import Math

dist 3 3 8 7
```

