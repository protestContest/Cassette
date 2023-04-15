# Rye

> Note: This project needs a new name.

Rye is a homebrew programming language inspired by Lisp and Elixir. Here's what
it looks like:

```
def (reduce list acc fn) do
  if (nil? list) do
    acc
  else
    reduce (tail list) (fn (head list) acc) fn
  end
end

def (map list fn) do
  let mapped = reduce list nil (item acc) -> (fn item) | acc
  reverse mapped
end

map [1 2 3 4 5] (num acc) -> do
  if (even? num) do
    (num / 2)
  else
    (3 * num + 1)
  end
end

; => [4 1 10 2 16]
```

## Building

Use `make` to build this project. This project requires the `univ` library,
which can be found [here](https://git.sr.ht/~zjm/univ).
