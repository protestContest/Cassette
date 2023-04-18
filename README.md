# Rye

> Note: This project needs a new name.

Rye is a homebrew programming language inspired by Lisp and Elixir. Here's what
it looks like:

```
def (reverse list) do
  def (reverse-onto list rest) do
    if list == nil do
      rest
    else
      reverse-onto (tail list) [(head list) | rest]
    end
  end

  reverse-onto list nil
end

def (reduce list acc fn) do
  if list == nil do
    acc
  else
    reduce (tail list) (fn (head list) acc) fn
  end
end

def (map list fn) do
  let mapped = reduce list [] (item acc) -> [(fn item) | acc]
  reverse mapped
end

def (even? n) do (rem n 2) == 0 end

map [1 2 3 4 5] (num) -> do
  if (even? num) do
    num / 2
  else
    3 * num + 1
  end
end

; => [4 1 10 2 16]
```

See [grammar.txt](https://git.sr.ht/~zjm/Rye/tree/master/item/grammar.txt) for
details on the syntax.

## Building

Use `make` to build this project. This project uses the `univ` library, which is
included for convenience, and can be found [here](https://git.sr.ht/~zjm/univ).
