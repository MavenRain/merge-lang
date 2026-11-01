/* The Wasm merge engine against the guest values (S7 design sec 6). For each
   form: the module, one call script for test/run-wasm.mjs with a visit of
   each schedule and each refused instance of the sim search, and the want
   lines from the guest values. `build/engine DIR` writes DIR/F.wasm,
   DIR/F.calls and DIR/F.want; `build/engine -c DIR` compares each DIR/F.want
   with DIR/F.got (the node output). A want `*` is a nonzero numeral (a
   pointer). */
#include "engine.h"
#include "front/check.h"
#include "front/front.h"
#include "target.h"
#include <stdio.h>
#include <string.h>

#define WORDS_MAX 64u
#define LINE_MAX_BYTES 256u
#define SOURCE_BYTES 65536u
#define SEGS_MAX 8u
#define WORD_LIMIT 0x8000000000000000ull /* ENGINE_WORD_LIMIT of src/engine.c */

typedef struct {
  const char *stem;
  const char *path;
  uint32_t ops;
} Fixture;

static const Fixture FIXTURES[] = {
    {"tags", "examples/tags.merge", 2u},
    {"owner", "examples/owner.merge", 2u},
    {"inventory", "examples/inventory.merge", 2u},
    {"inventory-escrow", "examples/inventory-escrow.merge", 2u},
    {"classes", "test/report/classes.merge", 2u},
    {"arguments", "test/engine/arguments.merge", 1u},
    {"large-pack", "test/engine/large-pack.merge", 0u},
};
#define FIXTURE_COUNT (sizeof FIXTURES / sizeof FIXTURES[0])

typedef struct {
  Machine *m;
  const EngineForm *f;
  FILE *calls;
  FILE *want;
  uint64_t line; /* the call lines of the script */
  uint64_t schedules;
  uint64_t refusals;
  size_t words; /* L */
  Arena scratch; /* the values of one visit */
  int failed;
} Visit;

/* The guest member J of a Fin type: pack j p. */
static const Value *member(Machine *m, const EngineForm *f, uint64_t j) {
  return val_make(m, OP_PACK, 0, 2u, val_nat(m, j), f->search.proof, NULL);
}

/* The word of the guest leaf V: a numeral, or 1 for true and 0 for false. */
static int leaf_word(const Value *v, uint64_t *out) {
  if (v == NULL) return 0;
  if (v->kind == VAL_NAT) {
    *out = v->nat;
    return 1;
  }
  if (!val_is(v, OP_TRUE) && !val_is(v, OP_FALSE)) return 0;
  *out = val_is(v, OP_TRUE) ? 1u : 0u;
  return 1;
}

/* The words of the guest function V on Fin D0 (then on Fin D1 when D1 > 0),
   row major, to OUT from *AT. */
static int fn_words(Machine *m, const EngineForm *f, const Value *v, uint64_t d0, uint64_t d1, uint64_t *out,
                    size_t *at) {
  for (uint64_t a = 0; a < d0; a++) {
    const Value *row = apply_value(m, v, member(m, f, a));
    for (uint64_t b = 0; b < (d1 > 0 ? d1 : 1u); b++) {
      const Value *leaf = d1 > 0 ? apply_value(m, row, member(m, f, b)) : row;
      if (*at >= WORDS_MAX || !leaf_word(leaf, &out[*at])) return 0;
      (*at)++;
    }
  }
  return 1;
}

/* The part J of the K parts of the state S: the right-nested pack chain. */
static const Value *part_of(const Value *s, size_t k, size_t j) {
  for (size_t a = 0; a < j && s != NULL; a++) s = val_is(s, OP_PACK) && s->argc == 2u ? s->args[1] : NULL;
  return j + 1u == k || s == NULL ? s : val_is(s, OP_PACK) && s->argc == 2u ? s->args[0] : NULL;
}

/* The engine layout of the guest state S: per part, the witness words, then
   the payload words. */
