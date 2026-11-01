/* The Wasm merge engine (S7 design sec 2 to 5). The sizes n and r are known
   at build, so each template is unrolled: one ALLOC of the L layout words.
   The replica i is a runtime value: a row word is a select over the r
   choices. Each counter word and each sum traps at 2^63 (Q71 (a)). */
#include "engine.h"

#include <string.h>

#define ENGINE_WORD_LIMIT 0x8000000000000000ull
#define ENGINE_WORDS_MAX 65536u
#define ENGINE_INV_MAX 63u

/* The build state of one entry. OK goes to 0 when the arena is full. */
typedef struct {
  Arena *arena;
  uint32_t locals;
  int ok;
} Fn;

/* A statement list under construction. */
typedef struct {
  const IrStmt **items;
  size_t count;
  size_t cap;
} Stmts;

/* The terms of a checked sum. */
typedef struct {
  const IrExpr **items;
  size_t count;
} Terms;

/* The form and the first word of each part. */
typedef struct {
  const EngineForm *form;
  const uint32_t *offs;
  uint32_t len;
} Layout;

static const IrExpr expr_none = {IR_EXPR_CONST, 0, 0, IR_OP_ADD, 0, NULL, NULL};
static const IrScalar engine_params[4] = {IR_SCALAR_NAT, IR_SCALAR_NAT, IR_SCALAR_NAT, IR_SCALAR_NAT};

static int internal(FILE *err, const EngineForm *form, const char *what) {
  fprintf(err, "langc: INTERNAL: %s: %s\n", form->name, what);
  return 0;
}

static void *fn_alloc(Fn *f, size_t size) {
  void *p = arena_alloc(f->arena, size);
  if (p == NULL) f->ok = 0;
  return p;
}

static uint32_t fn_local(Fn *f) { return f->locals++; }

static const IrExpr *expr_new(Fn *f, IrExprKind kind, uint64_t value, uint32_t local, IrOp op, uint32_t field,
                              const IrExpr *left, const IrExpr *right) {
  IrExpr *e = fn_alloc(f, sizeof *e);
  if (e == NULL) return &expr_none;
  e->kind = kind;
  e->value = value;
  e->local = local;
  e->op = op;
  e->field = field;
  e->left = left;
  e->right = right;
  return e;
}

static const IrExpr *ex_const(Fn *f, uint64_t value) {
  return expr_new(f, IR_EXPR_CONST, value, 0, IR_OP_ADD, 0, NULL, NULL);
}

static const IrExpr *ex_local(Fn *f, uint32_t local) {
  return expr_new(f, IR_EXPR_LOCAL, 0, local, IR_OP_ADD, 0, NULL, NULL);
}

static const IrExpr *ex_bin(Fn *f, IrOp op, const IrExpr *left, const IrExpr *right) {
  return expr_new(f, IR_EXPR_BINARY, 0, 0, op, 0, left, right);
}

/* Word K of the block at the address in LOCAL. */
static const IrExpr *ex_word(Fn *f, uint32_t local, uint32_t k) {
  return expr_new(f, IR_EXPR_LOAD, 0, 0, IR_OP_ADD, k, ex_local(f, local), NULL);
}

static const IrExpr *ex_eq(Fn *f, const IrExpr *e, uint64_t v) { return ex_bin(f, IR_OP_EQ, e, ex_const(f, v)); }

static const IrExpr *ex_not(Fn *f, const IrExpr *flag) { return ex_eq(f, flag, 0); }

/* natMax a b = (a - b) + b, since SUB truncates at 0. */
static const IrExpr *ex_max(Fn *f, const IrExpr *a, const IrExpr *b) {
  return ex_bin(f, IR_OP_ADD, ex_bin(f, IR_OP_SUB, a, b), b);
}

/* select: K at I = J, else 0. */
static const IrExpr *ex_select(Fn *f, const IrExpr *i, uint64_t j, const IrExpr *k) {
  return ex_bin(f, IR_OP_MUL, ex_eq(f, i, j), k);
}

static IrStmt *stmt_new(Fn *f, IrStmtKind kind) {
  IrStmt *s = fn_alloc(f, sizeof *s);
  if (s != NULL) {
    memset(s, 0, sizeof *s);
    s->kind = kind;
  }
  return s;
}

