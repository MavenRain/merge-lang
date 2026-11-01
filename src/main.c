/* The langc command line. SHARED by the tcc kits.
   Exit 0: ok. Exit 1: the program is refused. Exit 2: usage or IO. */
#include <stdio.h>
#include <string.h>

#include "front/check.h"
#include "front/fmt.h"
#include "front/front.h"
#include "front/json.h"
#include "front/lower.h"
#include "front/parser.h"
#include "target.h"

#define ARENA_LIMIT_BYTES ((size_t)1 << 30)

typedef enum {
  CMD_CHECK,
  CMD_EVAL,
  CMD_IR,
  CMD_ABI,
  CMD_BUILD,
  CMD_FMT,
  CMD_SIM
} Command;

/* The sim bounds (S5 design sec 1): replicas N, prefix P, ops M. */
enum { BOUND_REPLICAS, BOUND_PREFIX, BOUND_OPS, BOUND_COUNT };
static const char *const BOUND_FLAGS[BOUND_COUNT] = {"--replicas", "--prefix", "--ops"};

typedef struct {
  Command command;
  const char *prog_path;
  const char *entry;
  char **args;
  int arg_count;
  const char *out_path;
  int runtime;
  int json;
  uint32_t bound[BOUND_COUNT]; /* sim: Q17 (a) defaults P 1, M 2, N declared */
  int bound_set[BOUND_COUNT];
} Options;

static const struct {
  const char *word;
  Command command;
} COMMANDS[] = {
  {"check", CMD_CHECK},
  {"eval", CMD_EVAL},
  {"ir", CMD_IR},
  {"abi", CMD_ABI},
  {"build", CMD_BUILD},
  {"fmt", CMD_FMT},
  {"sim", CMD_SIM},
};

static int usage(FILE *err) {
  fputs("usage: langc check [--json] PROG\n"
        "       langc eval PROG NAME [ARGS...]\n"
        "       langc ir PROG\n"
        "       langc abi PROG\n"
        "       langc build PROG -o OUT [--runtime]\n"
        "       langc fmt PROG\n"
        "       langc sim PROG [--replicas N] [--prefix P] [--ops M]\n", err);
  return 2;
}

static int find_command(const char *word, Command *out) {
  for (size_t i = 0; i < sizeof COMMANDS / sizeof COMMANDS[0]; i++) {
    if (strcmp(COMMANDS[i].word, word) == 0) {
      *out = COMMANDS[i].command;
      return 1;
    }
  }
  return 0;
}

static int parse_build_options(int argc, char **argv, Options *opt, Diag *diag) {
  for (int i = 3; i < argc; i++) {
    int is_out = strcmp(argv[i], "-o") == 0 && i + 1 < argc;
    int is_runtime = strcmp(argv[i], "--runtime") == 0;
    if (!is_out && !is_runtime) return diag_fail(diag, "USAGE", NULL, "unknown build argument '%s'", argv[i]);
    opt->out_path = is_out ? argv[i + 1] : opt->out_path;
    opt->runtime |= is_runtime;
    i += is_out;
  }
  return opt->out_path != NULL ? 1 : diag_fail(diag, "USAGE", NULL, "build needs -o OUT");
}

/* A bound: one decimal numeral, at most SEARCH_CAP. */
static int parse_bound(const char *s, uint32_t *out) {
  uint32_t n = 0;
  size_t i;
  for (i = 0; s[i] >= '0' && s[i] <= '9' && n <= SEARCH_CAP; i++) n = n * 10u + (uint32_t)(s[i] - '0');
  *out = n;
  return i > 0u && s[i] == '\0' && n <= SEARCH_CAP;
}

static int parse_sim_options(int argc, char **argv, Options *opt, Diag *diag) {
  opt->bound[BOUND_PREFIX] = 1u;
  opt->bound[BOUND_OPS] = 2u;
  for (int i = 3; i < argc; i += 2) {
    int k = 0;
    while (k < BOUND_COUNT && strcmp(argv[i], BOUND_FLAGS[k]) != 0) k++;
    if (k == BOUND_COUNT || i + 1 >= argc) return diag_fail(diag, "USAGE", NULL, "unknown sim argument '%s'", argv[i]);
    if (opt->bound_set[k]) return diag_fail(diag, "USAGE", NULL, "sim: %s is given twice", argv[i]);
    if (!parse_bound(argv[i + 1], &opt->bound[k]))
      return diag_fail(diag, "USAGE", NULL, "sim: %s needs a numeral at most %u", argv[i], SEARCH_CAP);
    opt->bound_set[k] = 1;
  }
  return 1;
}

