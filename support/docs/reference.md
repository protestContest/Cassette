# Function Reference

## [Kernel](#kernel)

These functions are always available in the global scope.

### head(_pair_)

Returns the head of a pair.

```cassette
head([1, 2, 3])   ; => 1
head([1])         ; => 1
head([])          ; => nil
head(3 | 4)       ; => 3
head(nil)         ; => nil
```

### tail(_pair_)

Returns the tail of a pair.

```cassette
tail([1, 2, 3])   ; => [2, 3]
tail([1])         ; => nil
tail([])          ; => nil
tail(3 | 4)       ; => 4
tail(nil)         ; => nil
```

### panic!(_message_)

Triggers a runtime error with the given message. Runtime errors are not recoverable.

```cassette
panic!("Oops!")    ; Runtime Error: Oops!
```

### float?(_value_)
### integer?(_value_)
### symbol?(_value_)
### pair?(_value_)
### tuple?(_value_)
### binary?(_value_)
### map?(_value_)
### function?(_value_)

Returns whether a value is a given type. Note that lists and `nil` are pairs, and that `true` and `false` are symbols.

```cassette
float?(1.0)       ; => true
integer(3)        ; => true
symbol(:foo)      ; => true
symbol(true)      ; => true
symbol(false)     ; => true
pair([1, 2])      ; => true
pair(3 | 4)       ; => true
pair(nil)         ; => true
tuple({3, 4})     ; => true
tuple({})         ; => true
binary("x")       ; => true
map({x: 1})       ; => true
```



## [Type](#type)

The `Type` module allows some primitive operations on data types not otherwise possible, such as setting a map key or splitting a binary.

### map-get(_map_, _key_)

Accesses a map with a key of any value.

```cassette
map-get(map {x: 1, y: 2})
map-get(map 3)

; these are equivalent:
map-get(map, :foo)
map.foo
```

### map-set(_map_, _key_, _value_)

Updates a map key of any value with a value. Returns a new map.

```cassette
map-set(map, {x: 1, y: 2}, true)
map-set(map, 3, some_value)
```

### map-keys(_map_)

Returns a tuple of a map's keys. The order of keys is arbitrary.

```cassette
map-keys({foo: 1, bar: 3})   ; => {:foo, :bar}
```

### split-bin(_binary_, _index_)

Splits a binary at the given index into a pair of binaries.

```cassette
split-bin("foobar", 3)   ; => "foo" | "bar"
split-bin("foo", 100)    ; => "foo" | ""
split-bin("foo", -30)    ; => "" | "foo"
```

### join-bin()

Joins an IOData value into a binary. An IOData is either a binary, an integer between 0-255 (representing a single byte), or a list of IOData values. This makes it easy to build binaries without making a bunch of intermediate copies, by joining them up in a list and calling `join-bin` at the end.

```cassette
join-bin(["Hello", [0x20, "World!"]])  ; => "Hello World!"
```

### trunc(_num_)

Truncates a number to an integer. Useful for converting floats to integers.

```cassette
trunc(3.14)        ; => 3
trunc(1.0)         ; => 1
trunc(4)           ; => 4
trunc(-1.5)        ; => -1
```

### symbol-name(_symbol_)

Returns a binary representation of a symbol.

```cassette
symbol-name(:foo)  ; => "foo"
symbol-name(true)  ; => "true"
```



## [Device](#device)

The `Device` module handles interfacing with external devices and files. Each device has a type, and each device type has a driver that implements these basic device operations: `open`, `close`, `read`, `write`, `get-param`, and `set-param`. If an operation doesn't apply to a device type, the result is `nil`.

The currently-implemented device types are:

- `:console`: Standard input/output
- `:directory`: Filesystem access
- `:file`: Normal file
- `:system`: System information and special attributes
- `:window`: A window with a drawing context

### open(_type_, _opts_)

Opens a device of the given type (a symbol), with a tuple of options. Returns a result with a device reference.

The options tuple depends on the type of device:

- `:console`: Options are ignored
- `:directory`
  - `path`: The path of the directory (a string)
- `:file`
  - `path`: The path of the file (a string)
  - `mode`. A file access mode string: one of "r" (read), "w" (overwrite), "a" (append), "r+", "w+", or "a+"
- `:system`: Options are ignored
- `:window`
  - `title` (optional): The window title (a string)
  - `width`: The window's width
  - `height`: The window's height

```cassette
Device.open(:console, {})                   ; => :ok | 0
Device.open(:directory, {"../test"})        ; => :ok | 1
Device.open(:file, {"foo.txt", "rw"})       ; => :ok | 2
Device.open(:system, {})                    ; => :ok | 3
Device.open(:window, {200, 300})            ; => :ok | 4
Device.open(:window, {"Canvas", 200, 300})  ; => :ok | 5
```

### close(_device_)

Closes a device previously opened with `open`. Returns `:ok` or an error result.

```cassette
Device.close(device)         ; => :ok
```

