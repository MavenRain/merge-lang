/* The test of `langc explain-code` (S8 design sec 3). Run it from the repo
   root. It reads the code rows of docs/DIAGNOSTICS.md, runs `build/langc
   explain-code CODE` for each row and compares the output with the JSON
   line of the row (exit 0, one line, no stderr). Then it compares each row
   of the C table (src/codes.c) with the doc row of its code, cell for cell,
   and the two row counts. `explain-code NOPE` must give exit 1, no stdout
   and one stderr line; `explain-code` with no code must give exit 2. Each
   mismatch gives a `FAIL explain-code ...` line on stderr. When there is
   none, it prints `explain-code: N codes, 0 mismatches` and exits 0; else
   it exits 1. Port of evolve-lang 8c5d8e1 test/verbs.c (part 1). */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "codes.h"
#include "front/base.h"
#include "front/json.h"

#define DOC_PATH "docs/DIAGNOSTICS.md"
#define ARENA_BYTES ((size_t)1 << 24)

/* The cells: Code, Severity, Stage, Meaning and Fixture. */
#define CELL_COUNT 5

/* The lines of a file or of the output of a command, without their
   newlines. */
typedef struct {
  char **lines;
  size_t count;
} Lines;

/* A code row of the docs/DIAGNOSTICS.md table: the Code cell without its
   backticks, then the other cells, each without the spaces around it. The
   cells point into the line. */
typedef struct {
  char *cells[CELL_COUNT];
} DocRow;

static void free_lines(Lines *lines) {
  for (size_t i = 0; i < lines->count; i++) free(lines->lines[i]);
  free(lines->lines);
  lines->lines = NULL;
  lines->count = 0;
}

/* Reads the lines of FILE into LINES. Returns 0 on a read error or when
   there is no memory. */
static int read_stream(FILE *file, Lines *lines) {
  char *line = NULL;
  size_t size = 0;
  ssize_t len;
  int ok = 1;
  while (ok && (len = getline(&line, &size, file)) >= 0) {
    char **grown = realloc(lines->lines, (lines->count + 1) * sizeof *grown);
    ok = grown != NULL;
    if (ok) {
      lines->lines = grown;
      if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
      lines->lines[lines->count++] = line;
      line = NULL;
      size = 0;
    }
  }
  free(line);
  return ok && !ferror(file);
}

/* Reads the lines of the file PATH into LINES. Returns 0 when the file
   cannot be read. */
static int read_file(const char *path, Lines *lines) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) return 0;
  int ok = read_stream(file, lines);
  return fclose(file) == 0 && ok;
}

/* Runs COMMAND and reads its stdout into LINES. Returns the exit code, or -1
   when the command cannot run or its stdout cannot be read. */
