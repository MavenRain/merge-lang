/* The JSON writer of `check --json` (and of the build report). The
   buffer and the string escaper come from the tcc-json kit (lang-template
   5265a2a hosts/tcc-json/src/json.c); the escaper keeps valid UTF-8. */
#ifndef LANG_FRONT_JSON_H
#define LANG_FRONT_JSON_H

#include <stddef.h>

#include "front/base.h"
#include "front/core.h"
#include "front/front.h"

/* A text that grows in ARENA. DATA ends in '\0'. A failure records OOM
   (or INTERNAL) in DIAG. */
typedef struct {
  Arena *arena;
  Diag *diag;
  const char *def; /* the definition for diagnostics, or NULL */
  char *data;
  size_t len;
  size_t cap;
} Out;

void out_init(Out *o, Arena *arena, Diag *diag);
/* Each put returns 1, or 0 after a diagnostic. */
int put(Out *o, const char *text, size_t n);
int put_text(Out *o, const char *text);
/* A printf form for numbers (at most 255 bytes). */
int put_format(Out *o, const char *fmt, ...);
/* NULL puts null. Control bytes, invalid UTF-8 bytes, `"` and `\` are
   escaped; valid UTF-8 is kept. */
int put_string(Out *o, const char *text);
/* Puts one JSON object (no newline): code, severity, span, expected,
   found, fix, retry, message. */
int diag_print_json(const Diag *diag, Out *o);

/* What `check` knows besides the machine and the front. After a read or lex
   failure the front and the machine are all zero. */
typedef struct {
  Arena *arena;         /* the document text */
  Diag *sink;           /* a writer failure */
  const char *file;     /* the PROG path as given */
  int parsed, typed;    /* the results of front_load and check_program */
  const Diag *error;    /* the error (not NULL), in the document when set */
  const Diag *noncanon; /* the W_NONCANON warning (not NULL), in the document when set */
} ReportInput;

/* The coordination report (S6-DESIGN sec 2 to 4) as one line with no
   newline: version, file, replicas, bounds, progress, pairs, diagnostics.
   Returns 1 and sets TEXT and LEN, or 0 after a diagnostic in IN->sink. */
int report_document(Machine *m, const Front *front, const ReportInput *in, const char **text, size_t *len);

#endif