static int parse_options(int argc, char **argv, Options *opt, Diag *diag) {
  memset(opt, 0, sizeof *opt);
  if (argc < 3 || !find_command(argv[1], &opt->command)) {
    return diag_fail(diag, "USAGE", NULL, "expected a command and a program path");
  }
  opt->json = opt->command == CMD_CHECK && strcmp(argv[2], "--json") == 0;
  opt->prog_path = argv[2 + opt->json];
  switch (opt->command) {
    case CMD_CHECK:
    case CMD_IR:
    case CMD_ABI:
    case CMD_FMT:
      if (opt->json && argc == 3) return diag_fail(diag, "USAGE", NULL, "check --json needs a program path");
      return argc == 3 + opt->json ? 1 : diag_fail(diag, "USAGE", NULL, "too many arguments");
    case CMD_EVAL:
      if (argc < 4) return diag_fail(diag, "USAGE", NULL, "eval needs a definition name");
      opt->entry = argv[3];
      opt->args = argv + 4;
      opt->arg_count = argc - 4;
      return 1;
    case CMD_BUILD:
      return parse_build_options(argc, argv, opt, diag);
    case CMD_SIM:
      return parse_sim_options(argc, argv, opt, diag);
  }
  return 0;
}

static int read_source(Arena *arena, const char *path, char **out, size_t *len, Diag *diag) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) return diag_fail(diag, "IO", NULL, "cannot open %s", path);
  char *buf = arena_alloc(arena, SOURCE_MAX_BYTES + 1);
  size_t count = buf == NULL ? 0 : fread(buf, 1, SOURCE_MAX_BYTES + 1, file);
  int read_error = ferror(file);
  fclose(file);
  if (buf == NULL) return diag_fail(diag, "OOM", NULL, "no memory for %s", path);
  if (read_error) return diag_fail(diag, "IO", NULL, "cannot read %s", path);
  if (count > SOURCE_MAX_BYTES) return diag_fail(diag, "IO_SIZE", NULL, "%s is larger than %zu bytes", path, SOURCE_MAX_BYTES);
  *out = buf;
  *len = count;
  return 1;
}

/* Lowers the entries, then runs the ir, abi or build command. */
static int run_target(const Options *opt, Machine *machine, Diag *diag) {
  IrProgram prog;
  FILE *out;
  size_t i;
  int ok = 1;
  if (!lower_program(machine, &prog)) return 1;
  switch (opt->command) {
    case CMD_IR:
      ir_print(&prog, stdout);
      return 0;
    case CMD_ABI:
      for (i = 0; i < prog.func_count && ok; i++) ok = target_abi_line(&prog.funcs[i], stdout, stderr);
      return ok ? 0 : 1;
    case CMD_CHECK:
    case CMD_EVAL:
    case CMD_BUILD:
    case CMD_FMT:
    case CMD_SIM:
      break;
  }
  out = fopen(opt->out_path, "wb");
  if (out == NULL) return diag_fail(diag, "IO", NULL, "cannot write '%s'", opt->out_path) + 1;
  ok = target_write(&prog, opt->runtime ? TARGET_PART_RUNTIME : TARGET_PART_MAIN, out, stderr);
  return fclose(out) == 0 && ok ? 0 : 1;
}

static int run(const Options *opt, Arena *arena, Diag *diag) {
  char *text = NULL;
  size_t len = 0;
  if (!read_source(arena, opt->prog_path, &text, &len, diag)) return 2;
  Front front;
  if (!front_load(arena, opt->prog_path, text, len, &front, diag)) return 1;
  Machine machine;
  if (!check_program(arena, &front.decls, &machine, diag)) return 1;
  switch (opt->command) {
    case CMD_EVAL:
      return eval_command(&machine, opt->entry, opt->args, opt->arg_count, stdout, stderr);
    case CMD_CHECK: /* run_check does check */
    case CMD_IR:
    case CMD_ABI:
    case CMD_BUILD: /* run_build does build */
    case CMD_FMT: /* run_fmt does fmt */
    case CMD_SIM: /* run_sim does sim */
      break;
  }
  return run_target(opt, &machine, diag);
}

/* langc sim (S5 design sec 1 and 2): check in sim mode (Q57 (a)), then the
   sim search of each replicated form in the declaration order, to the
   first violation. K on a refusal is the kept count plus 1 (Q59 (a)). A
   violation prints the sim_print transcript (sec 4) after the header. With
   no invariant in a form, the search takes it as true, and with no
   invariant in all forms, there is no verdict line (S6 design Q67 (a)). */
