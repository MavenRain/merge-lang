/* The bounded search core (S4 design sec 2). */
#include "front/search.h"

#include <string.h>

typedef struct {
  Machine *m;
  const SearchForm *f;
  SearchBounds b;
  uint32_t o1;
  uint32_t o2;
  const Value *bot;
  SearchStep *path;
  uint32_t total; /* the instance count of this round */
  SearchOutcome *out;
  int stop;   /* after a violation, the cap or a failure */
  int failed; /* after a diagnostic */
} Search;

static int internal(Search *s, const char *what) {
  s->failed = 1;
  s->stop = 1;
  return diag_fail(s->m->diag, "INTERNAL", s->m->def, "search: %s", what);
}

/* The member J of a Fin type: pack J p. */
static const Value *member(Search *s, uint32_t j) {
  return val_make(s->m, OP_PACK, 0, 2u, val_nat(s->m, j), s->f->proof, NULL);
}

/* The state after instance ST on state IN, or NULL when the
   precondition fails (none) or after a failure (S->failed). */
static const Value *step(Search *s, const SearchStep *st, const Value *in) {
  const Value *v = def_value(s->m, s->f->words[st->op]);
  uint32_t j;
  v = v == NULL ? NULL : apply_value(s->m, v, member(s, st->seg == 0u ? 0u : st->seg - 1u));
  for (j = 0; v != NULL && j < s->f->arity[st->op]; j++) v = apply_value(s->m, v, member(s, st->args[j]));
  v = v == NULL ? NULL : apply_value(s->m, v, in);
  if (v == NULL) {
    s->failed = 1;
    s->stop = 1;
    return NULL;
  }
  if (val_is(v, OP_SOME))
    return v->args[0];
  if (!val_is(v, OP_NONE))
    internal(s, "an op did not compute an Option");
  return NULL;
}

/* 1 when NAMEInv is true at V, 0 when it is false, -1 after a failure. */
static int holds(Search *s, const Value *v) {
  const Value *r = apply_value(s->m, def_value(s->m, s->f->inv), v);
  if (r == NULL) {
    s->failed = 1;
    s->stop = 1;
    return -1;
  }
  if (val_is(r, OP_TRUE) || val_is(r, OP_FALSE))
    return val_is(r, OP_TRUE);
  internal(s, "the invariant did not compute a Bool");
  return -1;
}

/* The state before an instance in segment SEG at position D: the state
   after the instance at D - 1 when it is in SEG, else the ancestor (the
   state after the last prefix instance, else NAMEBot). */
static const Value *before(const Search *s, uint32_t d, uint32_t seg) {
  uint32_t j;
  if (d > 0u && s->path[d - 1u].seg == seg)
    return s->path[d - 1u].after;
  for (j = d; j > 0u && s->path[j - 1u].seg != 0u; j--) {}
  return j > 0u ? s->path[j - 1u].after : s->bot;
}

/* One more visited schedule (a leaf or a dropped instance). */
static void count_visit(Search *s) {
  if (++s->out->visits > SEARCH_CAP) {
    s->out->result = SEARCH_ABOVE_CAP;
    s->stop = 1;
  }
}

/* A complete schedule: keep it when O1 is in one branch and O2 is in a
   different branch, then merge the N branch states in replica order. */
static void leaf(Search *s) {
  uint32_t i, j, r;
  int pair = 0;
  const Value *merged, *ancestor = before(s, s->total, 0u);
  count_visit(s);
  for (i = 0; i < s->total && !pair; i++) {
    for (j = 0; j < s->total && !pair; j++) {
      const SearchStep *a = &s->path[i], *b = &s->path[j];
      pair = a->seg > 0u && b->seg > 0u && a->seg != b->seg && a->op == s->o1 && b->op == s->o2;
    }
  }
  if (!pair || s->stop)
    return;
  merged = NULL;
  for (r = 1; r <= s->b.replicas; r++) {
    const Value *last = ancestor;
    for (j = 0; j < s->total; j++) last = s->path[j].seg == r ? s->path[j].after : last;
    merged = merged == NULL ? last : apply_value(s->m, apply_value(s->m, def_value(s->m, s->f->merge), merged), last);
    if (merged == NULL) {
      s->failed = 1;
      s->stop = 1;
      return;
    }
  }
  switch (holds(s, merged)) {
  case 1:
    s->out->kept++;
    return;
  case 0:
    s->out->result = SEARCH_VIOLATION;
    s->out->merged = merged;
    s->out->step_count = s->total;
    s->out->steps = arena_alloc(s->m->arena, (s->total + 1u) * sizeof *s->out->steps);
    if (s->out->steps == NULL) {
      s->failed = 1;
      diag_fail(s->m->diag, "OOM", s->m->def, "out of memory");
    } else {
      memcpy((SearchStep *)s->out->steps, s->path, s->total * sizeof *s->path);
    }
    s->stop = 1;
    return;
  default:
    return;
  }
}

