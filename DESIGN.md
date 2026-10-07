# Line Editor — Paper Design

*Written before any code was typed, per the competition rules.*

## Data structure choice

**A dynamic array of strings** (`char **lines`, with `count` and `capacity`).

```c
typedef struct {
    char  **lines;      /* heap-allocated C string per line */
    size_t  count;      /* lines currently stored */
    size_t  capacity;   /* slots allocated */
} Document;
```

**Why this, and not the alternatives:**

| Option | Verdict |
|---|---|
| Fixed-size 2D char array (`char buf[MAX][MAX]`) | Wastes memory, hard line-count limit — rejected |
| Singly linked list of lines | O(n) walk for every line-number access, and this editor is *all about* line numbers — rejected |
| **Dynamic array of `char *`** | **O(1) access by line number**; insert/delete are O(n) pointer shifts via `memmove`, which is simple and cache-friendly; capacity doubles as needed so there is no hard limit — **chosen** |

The trade-off is conscious: we pay O(n) shifts on insert/delete to keep
line-number access O(1) and the code simple enough to finish in the time limit.

## Command set (all 1-based line numbers)

| Command | Meaning |
|---|---|
| `i <n> <text>` | Insert text as line n (valid: 1 … count+1) |
| `d <n>` | Delete line n |
| `p` | Display all lines with numbers |
| `s <file>` | Save document to file |
| `l <file>` | Load file (replaces current document) |
| `f <word>` | Search: list lines containing word *(bonus)* |
| `r <n> <old> <new>` | Replace first `<old>` with `<new>` on line n *(bonus)* |
| `c` | Line / word / character counts *(bonus)* |
| `h` | Help |
| `q` | Quit |

## Function outline

- `doc_init / doc_free / doc_clear` — lifecycle management
- `doc_grow` — double capacity when full
- `doc_insert(doc, n, text)` — validate range, grow if needed, `memmove` pointers down, store a heap copy
- `doc_delete(doc, n)` — validate range, free the line, `memmove` pointers up
- `doc_display` — print `"%4zu  %s"` per line; `(empty document)` if none
- `doc_save / doc_load` — file I/O; load builds into a temp document first so a failed load never destroys the current one
- `doc_search` — `strstr` per line, print matches
- `doc_replace` — `strstr` for first occurrence, rebuild the line with `memcpy`/`strcpy`/`strcat`
- `doc_stats` — counts lines; words via whitespace-transition scan; chars via `strlen`
- `read_line` — `fgets`-based reader that strips `\n` and discards overlong remainders so the input stream never desynchronises
- `parse_number / parse_arg` — `strtoul`-based argument parsing (never trusts `scanf`)
- `main` — REPL loop: prompt, read, dispatch, print errors without crashing

## Robustness rules (decided up front)

1. Every line number is range-checked before use — invalid numbers print an error, never crash.
2. Empty document is a legal state: `p` prints `(empty document)`, `d`/`f`/`c` behave sanely.
3. Missing command arguments print a `usage:` hint.
4. Unknown commands print a hint pointing at `h`.
5. Load failures leave the current document untouched.
6. All heap memory is freed on quit (verified with sanitizers).
