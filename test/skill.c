/* Check the examples of skills/merge-lang/SKILL.md (S8-DESIGN sec 4, Q81 (a), Q82 (a)).
   Run it from the repo root. For each example, it compares the program block
   (the fence `merge-lang PATH`) with the file PATH byte for byte, compares the
   stdout of `build/langc check --json PATH` with the json block byte for byte,
   and compares the exit code with the sentence `The exit code is N.` */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define SKILL_PATH "skills/merge-lang/SKILL.md"
#define EXAMPLE_COUNT 5
#define PROGRAM_FENCE "```merge-lang "
#define JSON_FENCE "```json"
#define END_FENCE "```"
#define EXIT_PREFIX "The exit code is "

/* The characters of a path that the test puts on a command line. */
#define PATH_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/._-"

/* The lines of a file, without their newlines. */
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

/* Reads the lines of the file PATH into LINES. Returns 0 when the file cannot
   be read or when there is no memory. */
static int read_file(const char *path, Lines *lines) {
  FILE *file = fopen(path, "rb");
  char *line = NULL;
  size_t size = 0;
  ssize_t len;
  int ok = file != NULL;
  while (ok && (len = getline(&line, &size, file)) >= 0) {
    char **grown = realloc(lines->lines, (lines->count + 1) * sizeof *grown);
    ok = grown != NULL;
    if (ok) {
      if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
      grown[lines->count++] = line;
      lines->lines = grown;
      line = NULL;
      size = 0;
    }
  }
  free(line);
  ok = ok && !ferror(file);
  return file != NULL && fclose(file) == 0 && ok;
}

/* Returns the index of the first line at FROM or after it that starts with
   PREFIX, or the line count when there is none. */
static size_t find_line(const Lines *lines, size_t from, const char *prefix) {
  size_t i = from;
  while (i < lines->count && strncmp(lines->lines[i], prefix, strlen(prefix)) != 0) i++;
  return i;
}

/* Returns 1 when the bytes of FILE are the lines FROM to TO - 1 of LINES,
   each with its newline. Reads FILE to its end. */
static int same_bytes(FILE *file, const Lines *lines, size_t from, size_t to) {
  int same = 1;
  for (size_t i = from; same && i < to; i++) {
    for (const char *c = lines->lines[i]; same && *c != '\0'; c++) same = getc(file) == (unsigned char)*c;
    same = same && getc(file) == '\n';
  }
  same = same && getc(file) == EOF;
  while (getc(file) != EOF) {}
  return same && !ferror(file);
}

/* Returns 1 when the bytes of the file PATH are the lines FROM to TO - 1 of
   LINES. */
static int same_file(const char *path, const Lines *lines, size_t from, size_t to) {
  FILE *file = fopen(path, "rb");
  int same;
  if (file == NULL) return 0;
  same = same_bytes(file, lines, from, to);
  return fclose(file) == 0 && same;
}

/* Runs `build/langc check --json PATH` and sets *SAME to 1 when its stdout is
   the lines FROM to TO - 1 of LINES. Returns the exit code, or -1 when the
   command cannot run. */
static int run_check(const char *path, const Lines *lines, size_t from, size_t to, int *same) {
  char command[512];
  FILE *pipe;
  int status;
  *same = 0;
  if (snprintf(command, sizeof command, "build/langc check --json %s", path) >= (int)sizeof command) return -1;
  pipe = popen(command, "r");
  if (pipe == NULL) return -1;
  *same = same_bytes(pipe, lines, from, to);
  status = pclose(pipe);
  return status != -1 && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* Checks example N, whose program fence is at line AT of SKILL. Sets *NEXT to
   the line after the exit sentence. Returns 1 when the program, the output
   and the exit code match. */
static int check_example(const Lines *skill, size_t at, size_t n, size_t *next) {
  const char *path = skill->lines[at] + strlen(PROGRAM_FENCE);
  size_t to = find_line(skill, at + 1, END_FENCE);
  size_t json = find_line(skill, to + 1, JSON_FENCE);
  size_t json_to = find_line(skill, json + 1, END_FENCE);
  size_t exit_at = find_line(skill, json_to + 1, EXIT_PREFIX);
  const char *text;
  char *end = NULL;
  long want;
  int code, same, ok = 1;
  *next = exit_at + 1;
  if (exit_at >= skill->count || strspn(path, PATH_CHARS) != strlen(path)) {
    fprintf(stderr, "FAIL skill: example %zu (%s) has no program, json or exit block\n", n, path);
    return 0;
  }
  if (!same_file(path, skill, at + 1, to)) {
    fprintf(stderr, "FAIL skill: example %zu: the program block is not %s\n", n, path);
    ok = 0;
  }
  code = run_check(path, skill, json + 1, json_to, &same);
  if (!same) {
    fprintf(stderr, "FAIL skill: example %zu: the stdout of check --json %s is not the json block\n", n, path);
    ok = 0;
  }
  text = skill->lines[exit_at] + strlen(EXIT_PREFIX);
  want = strtol(text, &end, 10);
  if (end == text || strcmp(end, ".") != 0 || code != want) {
    fprintf(stderr, "FAIL skill: example %zu: exit %d, the sentence gives '%s'\n", n, code, text);
    ok = 0;
  }
  return ok;
}

int main(void) {
  Lines skill = {NULL, 0};
  size_t examples = 0, matches = 0, at, next;
  int ok = read_file(SKILL_PATH, &skill);
  if (!ok) fprintf(stderr, "FAIL skill: cannot read %s\n", SKILL_PATH);
  for (at = find_line(&skill, 0, PROGRAM_FENCE); at < skill.count; at = find_line(&skill, next, PROGRAM_FENCE)) {
    int match = check_example(&skill, at, ++examples, &next);
    matches += (size_t)match;
    ok &= match;
  }
  if (examples != EXAMPLE_COUNT) {
    fprintf(stderr, "FAIL skill: %s has %zu examples, not %d\n", SKILL_PATH, examples, EXAMPLE_COUNT);
    ok = 0;
  }
  if (ok) printf("skill: %zu examples, %zu outputs match\n", examples, matches);
  free_lines(&skill);
  return ok ? 0 : 1;
}
