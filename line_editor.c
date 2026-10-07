/*
 * line_editor.c — a simple line-oriented text editor.
 *
 * The document is held in memory as a dynamic array of strings
 * (char **lines, with count and capacity). Line numbers shown to the
 * user are 1-based; internally they map to array indices (n - 1).
 *
 * Commands (typed at the prompt):
 *   i <n> <text>      insert text as line n (1..count+1)
 *   d <n>             delete line n
 *   p                 display the document with line numbers
 *   s <file>          save the document to file
 *   l <file>          load a document from file (replaces current)
 *   f <word>          search: list lines containing word      [bonus]
 *   r <n> <old> <new> replace first old with new on line n    [bonus]
 *   c                 line / word / character counts          [bonus]
 *   h                 show this help
 *   q                 quit
 *
 * Author: Ullas Gowda D — Portfolio Building, 3rd Semester CSE
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define MAX_INPUT 4096          /* longest accepted input line incl. '\n' */

/* ---------------- Document ---------------- */

typedef struct {
    char  **lines;      /* dynamic array of heap-allocated C strings */
    size_t  count;      /* lines currently stored */
    size_t  capacity;   /* slots allocated */
} Document;

static void doc_init(Document *doc)
{
    doc->count    = 0;
    doc->capacity = INITIAL_CAPACITY;
    doc->lines    = malloc(doc->capacity * sizeof *doc->lines);
    if (!doc->lines) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(EXIT_FAILURE);
    }
}

static void doc_free(Document *doc)
{
    for (size_t i = 0; i < doc->count; i++)
        free(doc->lines[i]);
    free(doc->lines);
    doc->lines = NULL;
    doc->count = doc->capacity = 0;
}

/* Remove all lines but keep the document usable. */
static void doc_clear(Document *doc)
{
    for (size_t i = 0; i < doc->count; i++)
        free(doc->lines[i]);
    doc->count = 0;
}

static int doc_grow(Document *doc)
{
    size_t new_cap = doc->capacity * 2;
    char **p = realloc(doc->lines, new_cap * sizeof *p);
    if (!p)
        return -1;
    doc->lines    = p;
    doc->capacity = new_cap;
    return 0;
}

/* Insert text as line n (1-based). Valid: 1 .. count+1. Returns 0 on success. */
static int doc_insert(Document *doc, size_t n, const char *text)
{
    if (n < 1 || n > doc->count + 1)
        return -1;
    if (doc->count == doc->capacity && doc_grow(doc) != 0)
        return -2;

    char *copy = malloc(strlen(text) + 1);
    if (!copy)
        return -2;
    strcpy(copy, text);

    size_t idx = n - 1;
    memmove(&doc->lines[idx + 1], &doc->lines[idx],
            (doc->count - idx) * sizeof *doc->lines);
    doc->lines[idx] = copy;
    doc->count++;
    return 0;
}

/* Delete line n (1-based). Returns 0 on success. */
static int doc_delete(Document *doc, size_t n)
{
    if (n < 1 || n > doc->count)
        return -1;
    size_t idx = n - 1;
    free(doc->lines[idx]);
    memmove(&doc->lines[idx], &doc->lines[idx + 1],
            (doc->count - idx - 1) * sizeof *doc->lines);
    doc->count--;
    return 0;
}

static void doc_display(const Document *doc)
{
    if (doc->count == 0) {
        printf("(empty document)\n");
        return;
    }
    for (size_t i = 0; i < doc->count; i++)
        printf("%4zu  %s\n", i + 1, doc->lines[i]);
}

static int doc_save(const Document *doc, const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        return -1;
    for (size_t i = 0; i < doc->count; i++)
        fprintf(fp, "%s\n", doc->lines[i]);
    fclose(fp);
    return 0;
}

/*
 * Read one full line from fp into buf (size bufsz), stripping the trailing
 * newline. Overlong lines are truncated and the remainder discarded so the
 * stream stays in sync. Returns 1 if a line was read, 0 on EOF, -1 on error.
 */
static int read_line(FILE *fp, char *buf, size_t bufsz)
{
    if (!fgets(buf, (int)bufsz, fp))
        return feof(fp) ? 0 : -1;

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else if (!feof(fp)) {
        int ch;                     /* line was longer than buf: discard rest */
        while ((ch = fgetc(fp)) != '\n' && ch != EOF)
            ;
    }
    return 1;
}

/* Load file into doc, replacing current contents. Returns 0 on success. */
static int doc_load(Document *doc, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    Document tmp;
    doc_init(&tmp);

    char buf[MAX_INPUT];
    int rc, status = 0;
    while ((rc = read_line(fp, buf, sizeof buf)) == 1) {
        if (doc_insert(&tmp, tmp.count + 1, buf) != 0) {
            status = -2;            /* out of memory */
            break;
        }
    }
    fclose(fp);
    if (rc < 0)
        status = -1;

    if (status == 0) {
        doc_clear(doc);
        /* move tmp's storage into doc */
        free(doc->lines);
        doc->lines    = tmp.lines;
        doc->count    = tmp.count;
        doc->capacity = tmp.capacity;
    } else {
        doc_free(&tmp);
    }
    return status;
}

/* Print every line containing word. Returns number of matches. */
static size_t doc_search(const Document *doc, const char *word)
{
    size_t hits = 0;
    for (size_t i = 0; i < doc->count; i++) {
        if (strstr(doc->lines[i], word)) {
            printf("%4zu  %s\n", i + 1, doc->lines[i]);
            hits++;
        }
    }
    return hits;
}

/*
 * Replace the first occurrence of old with new on line n (1-based).
 * Returns 1 if replaced, 0 if old not found, -1 on bad line number,
 * -2 on allocation failure.
 */