static int run_command(const char *command, Lines *lines) {
  FILE *pipe = popen(command, "r");
  if (pipe == NULL) return -1;
  int ok = read_stream(pipe, lines);
  int status = pclose(pipe);
  return ok && status != -1 && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* The JSON object of CELLS on one line, by the writer of langc
   (front/json.c), in ARENA, or NULL when there is no memory. */
static const char *json_line(Arena *arena, char *const *cells) {
  static const char *const NAMES[CELL_COUNT] = {"code", "severity", "stage", "meaning", "fixture"};
  Diag diag;
  diag_init(&diag);
  Out o;
  out_init(&o, arena, &diag);
  int ok = 1;
  for (int i = 0; i < CELL_COUNT && ok; i++) {
    ok = put_text(&o, i == 0 ? "{\"" : ",\"") && put_text(&o, NAMES[i]) && put_text(&o, "\":") &&
         put_string(&o, cells[i]);
  }
  return ok && put_text(&o, "}") ? o.data : NULL;
}

/* The text of CELL without the spaces around it. */
static char *trim(char *cell) {
  size_t len = strlen(cell);
  while (len > 0 && cell[len - 1] == ' ') cell[--len] = '\0';
  while (*cell == ' ') cell++;
  return cell;
}

/* Splits LINE, a code row `| `CODE` | ... |`, in place into ROW. Returns 0
   when LINE has fewer cells or its Code cell has no backticks. */
static int split_row(char *line, DocRow *row) {
  char *at = line + 1;
  for (int i = 0; i < CELL_COUNT; i++) {
    char *bar = strchr(at, '|');
    if (bar == NULL) return 0;
    *bar = '\0';
    row->cells[i] = trim(at);
    at = bar + 1;
  }
  size_t len = strlen(row->cells[0]);
  if (len < 2 || row->cells[0][0] != '`' || row->cells[0][len - 1] != '`') return 0;
  row->cells[0][len - 1] = '\0';
  row->cells[0]++;
  return 1;
}

/* Runs `build/langc explain-code CODE` for the doc row ROW and compares its
   stdout and stderr with the JSON line of ROW. Returns the number of FAIL
   lines. */
static int check_doc_row(Arena *arena, const DocRow *row) {
  char command[256];
  Lines out = {NULL, 0};
  const char *want = json_line(arena, row->cells);
  snprintf(command, sizeof command, "build/langc explain-code '%s' 2>&1", row->cells[0]);
  int status = run_command(command, &out);
  int ok = want != NULL && status == 0 && out.count == 1 && strcmp(out.lines[0], want) == 0;
  if (!ok) fprintf(stderr, "FAIL explain-code %s: exit %d, %zu lines, not the doc row\n", row->cells[0], status, out.count);
  free_lines(&out);
  return !ok;
}

/* Compares the table row ROW cell for cell with the doc row of its code in
   the COUNT ROWS. Returns the number of FAIL lines. */
static int check_table_row(const CodeRow *row, const DocRow *rows, size_t count) {
  const char *cells[CELL_COUNT] = {row->code, row->severity, row->stage, row->meaning, row->fixture};
  const DocRow *doc = NULL;
  for (size_t i = 0; i < count && doc == NULL; i++) doc = strcmp(rows[i].cells[0], row->code) == 0 ? &rows[i] : NULL;
  if (doc == NULL) {
    fprintf(stderr, "FAIL explain-code: the table code %s has no row in %s\n", row->code, DOC_PATH);
    return 1;
  }
  int fails = 0;
  for (int c = 1; c < CELL_COUNT; c++) {
    if (strcmp(cells[c], doc->cells[c]) != 0) {
      fprintf(stderr, "FAIL explain-code %s: cell %d of the table is not the doc cell\n", row->code, c + 1);
      fails++;
    }
  }
  return fails;
}

/* An unknown code gives exit 1, no stdout and one stderr line; no code
   gives exit 2. Returns the number of FAIL lines. */
static int check_errors(void) {
  Lines out = {NULL, 0};
  Lines err = {NULL, 0};
  Lines none = {NULL, 0};
  int status = run_command("build/langc explain-code NOPE 2>/dev/null", &out);
  int status_err = run_command("build/langc explain-code NOPE 2>&1 >/dev/null", &err);
  int status_none = run_command("build/langc explain-code 2>/dev/null", &none);
  int fails = 0;
  if (status != 1 || status_err != 1 || out.count != 0 || err.count != 1) {
    fprintf(stderr, "FAIL explain-code NOPE: exit %d, %zu stdout lines, %zu stderr lines, not exit 1, 0 and 1\n",
            status, out.count, err.count);
    fails++;
  }
  if (status_none != 2) {
    fprintf(stderr, "FAIL explain-code: no code gives exit %d, not 2\n", status_none);
    fails++;
  }
  free_lines(&out);
  free_lines(&err);
  free_lines(&none);
  return fails;
}

int main(void) {
  Arena arena;
  arena_init(&arena, ARENA_BYTES);
  Lines doc = {NULL, 0};
  int fails = read_file(DOC_PATH, &doc) ? 0 : 1;
  DocRow *rows = calloc(doc.count + 1, sizeof *rows);
  size_t count = 0;
  if (fails > 0 || rows == NULL) fprintf(stderr, "FAIL explain-code: cannot read %s\n", DOC_PATH);
  for (size_t i = 0; rows != NULL && i < doc.count; i++) {
    if (strncmp(doc.lines[i], "| `", 3) != 0) continue;
    if (split_row(doc.lines[i], &rows[count])) {
      count++;
    } else {
      fprintf(stderr, "FAIL explain-code: line %zu of %s is not a row of 5 cells\n", i + 1, DOC_PATH);
      fails++;
    }
  }
  /* Doc to table: the output of `langc explain-code` for each doc row. */
  for (size_t i = 0; i < count; i++) fails += check_doc_row(&arena, &rows[i]);
  /* Table to doc: each row of the C table has a doc row with equal cells. */
  for (size_t i = 0; rows != NULL && i < code_count(); i++) fails += check_table_row(code_at(i), rows, count);
  if (code_count() != count) {
    fprintf(stderr, "FAIL explain-code: the table has %zu rows, %s has %zu\n", code_count(), DOC_PATH, count);
    fails++;
  }
  fails += check_errors();
  if (fails == 0) printf("explain-code: %zu codes, 0 mismatches\n", count);
  free(rows);
  free_lines(&doc);
  arena_release(&arena);
  return fails == 0 ? 0 : 1;
}