static void push(Fn *f, Stmts *list, const IrStmt *s) {
  if (s == NULL) return;
  if (list->count == list->cap) {
    size_t cap = list->cap == 0 ? 16 : list->cap * 2;
    const IrStmt **items = fn_alloc(f, cap * sizeof *items);
    if (items == NULL) return;
    if (list->count > 0) memcpy(items, list->items, list->count * sizeof *items);
    list->items = items;
    list->cap = cap;
  }
  list->items[list->count++] = s;
}

static IrBlock block_of(const Stmts *list) {
  IrBlock b;
  b.items = list->items;
  b.count = list->count;
  return b;
}

static void st_set(Fn *f, Stmts *list, uint32_t local, const IrExpr *e) {
  IrStmt *s = stmt_new(f, IR_STMT_SET);
  if (s == NULL) return;
  s->local = local;
  s->expr = e;
  push(f, list, s);
}

static void st_return(Fn *f, Stmts *list, const IrExpr *e) {
  IrStmt *s = stmt_new(f, IR_STMT_RETURN);
  if (s == NULL) return;
  s->expr = e;
  push(f, list, s);
}

/* IF COND: the one statement ONLY, else nothing. */
static void st_if(Fn *f, Stmts *list, const IrExpr *cond, const IrStmt *only) {
  IrStmt *s = stmt_new(f, IR_STMT_IF);
  const IrStmt **items = fn_alloc(f, sizeof *items);
  if (s == NULL || items == NULL || only == NULL) return;
  items[0] = only;
  s->expr = cond;
  s->body.items = items;
  s->body.count = 1;
  push(f, list, s);
}

static void st_trap_if(Fn *f, Stmts *list, const IrExpr *cond) { st_if(f, list, cond, stmt_new(f, IR_STMT_TRAP)); }

/* Returns 0 (a failed precondition, or a decode refusal) unless COND. */
static void st_require(Fn *f, Stmts *list, const IrExpr *cond) {
  IrStmt *r = stmt_new(f, IR_STMT_RETURN);
  if (r == NULL) return;
  r->expr = ex_const(f, 0);
  st_if(f, list, ex_not(f, cond), r);
}

/* LOCAL := a new block of the N words FIELDS. */
static void st_alloc(Fn *f, Stmts *list, uint32_t local, const IrExpr *const *fields, size_t n) {
  IrStmt *s = stmt_new(f, IR_STMT_ALLOC);
  if (s == NULL) return;
  s->local = local;
  s->fields = fields;
  s->field_count = n;
  push(f, list, s);
}

/* A + B, after a trap when the sum is at or above 2^63. */
static const IrExpr *add_chk(Fn *f, Stmts *list, const IrExpr *a, const IrExpr *b) {
  const IrExpr *sum = ex_bin(f, IR_OP_ADD, a, b);
  st_trap_if(f, list,
             ex_bin(f, IR_OP_OR, ex_bin(f, IR_OP_ADD_CARRY, a, b), ex_bin(f, IR_OP_LE, ex_const(f, ENGINE_WORD_LIMIT), sum)));
  return sum;
}

static Terms terms_new(Fn *f, size_t cap) {
  Terms t;
  t.items = fn_alloc(f, (cap == 0 ? 1 : cap) * sizeof *t.items);
  t.count = 0;
  return t;
}

static void term(Terms *t, const IrExpr *e) {
  if (t->items != NULL) t->items[t->count++] = e;
}

/* The checked sum of the terms T, in a new local. */
static const IrExpr *sum_chk(Fn *f, Stmts *list, const Terms *t) {
  uint32_t acc = fn_local(f);
  if (t->items == NULL) return &expr_none;
  st_set(f, list, acc, t->count == 0 ? ex_const(f, 0) : t->items[0]);
  for (size_t k = 1; k < t->count; k++) st_set(f, list, acc, add_chk(f, list, ex_local(f, acc), t->items[k]));
  return ex_local(f, acc);
}

/* An array of the N words of the block at the address in LOCAL. */
static const IrExpr **words_of(Fn *f, uint32_t local, uint32_t first, uint32_t n) {
  const IrExpr **fields = fn_alloc(f, (n == 0 ? 1 : n) * sizeof *fields);
  if (fields == NULL) return NULL;
  for (uint32_t k = 0; k < n; k++) fields[k] = ex_word(f, local, first + k);
  return fields;
}

