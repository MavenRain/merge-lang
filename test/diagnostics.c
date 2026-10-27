/* Exercise the CLI's warning construction and apply each JSON edit span. */
#define main langc_cli_main
#include "../src/main.c"
#undef main

/* Locate a code-point column without stopping inside a UTF-8 sequence. */
static size_t source_offset(const char *text, size_t len, int line, int col) {
  int at_line = 1;
  int at_col = 1;
  for (size_t i = 0; i <= len; i++) {
    unsigned char byte = i == len ? 0 : (unsigned char)text[i];
    if ((byte & 0xc0u) != 0x80u && at_line == line && at_col == col) return i;
    if (byte == '\n') {
      at_line++;
      at_col = 1;
    } else if ((byte & 0xc0u) != 0x80u) {
      at_col++;
    }
  }
  return len + 1;
}

static int warning_edit(const char *text, int line, int end_line, int end_col) {
  Arena arena;
  Diag diag;
  Diag warn;
  Front front;
  FmtText canonical;
  size_t len = strlen(text);
  int ok = 0;
  arena_init(&arena, (size_t)1 << 24);
  diag_init(&diag);
  diag_init(&warn);
  if (!front_load(&arena, "warning-edit", text, len, &front, &diag)) goto done;
  noncanon(&arena, "warning-edit", text, len, &front, &warn);
  if (!warn.set || warn.span.line != line || warn.span.col != 1
      || warn.span.end_line != end_line || warn.span.end_col != end_col) goto done;
  size_t start = source_offset(text, len, warn.span.line, warn.span.col);
  size_t end = source_offset(text, len, warn.span.end_line, warn.span.end_col);
  if (start > end || end > len || warn.found == NULL || warn.fix == NULL
      || strlen(warn.found) != end - start || memcmp(warn.found, text + start, end - start) != 0) goto done;
  size_t fix_len = strlen(warn.fix);
  size_t edited_len = len - (end - start) + fix_len;
  char *edited = arena_alloc(&arena, edited_len + 1);
  if (edited == NULL) goto done;
  memcpy(edited, text, start);
  memcpy(edited + start, warn.fix, fix_len);
  memcpy(edited + start + fix_len, text + end, len - end);
  fmt_text_init(&canonical, &arena);
  if (!fmt_program(&canonical, front.decls.items + front.domain_count,
                   front.decls.count - front.domain_count, &front.toks)) goto done;
  if (canonical.len != edited_len || memcmp(canonical.data, edited, edited_len) != 0) goto done;
  diag_init(&diag);
  diag_init(&warn);
  if (!front_load(&arena, "warning-edit", edited, edited_len, &front, &diag)) goto done;
  noncanon(&arena, "warning-edit", edited, edited_len, &front, &warn);
  ok = !warn.set;
done:
  if (!ok) fprintf(stderr, "FAIL warning edit: expected %d:1 to %d:%d\n", line, end_line, end_col);
  arena_release(&arena);
  return ok;
}

int main(void) {
  int ok = 1;
  ok &= warning_edit("-- heading\nvalue : Nat\nvalue = 1\n", 1, 4, 1);
  ok &= warning_edit("firstValue : ℕ\nfirstValue = 1\n\n-- heading\nvalue : Nat\nvalue = 1\n", 3, 7, 1);
  ok &= warning_edit("value : ℕ\nvalue = 1\n\n-- λ", 3, 4, 5);
  ok &= warning_edit("value : ℕ\nvalue = 1\n  \n\n  ", 3, 5, 3);
  ok &= warning_edit("value : ℕ\nvalue = 1", 1, 2, 10);
  ok &= warning_edit("-- λ\nvalue : Nat\nvalue = 1 -- λ", 1, 3, 15);
  if (ok) puts("diagnostic edit regressions: passed");
  return ok ? 0 : 1;
}
