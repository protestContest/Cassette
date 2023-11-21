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
- To compile with canvas support, make sure SDL2 is installed and follow instructions in the Makefile.
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

## [Primitive Functions](#primitive-functions)

Cassette includes the following primitive functions.

### typeof _value_

Returns a symbol describing the type of `value`: one of `:float`, `:integer`,
`:symbol`, `:pair`, `:tuple`, or `:binary`;

Note that lists and `nil` are considered as pairs, and that `true` and `false`
are considered symbols.

### head _pair_

Returns the head of a pair.

### tail _pair_

Returns the tail of a pair.

### print _value_

Prints a number, symbol, or binary.

### inspect _value_

Prints a description of a value.

### open _filename_

Returns a reference to a file that can be read or written to. The file is
created if it doesn't exist. Returns `:error` if the file can't be opened.

### read _file_ref_

Reads from a file reference (obtained from `open`) until the end of the file,
and returns the contents as a binary. Returns `:error` if the file can't be
read.

### read _file_ref_ _length_

Same as `read` above, but only reads `length` bytes.

### read _filename_

Reads the entire contents of a file given by the binary `filename` and returns
the contents as a binary. Returns `:error` if the file doesn't exist or can't
be read.

### write _file_ref_ _binary_

Writes a binary to a file reference (obtained from `open`). Returns `:error` if
the file can't be written.

### ticks

Returns the approximate time since the beginning of execution.

### seed _seed_val_

Seeds the random number generator with an integer.

### random

Returns a random number between 0 and 1.

### random _max_

Returns a random integer between 0 and an integer `max`.

### random _min_ _max_

Returns a random integer between integers `min` and `max`.

### sqrt _num_

Returns the square root of a number `num`.

### new_canvas _width_ _height_

When canvas support is enabled, creates a canvas object with width `width` and
height `height` (integers). The canvas is drawn on the screen in a new window.

If canvas support isn't enabled, returns `:error`.

### set_font _filename_ _point_size_ _canvas_

Sets a canvas's font given by binary `filename` and integer `point_size`.
Returns `:ok` on success, or `:error` if the font couldn't be loaded. This
mutates `canvas` to have the new font property.

The font given by `filename` is either an absolute path, or a path relative to
the default font location (specified at compile time).

### draw_line _x1_ _y1_ _x2_ _y2_ _canvas_

When canvas support is enabled, draws a line from (`x1`, `y1`) to (`x2`, `y2`)
on `canvas` (a canvas reference obtained from `new_canvas`). Returns `:ok`.

If canvas support isn't enabled, returns `:error`.

### draw_text _text_ _x_ _y_ _canvas_

When canvas support is enabled, draws a UTF-8 string from the binary `text` on
`canvas` (a canvas reference obtained from `new_canvas`), at position
(`x`, `y`). `y` is the baseline of the rendered text. Returns `:ok`.

If canvas support isn't enabled, returns `:error`.
