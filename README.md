Cassette is a small functional programming language. Please visit <https://cassette-lang.com> for information about the language.

## Getting Started

To use Cassette, you must build it from source.

1. Get the project's dependencies
    - On macOS with Homebrew, run `brew install llvm git`
    - On Debian, run `apt install build-essential clang git libx11-dev`
2. Build Cassette
    - Clone Cassette with `git clone https://github.com/protestContest/Cassette` (and then `cd Cassette`)
    - Run `make` to build the project. This creates the executable `bin/cassette`.
3. Try the example with `./bin/cassette test/test.ct`.

## Feedback

If you're checking out Cassette, please [let me know](mailto:cassette@zjm.me) what you think! I'm happy to help with issues and hear any polite feedback.
