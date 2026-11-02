#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/* The test of the README table and of AGENTS.md (Q86 (a)). Run it from the
   repo root: `build/readme-test [README [AGENTS]]` (README.md and AGENTS.md
   when not given). Each row of the table under `## When to use merge-lang`
   has 3 cells. A yes row has `yes` or `yes: TEXT` in its second cell and
   names one or more parts in its Parts cell; a no row has `no` or
   `no: TEXT` and names no part. Each named part is an admitted part of
   docs/PLAN.md 3.2, and each admitted part is named at least once. Each
   `langc VERB` in AGENTS has VERB in the usage text of `build/langc`, and
   AGENTS names each verb of that usage text. It prints a FAIL line for each
   mismatch, then `readme: R rows, V verbs`, and exits 0 only when there is
   no FAIL line. */

#define TABLE_HEADING "## When to use merge-lang"
#define VERB_PREFIX "langc "
#define VERB_CHARS "abcdefghijklmnopqrstuvwxyz-"
#define CELL_COUNT 3
#define MAX_CELLS 4
#define PART_COUNT 4
#define MAX_VERBS 32

/* The admitted parts of docs/PLAN.md 3.2. */
static char *const PARTS[PART_COUNT] = {"GSet", "MVReg", "Counter", "Escrow"};

/* The lines of a file or of the stdout of a command, without their
   newlines. */
typedef struct {
  char **lines;
  size_t count;
} Lines;

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

/* Returns the index of the name TEXT (LEN bytes) in the first COUNT NAMES,
   or COUNT when it is not there. */
static size_t find_name(char *const *names, size_t count, const char *text, size_t len) {
  size_t i = 0;
  while (i < count && !(strlen(names[i]) == len && strncmp(names[i], text, len) == 0)) i++;
  return i;
}

/* Cuts each line of LINES (the usage text of `build/langc`) that has a verb
   after `langc ` to that verb in place, and moves these lines to the front.
   Returns the verb count. */
static size_t usage_verbs(Lines *lines) {
  size_t count = 0;
  for (size_t i = 0; i < lines->count; i++) {
    char *at = strstr(lines->lines[i], VERB_PREFIX);
    size_t len = at == NULL ? 0 : strspn(at + strlen(VERB_PREFIX), VERB_CHARS);
    if (len == 0) continue;
    memmove(lines->lines[i], at + strlen(VERB_PREFIX), len);
    lines->lines[i][len] = '\0';
    char *front = lines->lines[count];
    lines->lines[count++] = lines->lines[i];
    lines->lines[i] = front;
  }
  return count;
}

/* Cuts the spaces at the two ends of CELL. */
static char *trim(char *cell) {
  char *start = cell + strspn(cell, " ");
  size_t len = strlen(start);
  while (len > 0 && start[len - 1] == ' ') start[--len] = '\0';
  return start;
}

/* Splits the table row LINE in place into its cells (the text between two
   `|`, trimmed). Returns the cell count, at most MAX_CELLS. */
static size_t split_cells(char *line, char **cells) {
  size_t count = 0;
  char *at = strchr(line, '|');
  char *end = at == NULL ? NULL : strchr(at + 1, '|');
  while (end != NULL && count < MAX_CELLS) {
    *end = '\0';
    cells[count++] = trim(at + 1);
    at = end;
    end = strchr(at + 1, '|');
  }
  return count;
}

/* Returns 1 when CELL is WORD, or WORD then `: ` and more text. */
static int answer_is(const char *cell, const char *word) {
  size_t len = strlen(word);
  if (strncmp(cell, word, len) != 0) return 0;
  return cell[len] == '\0' || (cell[len] == ':' && cell[len + 1] == ' ' && cell[len + 2] != '\0');
}

/* Checks the Parts cell CELL of row N: `NAME` items with a comma between
   two items. Each NAME must be one of PARTS; the part gets a mark in NAMED.
   Returns the FAIL count, and the item count in *ITEMS. */
static int check_parts_cell(const char *cell, size_t n, int *named, size_t *items) {
  int fails = 0;
  const char *at = cell;
  *items = 0;
  while (*at != '\0') {
    const char *end = *at == '`' ? strchr(at + 1, '`') : NULL;
    if (end == NULL) {
      printf("FAIL: readme: row %zu: the Parts cell is not a list of `NAME` items: %s\n", n, cell);
      return fails + 1;
    }
    size_t len = (size_t)(end - at) - 1;
    size_t i = find_name(PARTS, PART_COUNT, at + 1, len);
    if (i < PART_COUNT) {
      named[i] = 1;
    } else {
      printf("FAIL: readme: row %zu: %.*s is not an admitted part of docs/PLAN.md 3.2\n", n, (int)len, at + 1);
      fails++;
    }
    (*items)++;
    at = end + 1;
    at += *at == ',' ? 1 + strspn(at + 1, " ") : 0;
  }
  return fails;
}

