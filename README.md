# Rye

Rye is a personal scripting language inspired by Elixir and Scheme. It's a Lisp
with infix operators and optional parentheses.

## Basic Types

Rye includes literal numbers, strings, symbols, lists, and dictionaries.

```
32
3.14
5_000_000
"hello!"
:error
[3, 4, "foo"]
{width: 512, height: 342}
```

Rye supports normal infix arithmetic and comparison, with precedence, and prefix
negation and logical not.

```
3 + 4
2 - 1
3.14 * 9
2 / 5
3 = 2 + 1
4 > 5
4 < 5
4 >= 5
4 <= 5
not true
-34
```

Lists are linked-lists of pairs. A pair can be created with the `|` operator.
Attaching a head to a list extends the list.

```
1 | 2         ; creates a pair with 1 as the head and 2 as the tail
1 | [2, 3]    ; creates a new list with 1 at the head: [1, 2, 3]
```

Dicts can be accessed with the `.` operator.

```
screen.width  ; accesses `width` key
```

Lists and dicts can be accessed by calling them as functions.

```
(screen :width)   ; accesses `width` key
(items 8)         ; accesses 8th element of items
```

Logical operators (`and` and `or`) short-circuit execution.

```
false and x   ; x not evaluated
true or y     ; y not evaluated
```

The operator precedence is:

- Dictionary access `.`
- Unary `-` or `not`
- Multiplication `*`, `/`
- Addition `+`, `-`
- Pair `|`
- Comparison `>`, `<`, `>=`, `<=`
- Equality `=`, `!=`
- Logic `and`, `or`

## Functions

An expression is a combination of operators to their operands. A function call
is a function expression followed by any number of expressions as its arguments.

```
send-email-after 3600 * 12 "admin@example.com" :urgent    ; a function call with three arguments
```

A function without arguments isn't called, unless it's surrounded with parentheses.

```
send-alert    ; not called
(send-alert)  ; called
```

## Anonymous Functions

Anonymous functions, aka lambdas, are defined with an arrow.

```
(x y) -> x * y      ; creates a lambda of two arguments
```

## Definitions

Variables and functions can be defined with `def`.

```
def max-value (get-width screen) - 4      ; defines variable `max-value` to be the result of an expression
def square (x) -> x * x                   ; defines `square` as a function of one parameter
def (square x) x * x                      ; syntax sugar for the above
```

## Blocks

A block is an expression that evaluates sequence of function calls.

```
do
  halt-system
  send-alert "Attention needed"
  open-pod-bay-doors
end
```

## Conditionals

A conditional using `if`:

```
if x = 1 :ok        ; evaluates to `:ok` if true, `nil` if false

if not (system-ok) do
  halt-system
  alert-admin
else
  :proceed
end
```
