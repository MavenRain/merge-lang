#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Exercise the counter through its CLI, including buffered output errors
   and a document whose visible table hides extra bytes after a NUL. */
static int run(char *const args[], int output) {
  FILE *errors = tmpfile();
  if (errors == NULL) return -1;
  pid_t child = fork();
  if (child == 0) {
    if (dup2(output, STDOUT_FILENO) < 0 || dup2(fileno(errors), STDERR_FILENO) < 0) _exit(126);
    execv(args[0], args);
    _exit(127);
  }
  int status = 0;
  int waited = child > 0 && waitpid(child, &status, 0) == child;
  fclose(errors);
  return waited && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static int expect(const char *name, char *const args[], int output, int wanted) {
  int actual = run(args, output);
  if (actual == wanted) return 0;
  printf("FAIL: tokens IO: %s exits %d, expected %d\n", name, actual, wanted);
  return 1;
}

static int nul_document(const char *path) {
  FILE *source = fopen("docs/TOKEN-BASELINE.md", "rb");
  FILE *target = fopen(path, "wb");
  if (source == NULL || target == NULL) {
    if (source != NULL) fclose(source);
    if (target != NULL) fclose(target);
    return 0;
  }
  char *line = NULL;
  size_t size = 0;
  ssize_t len;
  int inserted = 0;
  int ok = 1;
  while (ok && (len = getline(&line, &size, source)) >= 0) {
    if (strncmp(line, "| file |", 8) == 0 && len > 0 && line[len - 1] == '\n') {
      ok = fwrite(line, 1, (size_t)len - 1, target) == (size_t)len - 1;
      ok = ok && fwrite("\0extra\n", 1, 7, target) == 7;
      inserted = 1;
    } else {
      ok = fwrite(line, 1, (size_t)len, target) == (size_t)len;
    }
  }
  free(line);
  ok = ok && inserted && !ferror(source);
  int read_close = fclose(source);
  int write_close = fclose(target);
  return ok && read_close == 0 && write_close == 0;
}

int main(void) {
  char *table[] = {"build/tokcount", "examples/inventory-escrow.merge", NULL};
  char *check[] = {"build/tokcount", "--check", "docs/TOKEN-BASELINE.md", "examples/inventory-escrow.merge",
                  "test/baseline/inventory.py", "test/baseline/inventory.rs", NULL};
  FILE *output = tmpfile();
  int readonly = open("/dev/null", O_RDONLY);
  if (output == NULL || readonly < 0) {
    if (output != NULL) fclose(output);
    if (readonly >= 0) close(readonly);
    fputs("FAIL: tokens IO: cannot open test streams\n", stderr);
    return 1;
  }
  int fails = expect("table", table, fileno(output), 0);
  fails += expect("check", check, fileno(output), 0);
  fails += expect("table with unwritable stdout", table, readonly, 2);
  fails += expect("check with unwritable stdout", check, readonly, 2);
  char path[] = "build/merge-tokens-XXXXXX";
  int fd = mkstemp(path);
  int created = fd >= 0;
  if (created) close(fd);
  if (!created || !nul_document(path)) {
    puts("FAIL: tokens IO: cannot create NUL document");
    fails++;
  } else {
    check[2] = path;
    fails += expect("document with hidden bytes", check, fileno(output), 2);
  }
  if (created) unlink(path);
  close(readonly);
  fclose(output);
  printf("tokens IO regressions: 5 cases, %d failures\n", fails);
  return fails == 0 ? 0 : 1;
}