static int state_words(Machine *m, const EngineForm *f, const Value *s, uint64_t *out, size_t *count) {
  size_t at = 0;
  int ok = 1;
  for (size_t j = 0; j < f->part_count && ok; j++) {
    const EnginePart *p = &f->parts[j];
    const Value *v = part_of(s, f->part_count, j);
    if (v == NULL || (p->kind != ENGINE_GSET && (!val_is(v, OP_PACK) || v->argc != 2u))) return 0;
    switch (p->kind) {
      case ENGINE_GSET:
        ok = fn_words(m, f, v, p->n, 0, out, &at);
        break;
      case ENGINE_MVREG:
        ok = fn_words(m, f, v->args[0], f->r, p->n, out, &at) && fn_words(m, f, v->args[1], f->r, 0, out, &at);
        break;
      case ENGINE_COUNTER:
        ok = fn_words(m, f, v->args[0], f->r, 0, out, &at) && fn_words(m, f, v->args[1], f->r, 0, out, &at);
        break;
      case ENGINE_ESCROW:
        ok = fn_words(m, f, v->args[0], f->r, f->r, out, &at) && fn_words(m, f, v->args[1], f->r, 0, out, &at);
        break;
    }
  }
  *count = at;
  return ok;
}

/* The words of the part P on R replicas in the engine layout. */
static uint64_t part_width(const EnginePart *p, uint64_t r) {
  switch (p->kind) {
    case ENGINE_GSET:
      return p->n;
    case ENGINE_MVREG:
      return r * p->n + r;
    case ENGINE_COUNTER:
      return 2u * r;
    case ENGINE_ESCROW:
      return r * r + r;
  }
  return 0;
}

/* Writes a call line and its want line; returns the call line number. */
static uint64_t call(Visit *x, const char *want, const char *fmt, unsigned long long a, unsigned long long b,
                     unsigned long long c, unsigned long long d) {
  fprintf(x->calls, fmt, x->f->name, a, b, c, d);
  fprintf(x->want, "%s\n", want);
  return ++x->line;
}

/* The words line of the state at the call line AT, with the guest state S. */
static int words(Visit *x, uint64_t at, const Value *s) {
  uint64_t w[WORDS_MAX];
  size_t n = 0;
  if (!state_words(x->m, x->f, s, w, &n) || n != x->words) return 0;
  fprintf(x->calls, "words @%llu %zu\n", (unsigned long long)at, n);
  for (size_t i = 0; i < n; i++) fprintf(x->want, "%llu\n", (unsigned long long)w[i]);
  x->line++;
  return 1;
}

/* A word 0 that the decode check of part 0 refuses, or 0 when the part has
   no such check. Part 0 is at word 0; MVReg m 0 0 must be 2 ctx 0 (+ 1). */
static uint64_t bad_word(const EngineForm *f, const uint64_t *w) {
  const EnginePart *p = &f->parts[0];
  switch (p->kind) {
  case ENGINE_GSET:
    return 2u;
  case ENGINE_MVREG:
    return 2u * w[f->r * p->n] + 2u;
  case ENGINE_COUNTER:
  case ENGINE_ESCROW:
    return 0;
  }
  return 0;
}

/* A `store @ENC FIRST w1 ...` line (the words of W, with word 0 FIRST) and
   the decode of that block, want 0. */
static void store_decode(Visit *x, uint64_t enc, const uint64_t *w, size_t n, uint64_t first) {
  fprintf(x->calls, "store @%llu %llu", (unsigned long long)enc, (unsigned long long)first);
  for (size_t i = 1; i < n; i++) fprintf(x->calls, " %llu", (unsigned long long)w[i]);
  fprintf(x->calls, "\n");
  fprintf(x->want, "*\n");
  call(x, "0", "%s.decode @%llu\n", ++x->line, 0, 0, 0);
}

/* The decode lines of a form, in its first schedule: ENC is the encode line
   of the merged state S, AT the merge line. The canonical block decodes to
   S. A state block (word 0 is not L), a block with a word at WORD_LIMIT, and
   a block with a bad part 0 word decode to 0 (Q76 (a)). */