static int doc_replace(Document *doc, size_t n, const char *old, const char *new_)
{
    if (n < 1 || n > doc->count)
        return -1;
    char *line = doc->lines[n - 1];
    char *at = strstr(line, old);
    if (!at)
        return 0;

    size_t new_len = strlen(line) - strlen(old) + strlen(new_);
    char *rep = malloc(new_len + 1);
    if (!rep)
        return -2;

    size_t head = (size_t)(at - line);
    memcpy(rep, line, head);
    strcpy(rep + head, new_);
    strcat(rep, at + strlen(old));

    free(line);
    doc->lines[n - 1] = rep;
    return 1;
}

static size_t count_words(const char *s)
{
    size_t words = 0;
    int in_word = 0;
    for (; *s; s++) {
        if (isspace((unsigned char)*s)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    return words;
}

static void doc_stats(const Document *doc)
{
    size_t words = 0, chars = 0;
    for (size_t i = 0; i < doc->count; i++) {
        words += count_words(doc->lines[i]);
        chars += strlen(doc->lines[i]);
    }
    printf("lines: %zu  words: %zu  characters: %zu\n",
           doc->count, words, chars);
}

/* ---------------- Command parsing ---------------- */

/* Skip blanks; parse an unsigned line number. Returns 1 on success. */
static int parse_number(char *p, size_t *out, char **rest)
{
    while (*p == ' ' || *p == '\t')
        p++;
    char *end;
    unsigned long v = strtoul(p, &end, 10);
    if (end == p)
        return 0;
    *out  = (size_t)v;
    *rest = end;
    return 1;
}

/* Skip blanks and return pointer to the rest of the argument (may be ""). */
static char *parse_arg(char *p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

static void print_help(void)
{
    printf("Commands:\n"
           "  i <n> <text>       insert text as line n (1..lines+1)\n"
           "  d <n>              delete line n\n"
           "  p                  display the document with line numbers\n"
           "  s <file>           save the document to file\n"
           "  l <file>           load a document from file (replaces current)\n"
           "  f <word>           search: show lines containing word\n"
           "  r <n> <old> <new>  replace first <old> with <new> on line n\n"
           "  c                  show line / word / character counts\n"
           "  h                  show this help\n"
           "  q                  quit\n");
}

int main(void)
{
    Document doc;
    doc_init(&doc);

    char input[MAX_INPUT];
    printf("Simple Line Editor — type 'h' for help, 'q' to quit.\n");

    for (;;) {
        printf("> ");
        fflush(stdout);

        int rc = read_line(stdin, input, sizeof input);
        if (rc == 0) {              /* EOF (Ctrl+D) */
            putchar('\n');
            break;
        }
        if (rc < 0) {
            fprintf(stderr, "error: failed to read input\n");
            break;
        }

        char *p = parse_arg(input);
        if (*p == '\0')
            continue;

        char cmd = *p;
        p = parse_arg(p + 1);

        if (cmd == 'q') {
            break;
        } else if (cmd == 'h') {
            print_help();
        } else if (cmd == 'p') {
            doc_display(&doc);
        } else if (cmd == 'c') {
            doc_stats(&doc);
        } else if (cmd == 'i') {
            size_t n;
            char *rest;
            if (!parse_number(p, &n, &rest)) {
                printf("usage: i <line> <text>\n");
                continue;
            }
            char *text = parse_arg(rest);
            if (*text == '\0') {
                printf("usage: i <line> <text>  (no text given)\n");
                continue;
            }
            int r = doc_insert(&doc, n, text);
            if (r == -1)
                printf("error: line number out of range (1..%zu)\n", doc.count + 1);
            else if (r == -2)
                printf("error: out of memory\n");
        } else if (cmd == 'd') {
            size_t n;
            char *rest;
            if (!parse_number(p, &n, &rest)) {
                printf("usage: d <line>\n");
                continue;
            }
            if (doc_delete(&doc, n) != 0)
                printf("error: line number out of range (1..%zu)\n", doc.count);
        } else if (cmd == 's' || cmd == 'l') {
            char *path = parse_arg(p);
            if (*path == '\0') {
                printf("usage: %c <file>\n", cmd);
                continue;
            }
            int r = (cmd == 's') ? doc_save(&doc, path) : doc_load(&doc, path);
            if (r == 0)
                printf("%s \"%s\"\n", cmd == 's' ? "saved to" : "loaded", path);
            else if (r == -1)
                printf("error: cannot open \"%s\"\n", path);
            else
                printf("error: out of memory\n");
        } else if (cmd == 'f') {
            char *word = parse_arg(p);
            if (*word == '\0') {
                printf("usage: f <word>\n");
                continue;
            }
            size_t hits = doc_search(&doc, word);
            printf("%zu match%s found\n", hits, hits == 1 ? "" : "es");
        } else if (cmd == 'r') {
            size_t n;
            char *rest;
            if (!parse_number(p, &n, &rest)) {
                printf("usage: r <line> <old> <new>\n");
                continue;
            }
            char old[256], new_[256];
            if (sscanf(rest, "%255s %255s", old, new_) != 2) {
                printf("usage: r <line> <old> <new>\n");
                continue;
            }
            int r2 = doc_replace(&doc, n, old, new_);
            if (r2 == 1)
                printf("replaced on line %zu\n", n);
            else if (r2 == 0)
                printf("no match for \"%s\" on line %zu\n", old, n);
            else if (r2 == -1)
                printf("error: line number out of range (1..%zu)\n", doc.count);
            else
                printf("error: out of memory\n");
        } else {
            printf("unknown command '%c' — type 'h' for help\n", cmd);
        }
    }

    doc_free(&doc);
    return 0;
}