static uint64_t part_len(const EnginePart *p, uint64_t r) {
  switch (p->kind) {
  case ENGINE_GSET:
    return p->n;
  case ENGINE_MVREG:
    return r * p->n + r;
  case ENGINE_COUNTER:
    return 2 * r;
  case ENGINE_ESCROW:
    return r * r + r;
  }
  return 0;
}

/* Counter words: p j at OFF + j, d j at OFF + r + j. nonneg = Σd <= s0 + Σp. */
static const IrExpr *counter_nonneg(Fn *f, Stmts *list, const EnginePart *p, uint32_t off, uint32_t r, uint32_t cur,
                                    const IrExpr *k) {
  Terms lhs = terms_new(f, r + 1);
  Terms rhs = terms_new(f, r + 1);
  for (uint32_t j = 0; j < r; j++) term(&lhs, ex_word(f, cur, off + r + j));
  if (k != NULL) term(&lhs, k);
  term(&rhs, ex_const(f, p->s0));
  for (uint32_t j = 0; j < r; j++) term(&rhs, ex_word(f, cur, off + j));
  return ex_bin(f, IR_OP_LE, sum_chk(f, list, &lhs), sum_chk(f, list, &rhs));
}

/* Escrow words: R a b at OFF + a r + b, U a at OFF + r r + a. The stock
   reads as the guest escrowStock: Σ U <= Σ split + Σ R a a. */
static const IrExpr *escrow_nonneg(Fn *f, Stmts *list, const EnginePart *p, uint32_t off, uint32_t r, uint32_t cur) {
  Terms lhs = terms_new(f, r);
  Terms rhs = terms_new(f, 2 * r);
  for (uint32_t a = 0; a < r; a++) term(&lhs, ex_word(f, cur, off + r * r + a));
  for (uint32_t a = 0; a < r; a++) term(&rhs, ex_const(f, p->split[a]));
  for (uint32_t a = 0; a < r; a++) term(&rhs, ex_word(f, cur, off + a * r + a));
  return ex_bin(f, IR_OP_LE, sum_chk(f, list, &lhs), sum_chk(f, list, &rhs));
}

/* The guest escrowCan at the replica I: (out i + U i) + K <= (split i +
   R i i) + in i, else the op returns 0. */
static void escrow_can(Fn *f, Stmts *list, const EnginePart *p, uint32_t off, uint32_t r, uint32_t cur, const IrExpr *i,
                       const IrExpr *k) {
  Terms lhs = terms_new(f, r * r + 1);
  Terms rhs = terms_new(f, r * r + r);
  for (uint32_t a = 0; a < r; a++)
    for (uint32_t b = 0; b < r; b++)
      if (a != b) term(&lhs, ex_select(f, i, a, ex_word(f, cur, off + a * r + b)));
  for (uint32_t a = 0; a < r; a++) term(&lhs, ex_select(f, i, a, ex_word(f, cur, off + r * r + a)));
  term(&lhs, k);
  for (uint32_t a = 0; a < r; a++) term(&rhs, ex_select(f, i, a, ex_const(f, p->split[a])));
  for (uint32_t a = 0; a < r; a++) term(&rhs, ex_select(f, i, a, ex_word(f, cur, off + a * r + a)));
  for (uint32_t a = 0; a < r; a++)
    for (uint32_t b = 0; b < r; b++)
      if (a != b) term(&rhs, ex_select(f, i, a, ex_word(f, cur, off + b * r + a)));
  st_require(f, list, ex_bin(f, IR_OP_LE, sum_chk(f, list, &lhs), sum_chk(f, list, &rhs)));
}

/* MVReg words: m j y at OFF + j n + y, ctx j at OFF + r n + j. A write of X
   at I: ctx' j = ctx j + [j = i], delta m j y = 2 ctx' j + (1 - [j = i][x =
   y]), so row i gets 2 (ctx i + 1) + (1 - [x = y]) and a row j != i gets
   2 ctx j + 1. The result is natMax of the state and delta. */