static int decode_lines(Visit *x, uint64_t enc, uint64_t at, const Value *s) {
  uint64_t w[WORDS_MAX];
  size_t n = 0;
  uint64_t bad;
  if (!state_words(x->m, x->f, s, w, &n) || n != x->words || n == 0) return 0;
  if (!words(x, call(x, "*", "%s.decode @%llu\n", enc, 0, 0, 0), s)) return 0;
  if (w[0] != n) call(x, "0", "%s.decode @%llu\n", at, 0, 0, 0);
  store_decode(x, enc, w, n, WORD_LIMIT);
  bad = bad_word(x->f, w);
  if (bad != 0) store_decode(x, enc, w, n, bad);
  return 1;
}

/* A schedule (MERGED set) or a refused instance (the last step, MERGED
   NULL). */
static int visit_body(void *ctx, const SearchStep *steps, uint32_t count, const Value *merged) {
  Visit *x = ctx;
  const SearchForm *sf = &x->f->search;
  uint64_t last[SEGS_MAX] = {0};
  uint64_t bot, at, enc, ends[2];
  uint32_t i;
  fprintf(x->calls, "reset\n");
  fprintf(x->want, "0\n");
  x->line++;
  bot = call(x, "0", "%s.bot\n", 0, 0, 0, 0);
  for (i = 0; i < count; i++) {
    const SearchStep *st = &steps[i];
    int refused = merged == NULL && i + 1u == count;
    uint64_t arg = 0, before;
    if (st->seg >= SEGS_MAX || st->op >= sf->op_count || (st->after == NULL) != refused) break;
    for (uint32_t a = sf->arity[st->op]; a > 0; a--) arg = st->args[a - 1u] + sf->sizes[st->op][a - 1u] * arg;
    before = last[st->seg] != 0 ? last[st->seg] : st->seg > 0 && last[0] != 0 ? last[0] : bot;
    at = call(x, refused ? "0" : "*", "%s.apply @%llu %llu %llu %llu\n", before, st->seg == 0 ? 0 : st->seg - 1u,
              st->op, arg);
    last[st->seg] = at;
    if (merged != NULL && !words(x, call(x, "*", "%s.encode @%llu\n", at, 0, 0, 0), st->after)) break;
  }
  if (i < count) {
    x->failed = 1;
    return 0;
  }
  if (merged == NULL) {
    x->refusals++;
    return 1;
  }
  for (uint32_t r = 0; r < 2u; r++) ends[r] = last[r + 1u] != 0 ? last[r + 1u] : last[0] != 0 ? last[0] : bot;
  at = call(x, "*", "%s.merge @%llu @%llu\n", ends[0], ends[1], 0, 0);
  enc = call(x, "*", "%s.encode @%llu\n", at, 0, 0, 0);
  if (!words(x, enc, merged)) {
    x->failed = 1;
    return 0;
  }
  if (sf->inv == SEARCH_NO_INV) {
    call(x, "0", "%s.check @%llu\n", at, 0, 0, 0);
  } else {
    const Value *h = apply_value(x->m, def_value(x->m, sf->inv), merged);
    if (h == NULL) {
      x->failed = 1;
      return 0;
    }
    call(x, val_is(h, OP_TRUE) ? "0" : "1", "%s.check @%llu\n", at, 0, 0, 0);
  }
  if (x->schedules == 0 && !decode_lines(x, enc, at, merged)) {
    x->failed = 1;
    return 0;
  }
  x->schedules++;
  return 1;
}

/* The visitor of search_visit. A visit runs on its own EVAL_FUEL_STEPS, so
   the search keeps the fuel of langc sim. A visit also runs on the scratch
   arena, which is released after each visit: gen_form fills each def_value
   cache first, so no cache of the Machine points into it. */
static int visit(void *ctx, const SearchStep *steps, uint32_t count, const Value *merged) {
  Visit *x = ctx;
  uint64_t fuel = x->m->fuel;
  Arena *arena = x->m->arena;
  int ok;
  x->m->fuel = EVAL_FUEL_STEPS;
  x->m->arena = &x->scratch;
  ok = visit_body(ctx, steps, count, merged);
  arena_release(&x->scratch);
  x->m->arena = arena;
  x->m->fuel = fuel;
  return ok;
}

