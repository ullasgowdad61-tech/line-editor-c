# Line Editor — Help

A simple line-oriented text editor that runs in the terminal.
Type a command, press Enter, and the editor acts on the document.
Line numbers are **1-based** (the first line is line 1).

## Commands

### `i <n> <text>` — Insert a line
Adds `text` as line `n`, shifting existing lines down.
Valid line numbers: `1` to (current lines + 1).

```
> i 1 Hello world
> i 2 Second line
> i 2 Inserted between them
> p
   1  Hello world
   2  Inserted between them
   3  Second line
```

### `d <n>` — Delete a line
Removes line `n`, shifting the lines below it up.

```
> d 2
> p
   1  Hello world
   2  Second line
```

### `p` — Display the document
Prints every line with its line number. Shows `(empty document)` when there is nothing to display.

### `s <file>` — Save to a file
Writes the in-memory document to `file`, one line per row.

```
> s notes.txt
saved to "notes.txt"
```

### `l <file>` — Load from a file
Reads `file` into the editor, **replacing** the current document.

```
> l notes.txt
loaded "notes.txt"
```

### `f <word>` — Search *(bonus)*
Lists every line containing `word`, with line numbers.

```
> f world
   1  Hello world
1 match found
```

### `r <n> <old> <new>` — Find & replace *(bonus)*
Replaces the first occurrence of `old` with `new` on line `n`.
`<old>` and `<new>` are single words (no spaces).

```
> r 1 world everyone
replaced on line 1
```

### `c` — Counts *(bonus)*
Reports the number of lines, words, and characters in the document.

```
> c
lines: 3  words: 9  characters: 46
```

### `h` — Help
Prints a summary of all commands.

### `q` — Quit
Exits the editor. (Ctrl+D / EOF also quits.)

## Notes & limits

- A line number of `0`, a negative number, or anything past the end of the
  document is rejected with an error message — the editor never crashes on
  bad input.
- Input lines longer than 4095 characters are truncated safely.
- Filenames are taken literally as typed (no quotes needed).
