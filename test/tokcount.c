#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/* The token count tool of docs/TOKEN-BASELINE.md (Q19 (a), Q90 (a), Q93 (a),
   Q95 (a)). The tool is a port of evolve-lang dd98209 test/tokcount.c. The
   count loop and the table are a port of leaf-lang 2fa3ea9 test/tokcount.c,
   and rule B is the word rule of quorum-lang 0c729b6 test/tokproxy.c. Each
   count is a fixed offline proxy, not the tokenizer of a model, on all of
   the file, comments too:
   - rule A (tokens A, compare with leaf-lang): a run of ASCII letters,
     digits and `_` is 1 token;
   - rule B (tokens B, compare with quorum-lang): a run of length L is
     ceil(L / 4) tokens, that is (L + 3) / 4;
   - in both rules, each other ASCII byte that is not white space is 1
     token, each byte from 0x80 is 1 token, and white space and control
     bytes are 0.
   The code points are the bytes below 0x80 or from 0xc0. The non-ASCII
   tokens are the tokens from the bytes from 0x80, the same under A and B.

   `build/tokcount FILE...` prints a Markdown table with one row for each
   FILE in the argument order, a blank line, then for each FILE after the
   first the ratio of its tokens A over the tokens A of the first FILE and
   the same ratio for B, then the share of the non-ASCII tokens in the
   tokens A of the first FILE and the same share for B (half up, 2
   decimals, integer arithmetic). There is no total row: each file is a
   full alternative, so a sum has no meaning.

   `build/tokcount --check DOC FILE...` makes the same text and compares it
   byte for byte with the lines between the tokcount markers of DOC. It
   checks the half-up rule on fixed pairs and runs `build/langc check` on
   each FILE that ends in `.merge` (exit 0). It prints a FAIL line for each
   mismatch, then `tokens: N files, 2 rules, M ratios, K mismatches`, and
   exits 0 only when K is 0. The share lines are not ratios. Run it from
   the repo root.

   A usage error, an invalid text document or a read or write error prints
   one line on stderr and exits 2. */

#define CHECK_FLAG "--check"
#define USAGE "usage: tokcount [--check DOC] FILE...\n"
#define BEGIN_MARK "<!-- tokcount begin -->"
#define END_MARK "<!-- tokcount end -->"
#define MERGE_SUFFIX ".merge"
#define RULE_COUNT 2

/* The characters of a path that the tool puts on a command line. */
#define PATH_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/._-"

/* A fixed pair of the half-up check: NUM over DEN gives TEXT. */
typedef struct {
  unsigned long num;
  unsigned long den;
  const char *text;
} Pair;

/* 201 over 200 is 1.005 and rounds up; 1 over 3 is 0.333 and rounds down. */
#define PAIR_COUNT 2
static const Pair PAIRS[PAIR_COUNT] = {
  {201, 200, "1.01"},
  {1, 3, "0.33"},
};

/* The counts of one file. */
typedef struct {
  unsigned long bytes;
  unsigned long points;
  unsigned long tokens_a;
  unsigned long tokens_b;
  unsigned long nonascii;
} Count;

/* The lines of a file, of the stdout of a command or of the text of the
   tool, without their newlines. */
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

/* Reads the lines of FILE into LINES. Returns 0 on a read error, embedded
   NUL or when there is no memory. */
