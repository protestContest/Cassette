# Documentation

## Operators

- `map . key`: Accesses a map by the symbol `key`
- `a * b`, `a / b`: Multiplication and division (arithmetic)
- `a + b`, `a - b`: Addition and subtraction (arithmetic)
- `v in c`: Whether `v` is in collection `c` (boolean)
- `a > b`, `a < b`, `a >= b`, `a <= b`: Comparison (boolean)
- `a == b`, `a != b`: Equality (boolean)
- `a and b`, `a or b`: Boolean logic

## Primitive Functions

Functions beginning with `@` are primitive functions. They are usually defined in the global environment without the `@` prefix (but those can be redefined). By convention, functions ending with `!` mutate some data.

## Types

- `type-of v`: Returns a symbol denoting the type of v: `:integer`, `:float`, `:symbol`, `:pair`, `:tuple`, `:map`, or `:binary`
- `nil? v`: Returns whether a value is nil
- `integer? v`: Returns whether a value is an integer
- `float? v`: Returns whether a value is a float
- `numeric? v`: Returns whether a value is an integer or float
- `symbol? v`: Returns whether a value is a symbol
- `true? v`: Returns whether a value is neither `false` nor `nil`
- `pair? v`: Returns whether a value is a pair
- `tuple? v`: Returns whether a value is a tuple
- `map? v`: Returns whether a value is a map
- `binary? v`: Returns whether a value is a binary

## Math

- `ceil n`: Returns the smallest integer greater than `n`
- `floor n`: Returns the largest integer smaller than `n`
- `round n`: Returns the closest integer to `n`
- `div a b`: Returns the truncated division of `a / b`
- `rem a b`: Returns the remainder of the truncated division of `a / b`
- `abs n`: Returns the absolute value of `n`
- `min a b`: Returns the lesser of `a` and `b`
- `max a b`: Returns the greater of `a` and `b`
- `random`: Returns a random number in the range [0, 1)
- `random-int min max`: Returns a random integer in the range [`min`, `max`)

## Pairs & Lists

- `pair head tail`: Creates a pair with a head and tail
- `head pair`: Returns the head of a pair (i.e. the first element in a list)
- `tail pair`: Returns the tail of a pair (i.e. the rest of a list after the head)
- `set-head! pair value`: Sets a pair's head
- `set-tail! pair value`: Sets a pair's tail

## Tuples

- `tuple size`: Creates an empty tuple of a given size
- `tuple-size tuple`: Returns the number of elements in a tuple
- `get-nth tuple n`: Returns the nth element of a tuple. Error if out of bounds.
- `set-nth! tuple n value`: Sets the nth element in a tuple. Error if out of bounds.

## Maps

- `map size`: Makes an empty map of a given size
- `map-size map`: Returns the number of entries in a map
- `map-get map key`: Returns the value in a map for a given key, or nil
- `map-set! map key value`: Sets the value in a map for a given key. Error if key is missing and map is full.

## Binaries

- `allocate size`: Makes an empty binary of a given size
- `byte-size binary`: Returns the number of bytes in a binary
- `get-byte binary n`: Returns the nth byte in a binary. Error if out of bounds.
- `set-byte! binary n byte`: Sets the nth byte in a binary. Error if out of bounds or if `byte` isn't an integer between 0 and 255.