### read(_device_, _length_)

Reads data from a device previously opened with `open`. `length` may be any value, whose meaning depends on the device type — but it generally indicates some limit of data to read. Returns a result with the data value, depending on the device type.

- `:console`: Reads `length` bytes from standard input, returning a binary
- `:directory`: Returns a list of directory entries, each entry being a tuple of `name` and `type`. An entry's type is one of these:
  - `:device`: some block or character device
  - `:directory`: a sub-directory
  - `:pipe`: a Unix pipe
  - `:link`: a filesystem link
  - `:file`: a normal file
  - `:socket`: a Unix socket
- `:file`: Reads `length` bytes from a file, returning a binary. Returns `nil` when the end of the file is reached.
- `:system`: Can't be read; returns `nil`
- `:window`: Returns `length` number of input events recevied in the window. Not currently implemented.

```cassette
Device.read(dir, nil)         ; => [{"test.txt", :normal}, {"src", :directory}]
Device.read(my-file, 8)       ; => "hello, w"
Device.read(window, 1)        ; error: not implemented
```

### write(_device_, _data_)

Writes data to a device previously opened with `open`. `data` may be any value, whose meaning depends on the device type. Returns a result.

For many device types, `data` can be in the form of _iodata_. An iodata is either a binary, an integer from 0–255 (representing a byte), or a list of iodata items.

- `:console`: Writes iodata to standard output.
- `:directory`: Given a tuple with a filename and type, creates the file. If the type is `:delete`, the file is deleted.
- `:file`: Writes iodata to a file.
- `:system`: Can't be written; returns `nil`
- `:window`: Given a drawing command, draws into the window. A drawing command is a tuple tagged with the command type, and has one of these formats:
  - `{:clear, color}`: Clears the canvas with the given color. A color must be a tuple of red, green, and blue values (0–255).
  - `{:text text x y}`: Draws a string in the canvas at the given coordinates.
  - `{:line x1 y1 x2 y2}`: Draws a line between the given coordinates.
  - `{:arc x1 y1 x2 y2 x3 y3}`: Draws a circular arc between (x1, y1) and (x3, y3), through the point (x2, y2).
  - `{:blit img x y width height}`: Copies a binary chunk of pixels with the given width/height to the screen. The image binary stores pixels as 32-bit RGBA values.

```cassette
Device.write(console, "Hello!")             ; prints "Hello!" (without a newline)
Device.write(dir, {"newfile.txt", :file})   ; creates a file
Device.write(dir, {"newfile.txt", :directory})   ; creates a folder
Device.write(file, "foo")                   ; writes "foo" into a file
Device.write(win, {:line, 10, 20, 100, 200})    ; draws a line
```

### set-param(_device_, _key_, _value_)

Sets a parameter for a device previously opened with `open`. If `key` is not a valid parameter for the device, `nil` is returned, otherwise a result.

- `:console`: No parameters to set.
- `:directory`: No parameters to set.
- `:file`
  - `:position`: seeks to the given position in the file (an integer)
- `:system`
  - `:seed`: Seeds the system random number generator
- `:window`
  - `:font`: Given a path to a TTF file, sets the font the window will use for future `text` commands. `value` can also be a tuple of `{filename, size}` to set the font and size simultaneously.
  - `:font-size`: Sets the font size the window will use for future `text` commands.
  - `:color`: Sets the color the window will use for future drawing commands. `value` must be a tuple of three bytes, representing red, green, and blue.

```cassette
Device.set-param(my-file, :position, 0)
Device.set-param(sys, :seed, 100)
Device.set-param(win, :font, "Helvetica.ttf")
Device.set-param(win, :font-size, 16)
Device.set-param(win, :font, {"Helvetica.ttf", 16})
Device.set-param(win, :color, {0x00, 0x1F, 0xFF})
```

### get-param(_device_, _key_)

Gets a parameter for a device previously opened with `open`. If `key` is not a valid parameter for the device, `nil` is returned.

- `:console`: No parameters to get.
- `:directory`
  - `:path`: The directory's absolute path.
- `:file`
  - `:position`: The current position in the file.
  - `:size`: The file's size.
  - `:path`: The file's absolute path.
- `:system`
  - `:random`: A random integer.
  - `:time`: The current time.
- `:window`
  - `:width`: The window's width
  - `:height`: The window's height
  - `:font`: The filename of the window's current font.
  - `:font-size`: The font size of the window's current font.
  - `:color`: TThe color the window will use for future drawing commands. `value` must be a tuple of three bytes, representing red, green, and blue.

```cassette
Device.get-param(file, :position)       ; => 28
Device.get-param(dir, :path)            ; => "/home/users/data"
Device.get-param(sys, :random)          ; => 2642
Device.get-param(win, :font)            ; => "Helvetica.ttf"
Device.get-param(win, :font)-size       ; => 16
Device.get-param(win, :color)           ; => {0x00, 0x1F, 0xFF}
```