static int read_stream(FILE *file, Lines *lines) {
  char *line = NULL;
  size_t size = 0;
  ssize_t len;
  int ok = 1;
  while (ok && (len = getline(&line, &size, file)) >= 0) {
    if (memchr(line, '\0', (size_t)len) != NULL) {
      ok = 0;
      break;
    }
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

/* Adds the line that FORMAT gives to LINES. Returns 0 when there is no
   memory. */
static int add_line(Lines *lines, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int len = vsnprintf(NULL, 0, format, args);
  va_end(args);
  char *line = len >= 0 ? malloc((size_t)len + 1) : NULL;
  char **grown = line != NULL ? realloc(lines->lines, (lines->count + 1) * sizeof *grown) : NULL;
  if (grown == NULL) {
    free(line);
    return 0;
  }
  va_start(args, format);
  vsnprintf(line, (size_t)len + 1, format, args);
  va_end(args);
  lines->lines = grown;
  lines->lines[lines->count++] = line;
  return 1;
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

/* Returns the index of the first line at FROM or after it that is TEXT, or
   the line count when there is none. */
static size_t find_exact(const Lines *lines, size_t from, const char *text) {
  size_t i = from;
  while (i < lines->count && strcmp(lines->lines[i], text) != 0) i++;
  return i;
}

static int has_suffix(const char *text, const char *suffix) {
  size_t len = strlen(text);
  size_t suffix_len = strlen(suffix);
  return len >= suffix_len && strcmp(text + len - suffix_len, suffix) == 0;
}

static int is_word(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

/* An ASCII byte that is not a word byte, white space or a control byte. */
static int is_punct(int c) {
  return c >= 0x21 && c <= 0x7e && !is_word(c);
}

/* Adds the tokens of a word run of length RUN to COUNT. */
static void end_run(Count *count, unsigned long run) {
  count->tokens_a += run > 0 ? 1 : 0;
  count->tokens_b += (run + 3) / 4;
}

/* Counts the file at PATH into COUNT. Returns 0 when the file cannot be
   read. */
static int count_file(const char *path, Count *count) {
  *count = (Count){0, 0, 0, 0, 0};
  FILE *file = fopen(path, "rb");
  if (file == NULL) return 0;
  unsigned long run = 0;
  int c;
  while ((c = getc(file)) != EOF) {
    count->bytes++;
    if (c < 0x80 || c >= 0xc0) count->points++;
    if (c >= 0x80) count->nonascii++;
    if (is_word(c)) {
      run++;
      continue;
    }
    end_run(count, run);
    run = 0;
    if (is_punct(c) || c >= 0x80) {
      count->tokens_a++;
      count->tokens_b++;
    }
  }
  end_run(count, run);
  int ok = !ferror(file);
  return fclose(file) == 0 && ok;
}

/* Writes NUM over DEN, rounded half up to two decimal places, to OUT, or
   `none` when DEN is 0. */
static void ratio(unsigned long num, unsigned long den, char *out, size_t size) {
  if (den == 0) {
    snprintf(out, size, "none");
    return;
  }
  unsigned long hundredths = (num * 200u + den) / (den * 2u);
  snprintf(out, size, "%lu.%02lu", hundredths / 100u, hundredths % 100u);
}

/* Makes the text of the COUNT files FILES into TEXT: the table, a blank line,
   the ratio lines and the share lines. Returns 0 when a file cannot be read (then *BAD is its
   path) or when there is no memory (then *BAD is NULL). */
static int make_text(char **files, size_t count, Lines *text, const char **bad) {
  Count *counts = calloc(count, sizeof *counts);
  int ok = counts != NULL;
  *bad = NULL;
  for (size_t i = 0; ok && i < count; i++) {
    ok = count_file(files[i], &counts[i]);
    if (!ok) *bad = files[i];
  }
  ok = ok && add_line(text, "| file | bytes | code points | tokens A | tokens B | non-ASCII tokens |");
  ok = ok && add_line(text, "| :--- | ---: | ---: | ---: | ---: | ---: |");
  for (size_t i = 0; ok && i < count; i++) {
    ok = add_line(text, "| %s | %lu | %lu | %lu | %lu | %lu |", files[i], counts[i].bytes, counts[i].points, counts[i].tokens_a, counts[i].tokens_b, counts[i].nonascii);
  }
  if (count > 0) ok = ok && add_line(text, "");
  for (size_t i = 1; ok && i < count; i++) {
    char a[32];
    char b[32];
    ratio(counts[i].tokens_a, counts[0].tokens_a, a, sizeof a);
    ratio(counts[i].tokens_b, counts[0].tokens_b, b, sizeof b);
    ok = add_line(text, "ratio of the tokens A (%s over %s): %s", files[i], files[0], a);
    ok = ok && add_line(text, "ratio of the tokens B (%s over %s): %s", files[i], files[0], b);
  }
  if (ok && count > 0) {
    char a[32];
    char b[32];
    ratio(counts[0].nonascii, counts[0].tokens_a, a, sizeof a);
    ratio(counts[0].nonascii, counts[0].tokens_b, b, sizeof b);
    ok = add_line(text, "share of the non-ASCII tokens in the tokens A of %s: %s", files[0], a);
    ok = ok && add_line(text, "share of the non-ASCII tokens in the tokens B of %s: %s", files[0], b);
  }
  free(counts);
  return ok;
}

/* Compares TEXT with the lines between the tokcount markers of the file
   PATH, whose lines are DOC. Returns the number of FAIL lines. */
static int check_region(const char *path, const Lines *doc, const Lines *text) {
  size_t begin = find_exact(doc, 0, BEGIN_MARK);
  size_t end = find_exact(doc, begin + 1, END_MARK);
  if (end >= doc->count) {
    printf("FAIL: tokens: %s has no tokcount region\n", path);
    return 1;
  }
  size_t len = end - begin - 1;
  size_t at = 0;
  while (at < len && at < text->count && strcmp(doc->lines[begin + 1 + at], text->lines[at]) == 0) at++;
  if (at == len && at == text->count) return 0;
  printf("FAIL: tokens: %s line %zu is not '%s'\n", path, begin + 2 + at, at < text->count ? text->lines[at] : END_MARK);
  return 1;
}

/* Checks the half-up rule on PAIRS. Returns the number of FAIL lines. */
static int check_pairs(void) {
  int fails = 0;
  for (size_t i = 0; i < PAIR_COUNT; i++) {
    char text[32];
    ratio(PAIRS[i].num, PAIRS[i].den, text, sizeof text);
    if (strcmp(text, PAIRS[i].text) == 0) continue;
    printf("FAIL: tokens: %lu over %lu gives %s, not %s\n", PAIRS[i].num, PAIRS[i].den, text, PAIRS[i].text);
    fails++;
  }
  return fails;
}

/* Checks each of the COUNT files FILES that ends in `.merge`: `build/langc
   check` exits 0. There is no `build/langc fmt` leg: the demo has no ASCII
   twin (Q93 (a)). Returns the number of FAIL lines. */
static int check_merge(char **files, size_t count) {
  int fails = 0;
  for (size_t i = 0; i < count; i++) {
    if (!has_suffix(files[i], MERGE_SUFFIX)) continue;
    if (strspn(files[i], PATH_CHARS) != strlen(files[i])) {
      printf("FAIL: tokens: the path %s has a character that the tool does not put on a command line\n", files[i]);
      fails++;
      continue;
    }
    char command[512];
    Lines out = {NULL, 0};
    int code = snprintf(command, sizeof command, "build/langc check %s 2>&1", files[i]) < (int)sizeof command ? run_command(command, &out) : -1;
    free_lines(&out);
    if (code != 0) {
      printf("FAIL: tokens: build/langc check %s exits %d, not 0\n", files[i], code);
      fails++;
    }
  }
  return fails;
}

/* Flush before reporting success: stdio can defer a write failure until
   after the last printf, in either command mode. */
static int output_status(int status) {
  if (fflush(stdout) == 0 && !ferror(stdout)) return status;
  fputs("tokcount: cannot write stdout\n", stderr);
  return 2;
}

int main(int argc, char **argv) {
  int check = argc > 1 && strcmp(argv[1], CHECK_FLAG) == 0;
  int first = check ? 3 : 1;
  if (argc <= first) {
    fputs(USAGE, stderr);
    return 2;
  }
  char **files = argv + first;
  size_t count = (size_t)(argc - first);
  Lines text = {NULL, 0};
  const char *bad = NULL;
  if (!make_text(files, count, &text, &bad)) {
    if (bad != NULL) {
      fprintf(stderr, "tokcount: cannot read %s\n", bad);
    } else {
      fputs("tokcount: no memory\n", stderr);
    }
    free_lines(&text);
    return 2;
  }
  if (!check) {
    for (size_t i = 0; i < text.count; i++) printf("%s\n", text.lines[i]);
    free_lines(&text);
    return output_status(0);
  }
  Lines doc = {NULL, 0};
  if (!read_file(argv[2], &doc)) {
    fprintf(stderr, "tokcount: cannot read %s\n", argv[2]);
    free_lines(&text);
    free_lines(&doc);
    return 2;
  }
  int fails = check_region(argv[2], &doc, &text) + check_pairs() + check_merge(files, count);
  printf("tokens: %zu files, %d rules, %zu ratios, %d mismatches\n", count, RULE_COUNT, (count - 1) * RULE_COUNT, fails);
  free_lines(&text);
  free_lines(&doc);
  return output_status(fails == 0 ? 0 : 1);
}