static void dfs(Search *s, uint32_t d, uint32_t count);

/* The next argument tuple of ST (the first argument is the most
   significant digit); 0 after the last tuple. */
static int next_args(const Search *s, SearchStep *st) {
  uint32_t k = s->f->arity[st->op];
  while (k > 0u) {
    k--;
    if (++st->args[k] < s->f->sizes[st->op][k])
      return 1;
    st->args[k] = 0;
  }
  return 0;
}

/* Each instance at position D in segment SEG, which has COUNT instances
   before D, in the 5.2 order: the op, then the arguments. A branch
   instance is an instance of O1 or O2. A dropped instance (none, or a
   state that is not I-valid) ends its schedule. */
static void place(Search *s, uint32_t d, uint32_t seg, uint32_t count) {
  SearchStep *st = &s->path[d];
  uint32_t o, j;
  if (count >= (seg == 0u ? s->b.prefix : s->b.ops))
    return;
  for (o = 0; o < s->f->op_count && !s->stop; o++) {
    int more = seg == 0u || o == s->o1 || o == s->o2;
    st->seg = seg;
    st->op = o;
    memset(st->args, 0, sizeof st->args);
    for (j = 0; j < s->f->arity[o]; j++) more = more && s->f->sizes[o][j] > 0u;
    while (more && !s->stop) {
      const Value *after = step(s, st, before(s, d, seg));
      int ok = after == NULL ? 0 : holds(s, after);
      if (s->stop)
        return;
      if (ok == 1) {
        st->after = after;
        dfs(s, d + 1u, count + 1u);
      } else {
        count_visit(s);
      }
      more = next_args(s, st);
    }
  }
}

/* Extends the path at position D; COUNT is the instance count of the
   segment of position D - 1. The segment stays or grows. */
static void dfs(Search *s, uint32_t d, uint32_t count) {
  uint32_t seg = d == 0u ? 0u : s->path[d - 1u].seg, next;
  if (d == s->total) {
    leaf(s);
    return;
  }
  place(s, d, seg, d == 0u ? 0u : count);
  for (next = seg + 1u; next <= s->b.replicas && !s->stop; next++) place(s, d, next, 0u);
}

int search_pair(Machine *m, const SearchForm *f, SearchBounds b, uint32_t o1, uint32_t o2, SearchOutcome *out) {
  Search s;
  uint32_t max = b.prefix + b.ops * b.replicas, j;
  memset(out, 0, sizeof *out);
  memset(&s, 0, sizeof s);
  s.m = m;
  s.f = f;
  s.b = b;
  s.o1 = o1;
  s.o2 = o2;
  s.out = out;
  out->result = SEARCH_KEPT;
  for (j = 0; j < f->op_count; j++) {
    if (f->arity[j] > SEARCH_ARGS_MAX)
      return internal(&s, "an op has more than 8 parameters");
  }
  s.path = arena_alloc(m->arena, (max + 1u) * sizeof *s.path);
  if (s.path == NULL)
    return diag_fail(m->diag, "OOM", m->def, "out of memory");
  s.bot = def_value(m, f->bot);
  if (s.bot == NULL)
    return 0;
  /* An ancestor that is not I-valid gives no schedule (K = 0). */
  if (holds(&s, s.bot) != 1)
    return !s.failed;
  for (s.total = 0; s.total <= max && !s.stop; s.total++) dfs(&s, 0u, 0u);
  return !s.failed;
}
