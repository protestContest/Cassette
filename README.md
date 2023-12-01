![Cassette](https://cassette-lang.com/assets/banner-wide.svg)

Cassette is a small, Lisp-like programming language. It is a [home-cooked meal](https://www.robinsloan.com/notes/home-cooked-app/). It looks like this:

```cassette
import List
import Math

let width = 800,
    height = 480,
    canvas = Canvas.new width height

Canvas.text canvas "Lines!" 200.1 0

Sys.seed (Sys.ticks)

def (rand-line i) do
  let x0 = Math.floor i * width / 100,
      y0 = Math.rand-int 20 height / 10,
      x1 = Math.rand-int 0 width,
      y1 = Math.rand-int 20 height
  Canvas.line canvas x0 y0 x1 y1
end

List.map (List.range 0 100) \i -> (rand-line i)
```

<aside>
Please direct any comments, questions, and insults [here](mailto:cassette@zjm.me).
</aside>

## Press _Play_

I made Cassette because I wanted a simple functional scripting language to play with. I wanted a language that I could just start coding away in an editor with. Something that felt like making music in your basement on a cassette recorder.

There are other, arguably better, languages that could serve this purpose. Python, Scheme, Lua, Elixir, and Javascript are all inspirations, but none quite scratched my itch. Cassette won't serve millions of requests per second or train large language models, but maybe you can use it to make a generative art piece or game.

Here are some of the design goals of Cassette:

- Functional
- Few dependencies
- Simplicity over efficiency
- DIY over standard library
- Small implementation

## Getting Started

This project requires a C build toolchain. You may need to install `make` and `clang`. The source code can be found [here](https://git.sr.ht/~zjm/Cassette).

1. Clone the repo with `git clone https://git.sr.ht/~zjm/Cassette`.
2. Run `make` to build the project. This creates the executable `cassette`.
    - To compile with canvas support, make sure SDL2 is installed and follow instructions in the Makefile.
3. Optionally, run `make install` to install the Cassette executable. You can set the install folder in the Makefile.
4. Try the examples with `./cassette -p test/project.txt`.
5. Write a little script and run it with `./cassette script.ct`.

## More Info

For more information about Cassette is available at [cassette-lang.com](https://cassette-lang.com)