static void mvreg_write(Fn *f, Stmts *list, const EnginePart *p, uint32_t off, uint32_t r, uint32_t cur, const IrExpr *i,
                        const IrExpr *x, const IrExpr **fields) {
  uint32_t n = (uint32_t)p->n;
  for (uint32_t j = 0; j < r; j++) {
    uint32_t ctx = fn_local(f);
    uint32_t two = fn_local(f);
    const IrExpr *old = ex_word(f, cur, off + r * n + j);
    st_set(f, list, ctx, add_chk(f, list, old, ex_eq(f, i, j)));
    st_set(f, list, two, add_chk(f, list, ex_local(f, ctx), ex_local(f, ctx)));
    for (uint32_t y = 0; y < n; y++) {
      const IrExpr *live = ex_bin(f, IR_OP_MUL, ex_eq(f, i, j), ex_bin(f, IR_OP_EQ, x, ex_const(f, y)));
      const IrExpr *delta = add_chk(f, list, ex_local(f, two), ex_bin(f, IR_OP_SUB, ex_const(f, 1), live));
      fields[off + j * n + y] = ex_max(f, ex_word(f, cur, off + j * n + y), delta);
    }
    fields[off + r * n + j] = ex_max(f, old, ex_local(f, ctx));
  }
}

/* The word W at F + j, plus the select of K at I = j, for j < R. */
static void bump_row(Fn *f, Stmts *list, uint32_t first, uint32_t step, uint32_t r, uint32_t cur, const IrExpr *i,
                     const IrExpr *k, const IrExpr **fields) {
  for (uint32_t j = 0; j < r; j++) {
    uint32_t w = first + j * step;
    fields[w] = add_chk(f, list, ex_word(f, cur, w), ex_select(f, i, j, k));
  }
}

/* The evaluated argument of a step at the runtime pack, in a new local.
   Separate assignments keep the IR depth independent of the pack size. */
static const IrExpr *argument(Fn *f, Stmts *list, const uint64_t *values, uint64_t count) {
  uint32_t t = fn_local(f);
  st_set(f, list, t, ex_const(f, values[0]));
  for (uint64_t x = 1; x < count; x++) {
    IrStmt *set;
    if (values[x] == values[0]) continue;
    set = stmt_new(f, IR_STMT_SET);
    if (set == NULL) continue;
    set->local = t;
    set->expr = ex_const(f, values[x]);
    st_if(f, list, ex_eq(f, ex_local(f, 3), x), set);
  }
  return ex_local(f, t);
}

/* The new words FIELDS of the step ST on the state in CUR. Returns 1, or 0
   after an INTERNAL line on ERR. */
static int build_step(Fn *f, Stmts *list, const Layout *lay, const EngineStep *st,
                      uint64_t packs, uint32_t cur, const IrExpr **fields, FILE *err) {
  const EngineForm *form = lay->form;
  const EnginePart *p = &form->parts[st->part];
  uint32_t off = lay->offs[st->part];
  uint32_t r = (uint32_t)form->r;
  const IrExpr *i = ex_local(f, 1);
  const IrExpr *x = argument(f, list, st->args, packs);
  switch (st->word) {
  case ENGINE_ADD:
    if (p->kind != ENGINE_GSET) break;
    for (uint32_t y = 0; y < (uint32_t)p->n; y++)
      fields[off + y] = ex_bin(f, IR_OP_OR, ex_word(f, cur, off + y), ex_bin(f, IR_OP_EQ, x, ex_const(f, y)));
    return 1;
  case ENGINE_WRITE:
    if (p->kind != ENGINE_MVREG) break;
    mvreg_write(f, list, p, off, r, cur, i, x, fields);
    return 1;
  case ENGINE_DEC: {
    const IrExpr *k = x;
    if (p->kind == ENGINE_COUNTER) {
      st_require(f, list, counter_nonneg(f, list, p, off, r, cur, k));
      bump_row(f, list, off + r, 1, r, cur, i, k, fields);
      return 1;
    }
    if (p->kind != ENGINE_ESCROW) break;
    escrow_can(f, list, p, off, r, cur, i, k);
    bump_row(f, list, off + r * r, 1, r, cur, i, k, fields);
    return 1;
  }
  case ENGINE_RESTOCK: {
    const IrExpr *k = x;
    if (p->kind == ENGINE_COUNTER) {
      bump_row(f, list, off, 1, r, cur, i, k, fields);
      return 1;
    }
    if (p->kind != ENGINE_ESCROW) break;
    bump_row(f, list, off, r + 1, r, cur, i, k, fields);
    return 1;
  }
  case ENGINE_TRANSFER: {
    const IrExpr *k = x;
    const IrExpr *to = argument(f, list, st->to, packs);
    if (p->kind != ENGINE_ESCROW) break;
    st_require(f, list, ex_not(f, ex_bin(f, IR_OP_EQ, i, to)));
    escrow_can(f, list, p, off, r, cur, i, k);
    for (uint32_t a = 0; a < r; a++)
      for (uint32_t b = 0; b < r; b++) {
        uint32_t w = off + a * r + b;
        if (a != b)
          fields[w] = add_chk(f, list, ex_word(f, cur, w),
                              ex_bin(f, IR_OP_MUL, ex_bin(f, IR_OP_MUL, ex_eq(f, i, a), ex_eq(f, to, b)), k));
      }
    return 1;
  }
  }
  return internal(err, form, "an op step has no engine template");
}