static FILE *open_in(const char *dir, const char *stem, const char *ext, const char *mode) {
  char path[1024];
  int n = snprintf(path, sizeof path, "%s/%s.%s", dir, stem, ext);
  FILE *f = n > 0 && (size_t)n < sizeof path ? fopen(path, mode) : NULL;
  if (f == NULL) fprintf(stderr, "FAIL engine: cannot open %s/%s.%s\n", dir, stem, ext);
  return f;
}

/* Exercise the last valid pack without enumerating its 4096 members.
   The guest evaluates the same op; invalid packs, replicas and ops trap. */
static int last_pack(Visit *x) {
  const SearchForm *sf = &x->f->search;
  SearchStep st = {1u, 0u, {0}, NULL};
  const Value *v = apply_value(x->m, def_value(x->m, sf->words[0]), member(x->m, x->f, 0));
  uint64_t packs = 1, bot;
  for (uint32_t a = 0; a < sf->arity[0]; a++) {
    st.args[a] = sf->sizes[0][a] - 1u;
    packs *= sf->sizes[0][a];
    v = apply_value(x->m, v, member(x->m, x->f, st.args[a]));
  }
  v = apply_value(x->m, v, def_value(x->m, sf->bot));
  if (v == NULL || !val_is(v, OP_SOME) || v->argc != 1u) return 0;
  st.after = v->args[0];
  if (!visit(x, &st, 1u, st.after)) return 0;
  bot = call(x, "*", "%s.bot\n", 0, 0, 0, 0);
  call(x, "trap", "%s.apply @%llu 0 0 %llu\n", bot, packs, 0, 0);
  call(x, "trap", "%s.apply @%llu %llu 0 0\n", bot, x->f->r, 0, 0);
  call(x, "trap", "%s.apply @%llu 0 %llu 0\n", bot, x->f->op_count, 0, 0);
  return 1;
}

/* The module, the call script and the want lines of one fixture. */
static int gen_form(const Fixture *fx, const char *dir, uint64_t *s, uint64_t *r, uint64_t *c) {
  static char text[SOURCE_BYTES];
  Arena arena;
  Diag diag;
  Front front;
  Machine machine;
  EngineForm form;
  IrProgram prog;
  SearchOutcome out;
  Visit x;
  FILE *src = fopen(fx->path, "rb"), *wasm = NULL;
  size_t len = src == NULL ? 0 : fread(text, 1, sizeof text, src), i, width = 0;
  int ok = 0;
  memset(&x, 0, sizeof x);
  memset(&prog, 0, sizeof prog);
  arena_init(&arena, (size_t)1 << 30);
  arena_init(&x.scratch, (size_t)1 << 26);
  diag_init(&diag);
  if (src == NULL || ferror(src) || len == sizeof text) goto done;
  if (!front_load(&arena, fx->path, text, len, &front, &diag) ||
      !check_program_sim(&arena, &front.decls, &machine, &diag)) goto done;
  for (i = 0; i < front.decls.count && front.decls.items[i].kind != DECL_REPLICATED; i++) {}
  if (i == front.decls.count || !engine_form(&machine, &front.decls, i, &form)) goto done;
  if (form.r != 2u || form.inv_count > 1u) {
    fprintf(stderr, "FAIL engine %s: INTERNAL: %llu replicas, %zu invariants\n", fx->stem,
            (unsigned long long)form.r, form.inv_count);
    goto done;
  }
  for (size_t j = 0; j < form.part_count; j++) width += part_width(&form.parts[j], form.r);
  wasm = open_in(dir, fx->stem, "wasm", "wb");
  if (wasm == NULL || !engine_program(&machine, &front.decls, &prog, stderr) ||
      !target_write(&prog, TARGET_PART_MAIN, wasm, stderr)) goto done;
  x.m = &machine;
  x.f = &form;
  x.words = width;
  x.calls = open_in(dir, fx->stem, "calls", "w");
  x.want = open_in(dir, fx->stem, "want", "w");
  if (x.calls == NULL || x.want == NULL) goto done;
  for (uint32_t d = 0; d < machine.def_count; d++)
    if (def_value(&machine, d) == NULL) goto done;
  {
    SearchBounds b = {fx->ops == 0 ? 0u : 1u, fx->ops, 2u};
    if (!search_visit(&machine, &form.search, b, visit, &x, &out) || x.failed) goto done;
  }
  if (out.result == SEARCH_ABOVE_CAP || x.schedules != out.kept + (out.result == SEARCH_VIOLATION ? 1u : 0u)) {
    fprintf(stderr, "FAIL engine %s: %llu schedules for K %llu\n", fx->stem, (unsigned long long)x.schedules,
            (unsigned long long)out.kept);
    goto done;
  }
  if (fx->ops == 0 && !last_pack(&x)) goto done;
  *s += x.schedules;
  *r += x.refusals;
  *c += x.line;
  ok = 1;
done:
  if (src != NULL) fclose(src);
  if (wasm != NULL && fclose(wasm) != 0) ok = 0;
  if (x.calls != NULL && fclose(x.calls) != 0) ok = 0;
  if (x.want != NULL && fclose(x.want) != 0) ok = 0;
  if (!ok) {
    fprintf(stderr, "FAIL engine %s\n", fx->stem);
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&x.scratch);
  arena_release(&arena);
  return ok;
}

