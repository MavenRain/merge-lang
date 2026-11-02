/* The diagnostic codes of `langc explain-code` (S8 design sec 3). */
#ifndef LANG_CODES_H
#define LANG_CODES_H

#include <stddef.h>

/* One row of the docs/DIAGNOSTICS.md table. Each cell is the cell of the
   table without the spaces around it; the code has no backticks. The
   meaning is the Meaning cell verbatim, with its Markdown (Q80 (a)). */
typedef struct {
  const char *code;
  const char *severity;
  const char *stage;
  const char *meaning;
  const char *fixture;
} CodeRow;

/* The number of rows: the 47 codes of docs/DIAGNOSTICS.md. */
size_t code_count(void);

/* Row INDEX in the order of docs/DIAGNOSTICS.md, or NULL when INDEX is not
   below code_count(). */
const CodeRow *code_at(size_t index);

/* The row of CODE, or NULL when no row has that code. */
const CodeRow *code_find(const char *code);

#endif