/* The arm of OP in NAME.apply (state 0, replica 1, op 2, arg 3): check the
   pack (Q72 (a)), then each step on a new block. Returns 1, or 0 after an
   INTERNAL line on ERR. */
static int build_op(Fn *f, const Layout *lay, const EngineOp *op, Stmts *arm, FILE *err) {
  const EngineForm *form = lay->form;
  uint64_t prod = 1;
  uint32_t cur = 0;
  for (uint32_t a = 0; a < op->param_count; a++) {
    if (op->sizes[a] == 0 || prod * op->sizes[a] > ENGINE_PACK_MAX) return internal(err, form, "the argument pack of an op is above 4096");
    prod *= op->sizes[a];
  }
  st_trap_if(f, arm, ex_bin(f, IR_OP_LE, ex_const(f, prod), ex_local(f, 3)));
  for (size_t s = 0; s < op->step_count; s++) {
    const EngineStep *st = &op->steps[s];
    const IrExpr **fields = words_of(f, cur, 0, lay->len);
    uint32_t next = fn_local(f);
    if (st->part >= form->part_count || st->args == NULL ||
        (st->word == ENGINE_TRANSFER && st->to == NULL))
      return internal(err, form, "an op step has a bad part or parameter");
    if (fields == NULL) return 1;
    if (!build_step(f, arm, lay, st, prod, cur, fields, err)) return 0;
    st_alloc(f, arm, next, fields, lay->len);
    cur = next;
  }
  st_return(f, arm, ex_local(f, cur));
  return 1;
}

static int build_apply(Fn *f, const Layout *lay, Stmts *body, FILE *err) {
  const EngineForm *form = lay->form;
  IrBlock *arms = fn_alloc(f, (form->op_count == 0 ? 1 : form->op_count) * sizeof *arms);
  IrStmt *sw = stmt_new(f, IR_STMT_SWITCH);
  if (arms == NULL || sw == NULL) return 1;
  st_trap_if(f, body, ex_bin(f, IR_OP_LE, ex_const(f, form->r), ex_local(f, 1)));
  for (size_t o = 0; o < form->op_count; o++) {
    Stmts arm = {NULL, 0, 0};
    if (!build_op(f, lay, &form->ops[o], &arm, err)) return 0;
    arms[o] = block_of(&arm);
  }
  sw->expr = ex_local(f, 2);
  sw->arms = arms;
  sw->arm_count = form->op_count;
  push(f, body, sw);
  push(f, body, stmt_new(f, IR_STMT_TRAP));
  return 1;
}

/* NAME.merge (a 0, b 1): GSet bits by OR, each other word by natMax. */
static void build_merge(Fn *f, const Layout *lay, Stmts *body) {
  const EngineForm *form = lay->form;
  const IrExpr **fields = fn_alloc(f, (lay->len == 0 ? 1 : lay->len) * sizeof *fields);
  uint32_t t = fn_local(f);
  if (fields == NULL) return;
  for (size_t q = 0; q < form->part_count; q++) {
    uint32_t off = lay->offs[q];
    uint32_t len = (uint32_t)part_len(&form->parts[q], form->r);
    int bits = form->parts[q].kind == ENGINE_GSET;
    for (uint32_t k = off; k < off + len; k++)
      fields[k] = bits ? ex_bin(f, IR_OP_OR, ex_word(f, 0, k), ex_word(f, 1, k)) : ex_max(f, ex_word(f, 0, k), ex_word(f, 1, k));
  }
  st_alloc(f, body, t, fields, lay->len);
  st_return(f, body, ex_local(f, t));
}