/* Reads one line of F to BUF without the newline. Returns 0 at the end. */
static int read_line(FILE *f, char *buf) {
  size_t n;
  if (fgets(buf, LINE_MAX_BYTES, f) == NULL) return 0;
  n = strlen(buf);
  if (n > 0 && buf[n - 1u] == '\n') buf[n - 1u] = '\0';
  return 1;
}

static int matches(const char *want, const char *got) {
  if (strcmp(want, "*") != 0) return strcmp(want, got) == 0;
  return got[0] >= '1' && got[0] <= '9' && strspn(got, "0123456789") == strlen(got);
}

/* Compares DIR/F.want with DIR/F.got, and counts the schedules (merge
   lines), the refusals (the other reset lines) and the calls of DIR/F.calls. */
static int check_form(const Fixture *fx, const char *dir, uint64_t *s, uint64_t *r, uint64_t *c, uint64_t *mis) {
  char a[LINE_MAX_BYTES], b[LINE_MAX_BYTES];
  FILE *calls = open_in(dir, fx->stem, "calls", "r"), *want = open_in(dir, fx->stem, "want", "r"),
       *got = open_in(dir, fx->stem, "got", "r");
  uint64_t resets = 0, merges = 0, line = 0, shown = 0;
  int ok = calls != NULL && want != NULL && got != NULL;
  while (ok && read_line(calls, a)) {
    (*c)++;
    resets += strcmp(a, "reset") == 0;
    merges += strstr(a, ".merge ") != NULL;
  }
  while (ok) {
    int has_a = read_line(want, a), has_b = read_line(got, b);
    if (!has_a && !has_b) break;
    line++;
    if (has_a && has_b && matches(a, b)) continue;
    (*mis)++;
    if (shown++ < 5u)
      fprintf(stderr, "FAIL engine %s line %llu: want %s, got %s\n", fx->stem, (unsigned long long)line,
              has_a ? a : "(end)", has_b ? b : "(end)");
  }
  *s += merges;
  *r += resets - merges;
  if (calls != NULL) fclose(calls);
  if (want != NULL) fclose(want);
  if (got != NULL) fclose(got);
  return ok;
}

int main(int argc, char **argv) {
  int compare = argc == 3 && strcmp(argv[1], "-c") == 0, ok = 1;
  uint64_t s = 0, r = 0, c = 0, mis = 0;
  if (argc != 2 && !compare) {
    fprintf(stderr, "usage: build/engine DIR | build/engine -c DIR\n");
    return 2;
  }
  for (size_t i = 0; i < FIXTURE_COUNT; i++)
    ok = (compare ? check_form(&FIXTURES[i], argv[argc - 1], &s, &r, &c, &mis)
                  : gen_form(&FIXTURES[i], argv[argc - 1], &s, &r, &c)) && ok;
  printf("engine: %zu forms, %llu schedules, %llu refusals, %llu calls", FIXTURE_COUNT, (unsigned long long)s,
         (unsigned long long)r, (unsigned long long)c);
  if (compare) printf(", %llu mismatches", (unsigned long long)mis);
  printf("\n");
  return ok && mis == 0 ? 0 : 1;
}