/* Checks row N of the table (the line LINE). Returns the FAIL count. */
static int check_row(char *line, size_t n, int *named) {
  char *cells[MAX_CELLS];
  if (split_cells(line, cells) != CELL_COUNT) {
    printf("FAIL: readme: row %zu does not have %d cells\n", n, CELL_COUNT);
    return 1;
  }
  if (answer_is(cells[1], "no")) {
    if (cells[2][0] == '\0') return 0;
    printf("FAIL: readme: row %zu: a no row names parts: %s\n", n, cells[2]);
    return 1;
  }
  int fails = answer_is(cells[1], "yes") ? 0 : 1;
  if (fails > 0) printf("FAIL: readme: row %zu: %s is not a yes row or a no row\n", n, cells[1]);
  size_t items = 0;
  fails += check_parts_cell(cells[2], n, named, &items);
  if (items > 0) return fails;
  printf("FAIL: readme: row %zu: the yes row names no part\n", n);
  return fails + 1;
}

/* Checks the table of README. Returns the FAIL count, and the row count in
   *ROWS. */
static int check_table(const Lines *readme, size_t *rows) {
  int named[PART_COUNT] = {0};
  size_t at = 0;
  while (at < readme->count && strcmp(readme->lines[at], TABLE_HEADING) != 0) at++;
  while (at < readme->count && readme->lines[at][0] != '|') at++;
  *rows = 0;
  if (at + 1 >= readme->count || strncmp(readme->lines[at + 1], "|---", 4) != 0) {
    printf("FAIL: readme: no table under %s\n", TABLE_HEADING);
    return 1;
  }
  int fails = 0;
  at += 2;
  while (at < readme->count && readme->lines[at][0] == '|') {
    fails += check_row(readme->lines[at], ++*rows, named);
    at++;
  }
  for (size_t i = 0; i < PART_COUNT; i++) {
    if (named[i]) continue;
    printf("FAIL: readme: the part %s is in no row\n", PARTS[i]);
    fails++;
  }
  return fails;
}

/* Checks each `langc VERB` of AGENTS: VERB must be one of the first COUNT
   verbs of USAGE, and AGENTS must name each of these verbs. Returns the FAIL
   count, and the count of the usage verbs that AGENTS names in *VERBS. */
static int check_agents(const Lines *agents, const Lines *usage, size_t count, size_t *verbs) {
  int used[MAX_VERBS] = {0};
  int fails = 0;
  for (size_t i = 0; i < agents->count; i++) {
    const char *at = strstr(agents->lines[i], VERB_PREFIX);
    while (at != NULL) {
      const char *verb = at + strlen(VERB_PREFIX);
      size_t k = find_name(usage->lines, count, verb, strspn(verb, VERB_CHARS));
      if (k < count) {
        used[k] = 1;
      } else {
        printf("FAIL: agents: line %zu: langc %.*s is not in the usage text of build/langc\n", i + 1, (int)strcspn(verb, " `"), verb);
        fails++;
      }
      at = strstr(verb, VERB_PREFIX);
    }
  }
  *verbs = 0;
  for (size_t k = 0; k < count; k++) {
    if (used[k]) {
      (*verbs)++;
    } else {
      printf("FAIL: agents: the verb %s of the usage text of build/langc is not in AGENTS\n", usage->lines[k]);
      fails++;
    }
  }
  return fails;
}

int main(int argc, char **argv) {
  const char *readme_path = argc > 1 ? argv[1] : "README.md";
  const char *agents_path = argc > 2 ? argv[2] : "AGENTS.md";
  Lines readme = {NULL, 0};
  Lines agents = {NULL, 0};
  Lines usage = {NULL, 0};
  int fails = 0;
  if (!read_file(readme_path, &readme)) {
    printf("FAIL: readme: cannot read %s\n", readme_path);
    fails++;
  }
  if (!read_file(agents_path, &agents)) {
    printf("FAIL: agents: cannot read %s\n", agents_path);
    fails++;
  }
  size_t verb_count = run_command("build/langc 2>&1", &usage) == -1 ? 0 : usage_verbs(&usage);
  if (verb_count == 0 || verb_count > MAX_VERBS) {
    printf("FAIL: agents: cannot read the usage text of build/langc\n");
    fails++;
  }
  size_t rows = 0;
  size_t verbs = 0;
  if (fails == 0) {
    fails += check_table(&readme, &rows);
    fails += check_agents(&agents, &usage, verb_count, &verbs);
  }
  if (fails == 0) {
    printf("readme: %zu rows, %zu verbs\n", rows, verbs);
  } else {
    printf("readme: %d mismatches\n", fails);
  }
  free_lines(&readme);
  free_lines(&agents);
  free_lines(&usage);
  return fails == 0 ? 0 : 1;
}