/* NAME.check (state 0): bit k is set when invariant k is false. */
static int build_check(Fn *f, const Layout *lay, Stmts *body, FILE *err) {
  const EngineForm *form = lay->form;
  uint32_t r = (uint32_t)form->r;
  const IrExpr *bits = ex_const(f, 0);
  for (size_t k = 0; k < form->inv_count; k++) {
    size_t q = form->nonneg[k];
    const IrExpr *inv = &expr_none;
    uint32_t v = fn_local(f);
    if (q >= form->part_count) return internal(err, form, "an invariant has a bad part");
    switch (form->parts[q].kind) {
    case ENGINE_COUNTER:
      inv = counter_nonneg(f, body, &form->parts[q], lay->offs[q], r, 0, NULL);
      break;
    case ENGINE_ESCROW:
      inv = escrow_nonneg(f, body, &form->parts[q], lay->offs[q], r, 0);
      break;
    case ENGINE_GSET:
    case ENGINE_MVREG:
      return internal(err, form, "nonneg is on a part that is not a counter");
    }
    st_set(f, body, v, inv);
    bits = ex_bin(f, IR_OP_ADD, bits, ex_bin(f, IR_OP_MUL, ex_not(f, ex_local(f, v)), ex_const(f, (uint64_t)1 << k)));
  }
  st_return(f, body, bits);
  return 1;
}

/* NAME.encode (state 0): word 0 = L, then the L layout words. */
static void build_encode(Fn *f, const Layout *lay, Stmts *body) {
  const IrExpr **fields = fn_alloc(f, (lay->len + 1) * sizeof *fields);
  uint32_t t = fn_local(f);
  if (fields == NULL) return;
  fields[0] = ex_const(f, lay->len);
  for (uint32_t k = 0; k < lay->len; k++) fields[k + 1] = ex_word(f, 0, k);
  st_alloc(f, body, t, fields, lay->len + 1);
  st_return(f, body, ex_local(f, t));
}

/* NAME.decode (block 0): 0 unless word 0 is L and the layout is canonical
   (Q75 (a)), else a new state. */
static void build_decode(Fn *f, const Layout *lay, Stmts *body) {
  const EngineForm *form = lay->form;
  uint32_t r = (uint32_t)form->r;
  uint32_t t = fn_local(f);
  st_require(f, body, ex_eq(f, ex_word(f, 0, 0), lay->len));
  for (uint32_t k = 0; k < lay->len; k++)
    st_require(f, body, ex_bin(f, IR_OP_LE, ex_word(f, 0, k + 1), ex_const(f, ENGINE_WORD_LIMIT - 1)));
  for (size_t q = 0; q < form->part_count; q++) {
    const EnginePart *p = &form->parts[q];
    uint32_t off = lay->offs[q] + 1;
    uint32_t n = (uint32_t)p->n;
    switch (p->kind) {
    case ENGINE_GSET:
      for (uint32_t y = 0; y < n; y++) st_require(f, body, ex_bin(f, IR_OP_LE, ex_word(f, 0, off + y), ex_const(f, 1)));
      break;
    case ENGINE_MVREG:
      for (uint32_t j = 0; j < r; j++)
        for (uint32_t y = 0; y < n; y++) {
          const IrExpr *m = ex_word(f, 0, off + j * n + y);
          const IrExpr *two = ex_bin(f, IR_OP_ADD, ex_word(f, 0, off + r * n + j), ex_word(f, 0, off + r * n + j));
          st_require(f, body,
                     ex_bin(f, IR_OP_OR, ex_bin(f, IR_OP_EQ, m, two), ex_bin(f, IR_OP_EQ, m, ex_bin(f, IR_OP_ADD, two, ex_const(f, 1)))));
        }
      break;
    case ENGINE_COUNTER:
    case ENGINE_ESCROW:
      break;
    }
  }
  st_alloc(f, body, t, words_of(f, 0, 1, lay->len), lay->len);
  st_return(f, body, ex_local(f, t));
}

/* NAME.bot (): the state of NAMEBot, all L words 0 (the inits are template
   constants). */
static void build_bot(Fn *f, const Layout *lay, Stmts *body) {
  const IrExpr **fields = fn_alloc(f, (lay->len == 0 ? 1 : lay->len) * sizeof *fields);
  uint32_t t = fn_local(f);
  if (fields == NULL) return;
  for (uint32_t k = 0; k < lay->len; k++) fields[k] = ex_const(f, 0);
  st_alloc(f, body, t, fields, lay->len);
  st_return(f, body, ex_local(f, t));
}