static int has_invariant(const Decl *d) {
  size_t j;
  int found = 0;
  for (j = 0; j < d->item_count && !found; j++) found = d->items[j].kind == ITEM_INVARIANT;
  return found;
}

static int run_sim(const Options *opt, Arena *arena, Diag *diag) {
  char *text = NULL;
  size_t len = 0, i, at = 0;
  Front front;
  Machine machine;
  SearchOutcome out;
  SearchBounds b = {opt->bound[BOUND_PREFIX], opt->bound[BOUND_OPS], opt->bound[BOUND_REPLICAS]};
  const Decl *replicas = NULL;
  uint64_t kept = 0;
  int refuted = 0, invariant = 0;
  if (!read_source(arena, opt->prog_path, &text, &len, diag)) return 2;
  if (!front_load(arena, opt->prog_path, text, len, &front, diag)) return 1;
  if (!check_program_sim(arena, &front.decls, &machine, diag)) return 1;
  for (i = 0; i < front.decls.count && replicas == NULL; i++)
    replicas = front.decls.items[i].kind == DECL_REPLICAS ? &front.decls.items[i] : NULL;
  if (replicas == NULL) return diag_fail(diag, "USAGE", NULL, "sim: %s has no replicas form", opt->prog_path) + 2;
  b.replicas = opt->bound_set[BOUND_REPLICAS] ? b.replicas : (uint32_t)replicas->name_count;
  if (b.replicas > replicas->name_count)
    return diag_fail(diag, "USAGE", NULL, "sim: --replicas %u is above the %zu declared replicas", b.replicas, replicas->name_count) + 2;
  if ((uint64_t)b.prefix + (uint64_t)b.ops * b.replicas > SEARCH_CAP)
    return diag_fail(diag, "USAGE", NULL, "sim: the bounds give more than %u instances", SEARCH_CAP) + 2;
  for (i = 0; i < front.decls.count && !refuted; i++) {
    if (front.decls.items[i].kind != DECL_REPLICATED) continue;
    if (!sim_form(&machine, &front.decls, i, b, &out)) return 1;
    if (out.result == SEARCH_ABOVE_CAP)
      return diag_fail(diag, "INTERNAL", NULL, "sim: %s visits more than %u schedules", front.decls.items[i].name, SEARCH_CAP) + 1;
    kept += out.kept;
    refuted = out.result == SEARCH_VIOLATION;
    invariant = invariant || has_invariant(&front.decls.items[i]);
    at = i;
  }
  printf("merge-lang sim %s\nreplicas", opt->prog_path);
  for (i = 0; i < b.replicas; i++) printf(" %s", replicas->names[i].text);
  printf("; bounds prefix %u, ops %u; schedules %llu\n", b.prefix, b.ops, (unsigned long long)(kept + (uint64_t)refuted));
  if (refuted) {
    (void)sim_print(&machine, &front.decls, at, b, &out); /* rc 1 both ways: a failure is INTERNAL */
    return 1;
  }
  if (invariant)
    printf("invariant kept on all %llu schedules (bounded: %u, %u, %u)\n", (unsigned long long)kept, b.prefix, b.ops, b.replicas);
  return 0;
}

/* The span of declaration D. A domain declaration is in domain/domain.lang. */
static Span decl_span(const char *path, const Decl *d) {
  Span span = {d->origin == ORIGIN_DOMAIN ? "domain/domain.lang" : path, d->line, d->col, d->end_line, d->end_col};
  return span;
}

/* The offset after COUNT more newlines from AT, or LEN. */
static size_t skip_lines(const char *text, size_t len, size_t at, int count) {
  size_t pos = at;
  for (int n = 0; n < count && pos < len; pos++) n += text[pos] == '\n';
  return pos;
}

static int count_newlines(const char *text, size_t len) {
  int n = 0;
  for (size_t i = 0; i < len; i++) n += text[i] == '\n';
  return n;
}

/* Records W_NONCANON in WARN for the region of lines FIRST to LAST of
   declaration D (NULL for the tail). FOUND is the input text of the region,
   FIX its canonical text. */
