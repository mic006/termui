# TermUi

## Purpose

`TermUi` is a simple C++ library to build a terminal application: draw on the screen, get keyboard events.

It is a **very simplistic** replacement for [ncurses](https://invisible-island.net/ncurses/) or [termbox](https://github.com/nsf/termbox).

## Features

- C++ library (most other libraries are plain C)
- support modern terminals only: does not use terminfo or termcap
- lightweight
- depends only on termios
- support of palette colors and true colors (24 bits RGB)
- capture signals and keyboard events
- single framebuffer
- high level APIs to simplify application development

## Limitations

- Linux only
- tested on Konsole only

## How to start ?

Look at `termui.h` or generate the documentation with doxygen to see the API.

Look also at the demo for a more concrete and complete example.

## Showcase / Demo

A demo is included in the `demo/` folder. It contains several screens displaying different capabilities:
- text effect, like bold, italic...
- 256 colors mode with palette
- 24bits color mode with a gradient
- keyboard events that can be captured
- extract of the API documentation

## Thanks

Many thanks to ["nsf"](https://github.com/nsf) for [termbox](https://github.com/nsf/termbox), which I have used as a sample and guide to write this library.