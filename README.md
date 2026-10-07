# Line Editor in C

A simple command-line line editor: create, view, and modify a text document
through typed commands, with the document held in memory. Built for the
Portfolio Building (B25CS0311) 3rd-semester coding competition.

## Team

- **Ullas Gowda D** — design, implementation, testing, documentation

## Features implemented

**Core (all four):**
- Insert a line at a given line number (`i`)
- Delete a line (`d`)
- Display the document with line numbers (`p`)
- Save to / load from a file (`s`, `l`)

**Bonus (three):**
- Search for a word across lines (`f`)
- Find & replace on a line (`r`)
- Line / word / character counts (`c`)

Plus `h` (help) and `q` (quit). Invalid input (bad line numbers, missing
arguments, unknown commands, empty document) is handled gracefully — the
editor prints an error message and keeps running.

## How to compile

```sh
gcc -Wall -Wextra -std=c11 -o line_editor line_editor.c
```

Compiles with zero warnings under `-Wall -Wextra`. Also verified clean under
`-fsanitize=address,undefined` (no memory errors or leaks).

## How to run

```sh
./line_editor
```

Sample session:

```
> i 1 Hello world
> i 2 Second line
> p
   1  Hello world
   2  Second line
> s notes.txt
saved to "notes.txt"
> q
```

See [HELP.md](HELP.md) for the full command reference and [DESIGN.md](DESIGN.md)
for the data-structure design.