static void warn_region(Arena *arena, const char *path, const Decl *d, int first, const char *found,
                        size_t found_len, const char *fix, size_t fix_len, Diag *warn) {
  Span span = {path, first, 1, first, 1};
  for (size_t i = 0; i < found_len; i++) {
    unsigned char byte = (unsigned char)found[i];
    if (byte == '\n') {
      span.end_line++;
      span.end_col = 1;
    } else if ((byte & 0xc0u) != 0x80u) {
      span.end_col++;
    }
  }
  int last = span.end_line - (span.end_col == 1 && span.end_line > first);
  diag_fail_at(warn, "W_NONCANON", d != NULL ? d->name : NULL, span, NULL, NULL, NULL,
               "%s:%d:%d: lines %d to %d are not canonical; replace them with the fix text (the langc fmt output)",
               path, span.line, span.col, first, last);
  const char *found_copy = arena_strndup(arena, found, found_len);
  const char *fix_copy = fix_len == 0 ? "" : arena_strndup(arena, fix, fix_len);
  diag_attach(warn, fix_copy, found_copy, fix_copy);
}

/* Records one W_NONCANON in WARN for the first program region that differs
   from its canonical text (HANDOFF decision 3): region I runs from the line
   after declaration I-1 to the end of the last line of declaration I. The
   text after the last declaration is one more region (the tail). */
static void noncanon(Arena *arena, const char *path, const char *text, size_t len, const Front *front, Diag *warn) {
  const Decl *decls = front->decls.items + front->domain_count;
  size_t count = front->decls.count - front->domain_count;
  FmtText canon;
  size_t next_comment = 0;
  size_t pos = 0;
  int prev_line = 0;
  fmt_text_init(&canon, arena);
  for (size_t i = 0; i <= count && !warn->set && !canon.oom; i++) {
    int tail = i == count;
    size_t before = canon.len;
    size_t end = tail ? len : skip_lines(text, len, pos, decls[i].end_line - prev_line);
    if (tail) {
      fmt_tail(&canon, count, &front->toks, next_comment);
    } else {
      fmt_region(&canon, decls, i, &front->toks, &next_comment);
    }
    size_t fix_len = canon.len - before;
    int same = fix_len == end - pos && (fix_len == 0 || memcmp(canon.data + before, text + pos, fix_len) == 0);
    int lines = count_newlines(text + pos, end - pos);
    int last = tail ? prev_line + (lines > 0 ? lines : 1) : decls[i].end_line;
    if (!same && !canon.oom) {
      warn_region(arena, path, tail ? NULL : &decls[i], prev_line + 1, text + pos, end - pos, canon.data + before,
                  fix_len, warn);
    }
    prev_line = last;
    pos = end;
  }
}

/* The check of `check` and `build`: the source, the front end, the check,
   the default spans and the W_NONCANON warning. LOADED, PARSED and TYPED
   tell how far it went; the error (at most one) is in the caller's DIAG. */
typedef struct {
  Front front;
  Machine machine;
  Diag warn;
  int loaded, parsed, typed;
} Checked;

static void check_source(const Options *opt, Arena *arena, Checked *c, Diag *diag) {
  char *text = NULL;
  size_t len = 0;
  memset(&c->front, 0, sizeof c->front);
  memset(&c->machine, 0, sizeof c->machine);
  diag_init(&c->warn);
  c->loaded = read_source(arena, opt->prog_path, &text, &len, diag);
  c->parsed = c->loaded && front_load(arena, opt->prog_path, text, len, &c->front, diag);
  c->typed = c->parsed && check_program(arena, &c->front.decls, &c->machine, diag);
  if (c->parsed && !c->typed && c->machine.decl_failed) {
    diag_default_span(diag, decl_span(opt->prog_path, &c->front.decls.items[c->machine.checked]));
  }
  if (c->parsed) noncanon(arena, opt->prog_path, text, len, &c->front, &c->warn);
  for (uint32_t i = 0; i < c->machine.warn_count; i++) {
    diag_default_span(c->machine.warns[i], (Span){opt->prog_path, 0, 0, 0, 0});
  }
}

/* Writes the report document of C (one line) and a newline to OUT. 0 on a
   failure: a report failure puts a diagnostic in SINK, an IO failure does
   not. */
static int write_report(const Options *opt, Arena *arena, Checked *c, const Diag *diag, FILE *out, Diag *sink) {
  ReportInput in = {arena, sink, opt->prog_path, c->parsed, c->typed, diag, &c->warn};
  const char *doc = NULL;
  size_t doc_len = 0;
  return report_document(&c->machine, &c->front, &in, &doc, &doc_len) && fwrite(doc, 1, doc_len, out) == doc_len
         && fputc('\n', out) != EOF;
}

/* `check` in both modes: the error (at most one), then the W_BOUNDED
   warnings (one for each bounded op pair), then the W_NONCANON warning (at
   most one), then `ok` (text). JSON mode gives one report document (one
   line) that holds these diagnostics. A warning does not change the exit
   code. */