static const char *entry_name(Fn *f, const char *form, const char *suffix) {
  size_t a = strlen(form);
  size_t b = strlen(suffix);
  char *s = fn_alloc(f, a + b + 1);
  if (s == NULL) return "";
  memcpy(s, form, a);
  memcpy(s + a, suffix, b + 1);
  return s;
}

int engine_funcs(Arena *arena, const EngineForm *form, IrFunc *out, FILE *err) {
  static const char *const suffix[ENGINE_FUNCS] = {".apply", ".merge", ".check", ".encode", ".decode", ".bot"};
  static const size_t arity[ENGINE_FUNCS] = {4, 2, 1, 1, 1, 0};
  Fn f = {arena, 0, 1};
  Stmts body[ENGINE_FUNCS];
  uint32_t locals[ENGINE_FUNCS];
  uint32_t *offs = fn_alloc(&f, (form->part_count == 0 ? 1 : form->part_count) * sizeof *offs);
  uint64_t len = 0;
  Layout lay;
  if (offs == NULL) return internal(err, form, "the arena is full");
  if (form->r == 0 || form->r > ENGINE_WORDS_MAX) return internal(err, form, "the replica count is out of range");
  for (size_t q = 0; q < form->part_count; q++) {
    if (form->parts[q].n > ENGINE_WORDS_MAX) return internal(err, form, "the state is above 65536 words");
    if (form->parts[q].kind == ENGINE_ESCROW && form->parts[q].split == NULL) return internal(err, form, "an escrow part has no split");
    offs[q] = (uint32_t)len;
    len += part_len(&form->parts[q], form->r);
    if (len > ENGINE_WORDS_MAX) return internal(err, form, "the state is above 65536 words");
  }
  if (form->inv_count > ENGINE_INV_MAX) return internal(err, form, "the form has more than 63 invariants");
  lay.form = form;
  lay.offs = offs;
  lay.len = (uint32_t)len;
  memset(body, 0, sizeof body);
  f.locals = 4;
  if (!build_apply(&f, &lay, &body[0], err)) return 0;
  locals[0] = f.locals;
  f.locals = 2;
  build_merge(&f, &lay, &body[1]);
  locals[1] = f.locals;
  f.locals = 1;
  if (!build_check(&f, &lay, &body[2], err)) return 0;
  locals[2] = f.locals;
  f.locals = 1;
  build_encode(&f, &lay, &body[3]);
  locals[3] = f.locals;
  f.locals = 1;
  build_decode(&f, &lay, &body[4]);
  locals[4] = f.locals;
  f.locals = 0;
  build_bot(&f, &lay, &body[5]);
  locals[5] = f.locals;
  for (size_t e = 0; e < ENGINE_FUNCS; e++) {
    out[e].name = entry_name(&f, form->name, suffix[e]);
    out[e].params = engine_params;
    out[e].param_count = arity[e];
    out[e].result = IR_SCALAR_NAT;
    out[e].local_count = locals[e];
    out[e].body = block_of(&body[e]);
    out[e].keeps_heap = 1;
  }
  return f.ok ? 1 : internal(err, form, "the arena is full");
}

int engine_program(Machine *m, const DeclList *decls, IrProgram *prog, FILE *err) {
  size_t forms = 0;
  size_t n = prog->func_count;
  IrFunc *funcs;
  for (size_t i = 0; i < decls->count; i++)
    if (decls->items[i].kind == DECL_REPLICATED) forms++;
  if (forms == 0) return 1;
  funcs = arena_alloc(m->arena, (n + ENGINE_FUNCS * forms) * sizeof *funcs);
  if (funcs == NULL) {
    fprintf(err, "langc: INTERNAL: the arena is full\n");
    return 0;
  }
  if (n > 0) memcpy(funcs, prog->funcs, n * sizeof *funcs);
  for (size_t i = 0; i < decls->count; i++) {
    EngineForm form;
    if (decls->items[i].kind != DECL_REPLICATED) continue;
    if (!engine_form(m, decls, i, &form)) return 0;
    if (!engine_funcs(m->arena, &form, funcs + n, err)) return 0;
    n += ENGINE_FUNCS;
  }
  prog->funcs = funcs;
  prog->func_count = n;
  return 1;
}