static int run_check(const Options *opt, Arena *arena, Diag *diag) {
  Checked c;
  check_source(opt, arena, &c, diag);
  if (opt->json) {
    Diag sink;
    diag_init(&sink);
    int printed = write_report(opt, arena, &c, diag, stdout, &sink)
                  || diag_fail(&sink, "IO", NULL, "cannot write the JSON output");
    if (!printed) {
      diag_print(&sink, stderr);
      return 2;
    }
  } else {
    if (diag->set) diag_print(diag, stderr);
    for (uint32_t i = 0; i < c.machine.warn_count; i++) diag_print(c.machine.warns[i], stderr);
    if (c.warn.set) diag_print(&c.warn, stderr);
    if (c.typed) puts("ok");
  }
  return c.typed ? 0 : c.loaded ? 1 : 2;
}

/* The report path of OUT (S6 design sec 1, Q61): OUT with the suffix `.wasm`
   replaced by `.report.json`, or with `.report.json` added. NULL when there
   is no memory. */
static const char *report_path(Arena *arena, const char *out) {
  static const char suffix[] = ".report.json";
  size_t len = strlen(out);
  size_t keep = len >= 5 && strcmp(out + len - 5, ".wasm") == 0 ? len - 5 : len;
  char *path = arena_alloc(arena, keep + sizeof suffix);
  if (path == NULL) return NULL;
  memcpy(path, out, keep);
  memcpy(path + keep, suffix, sizeof suffix);
  return path;
}

/* `build` (S6 design sec 1): the check, then the report (also on a refusal,
   and on a lex or parse failure), then the lowering and OUT when the check
   is ok. A refusal gives exit 1 and no OUT; an IO failure on the report
   gives exit 2. The report bytes equal the `check --json` stdout. */
static int run_build(const Options *opt, Arena *arena, Diag *diag) {
  Checked c;
  Diag sink;
  check_source(opt, arena, &c, diag);
  diag_init(&sink);
  const char *path = report_path(arena, opt->out_path);
  FILE *out = path == NULL ? NULL : fopen(path, "wb");
  int written = out != NULL && write_report(opt, arena, &c, diag, out, &sink);
  int closed = out == NULL || fclose(out) == 0;
  if (path == NULL) diag_fail(&sink, "OOM", NULL, "no memory for the report path of '%s'", opt->out_path);
  if (!written || !closed) {
    diag_fail(&sink, "IO", NULL, "cannot write '%s'", path);
    diag_print(&sink, stderr);
    return 2;
  }
  return c.typed ? run_target(opt, &c.machine, diag) : c.loaded ? 1 : 2;
}

/* Lexes and parses the program only (no domain, no check), then prints the
   canonical text. */
static int run_fmt(const Options *opt, Arena *arena, Diag *diag) {
  char *text = NULL;
  size_t len = 0;
  if (!read_source(arena, opt->prog_path, &text, &len, diag)) return 2;
  TokenList toks;
  DeclList decls = {NULL, 0, 0};
  if (!lex_source(arena, opt->prog_path, text, len, &toks, diag)) return 1;
  if (!parse_source(arena, opt->prog_path, ORIGIN_PROGRAM, &toks, &decls, diag)) return 1;
  FmtText out;
  fmt_text_init(&out, arena);
  if (!fmt_program(&out, decls.items, decls.count, &toks)) {
    return diag_fail(diag, "OOM", NULL, "no memory for the canonical text of %s", opt->prog_path) + 2;
  }
  size_t written = out.len == 0 ? 0 : fwrite(out.data, 1, out.len, stdout);
  if (written != out.len || fflush(stdout) != 0) return diag_fail(diag, "IO", NULL, "cannot write the canonical text") + 2;
  return 0;
}

int main(int argc, char **argv) {
  Diag diag;
  diag_init(&diag);
  Options opt;
  if (!parse_options(argc, argv, &opt, &diag)) {
    diag_print(&diag, stderr);
    return usage(stderr);
  }
  Arena arena;
  arena_init(&arena, ARENA_LIMIT_BYTES);
  int status = opt.command == CMD_FMT     ? run_fmt(&opt, &arena, &diag)
               : opt.command == CMD_CHECK ? run_check(&opt, &arena, &diag)
               : opt.command == CMD_SIM   ? run_sim(&opt, &arena, &diag)
               : opt.command == CMD_BUILD ? run_build(&opt, &arena, &diag)
                                          : run(&opt, &arena, &diag);
  arena_release(&arena);
  /* run_check prints its own diagnostics. */
  if (status != 0 && diag.set && opt.command != CMD_CHECK) diag_print(&diag, stderr);
  return status;
}
