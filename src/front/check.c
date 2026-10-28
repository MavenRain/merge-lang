/* The type checker (see front/check.h). SHARED by the tcc kits.

   Checking is bidirectional. A term is checked against an expected type (the
   hint) or its type is inferred. Two types are equal when their normal forms
   are equal (front/eval.c). The checker writes core terms (front/core.h). */
#include "front/check.h"

#include <string.h>

#define CHECK_DEPTH_LIMIT 2000u
#define SPINE_MAX 64u
#define SHOW_MAX 512u
#define NAMES_MAX 64u
#define UNIVERSE_MAX 2u
#define PRINT_MAX 65536u
#define NO_FAMILY UINT32_MAX
#define NO_DEF UINT32_MAX

#define FAIL(c, code, ...) diag_fail((c)->m->diag, (code), (c)->m->def, __VA_ARGS__)

typedef struct Scope Scope;
struct Scope {
  const char *name; /* NULL for the binder of `A -> B` */
  const Value *type;
  uint32_t level;
  const Scope *next;
};

typedef struct {
  Machine *m;
  const DeclList *decls;
  size_t decl_index;
  const Scope *scope; /* the innermost binder first */
  const Env *env;     /* a variable for each binder */
  uint32_t level;     /* the number of binders */
  unsigned depth;
  uint32_t eta_count;
  uint32_t self;      /* the family whose fields are checked, or NO_FAMILY */
  uint32_t self_uses; /* the uses of that family in the current field */
  const Value *face;  /* the faces of the enclosing system cells, or NULL */
} Checker;

typedef enum {
  RULE_TYPE0,
  RULE_TYPE1,
  RULE_FIN,
  RULE_VALUE0,
  RULE_EMPTY,
  RULE_SOME,
  RULE_PACK,
  RULE_REFL,
  RULE_SIGMA_PART,
  RULE_OPTION,
  RULE_PURE,
  RULE_MAP,
  RULE_BIND,
  RULE_FOLD,
  RULE_SYMM,
  RULE_TRANS,
  RULE_CONG,
  RULE_TRANSPORT,
  RULE_NAT2,
  RULE_FLAG_IF,
  RULE_PATHP,
  RULE_INTERVAL,
  RULE_PARTIAL,
  RULE_SYSTEM,
  RULE_TRANSP,
  RULE_HCOMP,
  RULE_ELIM,
  RULE_SUM_FIN
} Rule;

typedef struct {
  const char *name;
  uint32_t arity; /* fold: the least number of arguments */
  Rule rule;
  Op op;
  Op result;     /* VALUE0, NAT2: the result type. EMPTY: the type former. */
  uint32_t part; /* SIGMA_PART: 0 or 1. PATHP: 0 PathP, 1 Path. */
} Builtin;

static const Builtin BUILTINS[] = {
  {"Nat", 0, RULE_TYPE0, OP_NAT, OP_NAT, 0},
  {"Bool", 0, RULE_TYPE0, OP_BOOL, OP_NAT, 0},
  {"Unit", 0, RULE_TYPE0, OP_UNIT, OP_NAT, 0},
  {"Option", 1, RULE_TYPE1, OP_OPTION, OP_NAT, 0},
  {"Fin", 1, RULE_FIN, OP_NAT, OP_NAT, 0},
  {"I", 0, RULE_VALUE0, OP_I, OP_SSET, 0},
  {"i0", 0, RULE_VALUE0, OP_I0, OP_I, 0},
  {"i1", 0, RULE_VALUE0, OP_I1, OP_I, 0},
  {"~", 1, RULE_INTERVAL, OP_INEG, OP_I, 0},
  {"∧", 2, RULE_INTERVAL, OP_IMIN, OP_I, 0},
  {"∨", 2, RULE_INTERVAL, OP_IMAX, OP_I, 0},
  /* Faces and systems. The parser makes the heads of `[ φ ↦ u , ... ]`;
     no identifier can spell them. */
  {"Partial", 2, RULE_PARTIAL, OP_PARTIAL, OP_SSET, 0},
  {"↦", 3, RULE_SYSTEM, OP_SYSTEM, OP_NAT, 0},
  {"[]", 0, RULE_SYSTEM, OP_SYSNIL, OP_NAT, 0},
  /* Kan operations. */
  {"transp", 3, RULE_TRANSP, OP_TRANSP, OP_NAT, 0},
  {"hcomp", 2, RULE_HCOMP, OP_HCOMP, OP_NAT, 0},
  {"PathP", 3, RULE_PATHP, OP_PATHP, OP_NAT, 0},
  {"Path", 3, RULE_PATHP, OP_PATHP, OP_NAT, 1},
  {"unit", 0, RULE_VALUE0, OP_UNIT_VAL, OP_UNIT, 0},
  {"true", 0, RULE_VALUE0, OP_TRUE, OP_BOOL, 0},
  {"false", 0, RULE_VALUE0, OP_FALSE, OP_BOOL, 0},
  {"none", 0, RULE_EMPTY, OP_NONE, OP_OPTION, 0},
  {"some", 1, RULE_SOME, OP_SOME, OP_OPTION, 0},
  {"pack", 2, RULE_PACK, OP_PACK, OP_NAT, 0},
  {"refl", 0, RULE_REFL, OP_PLAM, OP_PATHP, 0},
  {"witness", 1, RULE_SIGMA_PART, OP_WITNESS, OP_NAT, 0},
  {"payload", 1, RULE_SIGMA_PART, OP_PAYLOAD, OP_NAT, 1},
  {"option", 3, RULE_OPTION, OP_OPTION_ELIM, OP_OPTION, 0},
  {"pure", 1, RULE_PURE, OP_PURE, OP_NAT, 0},
  {"map", 2, RULE_MAP, OP_MAP, OP_NAT, 0},
  {"bind", 2, RULE_BIND, OP_BIND, OP_NAT, 0},
  {"fold", 2, RULE_FOLD, OP_FOLD_NAT, OP_NAT, 0},
  {"symm", 1, RULE_SYMM, OP_PLAM, OP_PATHP, 0},
  {"trans", 2, RULE_TRANS, OP_PLAM, OP_PATHP, 0},
  {"cong", 2, RULE_CONG, OP_PLAM, OP_PATHP, 0},
  {"transport", 3, RULE_TRANSPORT, OP_TRANSP, OP_PATHP, 0},
  {"natAdd", 2, RULE_NAT2, OP_NAT_ADD, OP_NAT, 0},
  {"natSub", 2, RULE_NAT2, OP_NAT_SUB, OP_NAT, 0},
  {"natMul", 2, RULE_NAT2, OP_NAT_MUL, OP_NAT, 0},
  {"natEq", 2, RULE_NAT2, OP_NAT_EQ, OP_BOOL, 0},
  {"natLe", 2, RULE_NAT2, OP_NAT_LE, OP_BOOL, 0},
  {"natMax", 2, RULE_NAT2, OP_NAT_MAX, OP_NAT, 0},
  {"sumFin", 2, RULE_SUM_FIN, OP_SUM_FIN, OP_NAT, 0},
  {"flagIf", 3, RULE_FLAG_IF, OP_FLAG_IF, OP_BOOL, 0},
  {"elim", 4, RULE_ELIM, OP_FLAG_IF, OP_BOOL, 0}
};

#define BUILTIN_COUNT ((uint32_t)(sizeof BUILTINS / sizeof BUILTINS[0]))

/* Names that no declaration can take (rule R4). */
static const char *const RESERVED[] = {"Text"};

typedef enum {
  HEAD_TERM, /* a local, a definition or another term: plain application */
  HEAD_FAMILY,
  HEAD_CTOR,
  HEAD_PROJ,
  HEAD_BUILTIN
} HeadKind;

typedef struct {
  HeadKind kind;
  uint32_t index; /* FAMILY: the family. CTOR, PROJ: the constructor. */
  uint32_t field; /* PROJ: the field number */
  const Builtin *builtin;
} Head;

/* One application of a core name to the arguments of a spine. */
typedef struct {
  Checker *c;
  const Builtin *b;
  const Term *const *args;
  uint32_t argc;      /* the arguments in the spine */
  const Value *hint;  /* the expected type of the result, or NULL */
  const Value *outer; /* the expected type of the whole spine, or NULL */
  uint32_t used;      /* the arguments that the core name takes */
  const Value *type;  /* the type of the result */
} Call;

static const Core *nul(int failed) {
  (void)failed;
  return NULL;
}

static Core *new_core(Checker *c, CoreKind kind) {
  Core *k = arena_alloc(c->m->arena, sizeof *k);
  if (k == NULL) {
    FAIL(c, "OOM", "out of memory");
    return NULL;
  }
  k->kind = kind;
  return k;
}

static const Core *core_index(Checker *c, CoreKind kind, uint32_t index) {
  Core *k = new_core(c, kind);
  if (k != NULL)
    k->index = index;
  return k;
}

static const Core *core_leaf(Checker *c, CoreKind kind, uint64_t nat) {
  Core *k = new_core(c, kind);
  if (k != NULL)
    k->nat = nat;
  return k;
}

static const Core *core_bind(Checker *c, CoreKind kind, const char *name, const Core *a, const Core *b) {
  Core *k;
  if (b == NULL || (a == NULL && kind != CORE_LAM))
    return NULL;
  k = new_core(c, kind);
  if (k == NULL)
    return NULL;
  k->name = name;
  k->a = a;
  k->b = b;
  return k;
}

static const Core *core_app(Checker *c, const Core *f, const Core *x) {
  Core *k;
  if (f == NULL || x == NULL)
    return NULL;
  k = new_core(c, CORE_APP);
  if (k == NULL)
    return NULL;
  k->a = f;
  k->b = x;
  return k;
}

static const Core *core_op(Checker *c, Op op, uint32_t inst, uint32_t field, const Core *const *args, uint32_t n) {
  const Core **copy = arena_alloc(c->m->arena, ((size_t)n + 1u) * sizeof *copy);
  Core *k;
  uint32_t i;
  if (copy == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (i = 0; i < n; i++) {
    if (args[i] == NULL)
      return NULL;
    copy[i] = args[i];
  }
  k = new_core(c, CORE_OP);
  if (k == NULL)
    return NULL;
  k->op = op;
  k->inst = inst;
  k->field = field;
  k->args = copy;
  k->argc = n;
  return k;
}

static const Core *op3(Checker *c, Op op, uint32_t inst, uint32_t n, const Core *a, const Core *b, const Core *x) {
  const Core *args[3];
  args[0] = a;
  args[1] = b;
  args[2] = x;
  return core_op(c, op, inst, 0, args, n);
}

static const Value *tyop(Checker *c, Op op, uint32_t n, const Value *a, const Value *b, const Value *x) {
  return val_make(c->m, op, 0, n, a, b, x);
}

static const Value *nat_type(Checker *c) {
  return tyop(c, OP_NAT, 0, NULL, NULL, NULL);
}

static const Value *bool_type(Checker *c) {
  return tyop(c, OP_BOOL, 0, NULL, NULL, NULL);
}

/* The value of the core term K in the current context. */
static const Value *here(Checker *c, const Core *k) {
  return k == NULL ? NULL : eval_core(c->m, c->env, k);
}

/* A value with no arguments: SSet, I, i0 or i1. */
static const Value *op0(Checker *c, Op op) {
  return tyop(c, op, 0, NULL, NULL, NULL);
}

/* Conversion in the enclosing system faces, including under fresh binders. */
static int convert_at(Checker *c, uint32_t level, const Value *a, const Value *b) {
  return conv_values(c->m, level, a, b)
      || (c->face != NULL && conv_under_face(c->m, level, c->face, a, b));
}

/* Path B x y: PathP over the constant line `λ _ → B` (there is no OP_PATH). */
static const Value *path_type(Checker *c, const Value *b, const Value *x, const Value *y) {
  const Value *line = b == NULL ? NULL : here(c, core_bind(c, CORE_LAM, "_", NULL, quote_value(c->m, c->level + 1u, b)));
  return line == NULL || x == NULL || y == NULL ? NULL : tyop(c, OP_PATHP, 3, line, x, y);
}

/* 1 when the line of a PathP type does not use its interval variable. */
static int line_const(Checker *c, const Value *line) {
  return convert_at(c, c->level + 1u, apply_value(c->m, line, val_var(c->m, c->level)), apply_value(c->m, line, op0(c, OP_I0)));
}

/* V in the surface syntax, for a diagnostic. */
static const char *show(Checker *c, const Value *v) {
  const char *names[NAMES_MAX];
  const Scope *s;
  char *buf = arena_alloc(c->m->arena, SHOW_MAX);
  uint32_t count = c->level < NAMES_MAX ? c->level : NAMES_MAX;
  for (s = c->scope; s != NULL; s = s->next) {
    if (s->level < NAMES_MAX)
      names[s->level] = s->name != NULL ? s->name : "_";
  }
  if (buf == NULL || v == NULL)
    return "?";
  value_print(c->m, names, count, v, buf, SHOW_MAX);
  return buf;
}

static int push(Checker *c, const char *name, const Value *type) {
  Scope *s = arena_alloc(c->m->arena, sizeof *s);
  const Env *e = env_push(c->m, c->env, val_var(c->m, c->level));
  if (type == NULL || s == NULL || e == NULL)
    return c->m->diag->set ? 0 : FAIL(c, "OOM", "out of memory");
  s->name = name;
  s->type = type;
  s->level = c->level;
  s->next = c->scope;
  c->scope = s;
  c->env = e;
  c->level++;
  return 1;
}

static void pop(Checker *c) {
  c->scope = c->scope->next;
  c->env = c->env->next;
  c->level--;
}

/* 1 when a term of type FOUND can stand where WANT is expected.
   Universes are not cumulative (former F14). A pretype (sort SSet) is in
   no universe. */
static int expect(Checker *c, const Value *found, const Value *want) {
  if (found == NULL || want == NULL)
    return 0;
  if (convert_at(c, c->level, found, want))
    return 1;
  if (val_is(found, OP_SSET))
    return FAIL(c, "TYPE_PRETYPE", "expected a term of type %s, found a pretype (its sort is SSet)", show(c, want));
  return FAIL(c, "TYPE_MISMATCH", "expected a term of type %s, found a term of type %s", show(c, want), show(c, found));
}

static const Scope *find_local(const Checker *c, const char *name) {
  const Scope *s;
  for (s = c->scope; s != NULL; s = s->next) {
    if (s->name != NULL && strcmp(s->name, name) == 0)
      return s;
  }
  return NULL;
}

static uint32_t find_def(const Machine *m, const char *name) {
  uint32_t i;
  for (i = 0; i < m->def_count; i++) {
    if (strcmp(m->defs[i].name, name) == 0)
      return i;
  }
  return NO_DEF;
}

/* An environment that binds VALS in order: the last one is index 0. */
static const Env *env_of(Checker *c, const Value *const *vals, uint32_t n) {
  const Env *e = NULL;
  uint32_t i;
  for (i = 0; i < n; i++)
    e = env_push(c->m, e, vals[i]);
  return e;
}

static const Core *elab(Checker *c, const Term *t, const Value *hint, const Value **type);
static const Core *check_lam(Checker *c, const Term *t, const Value *hint, const Value **type);
static int annotation_ok(Checker *c, const Term *ann, const Value *dom);

static const Core *check(Checker *c, const Term *t, const Value *want) {
  const Value *type = NULL;
  return want == NULL ? NULL : elab(c, t, want, &type);
}

static const Core *infer(Checker *c, const Term *t, const Value **type) {
  *type = NULL;
  return elab(c, t, NULL, type);
}

/* infer, but a failure leaves no diagnostic. */
static const Core *try_infer(Checker *c, const Term *t, const Value **type) {
  Diag saved = *c->m->diag;
  const Core *k = infer(c, t, type);
  if (k == NULL)
    *c->m->diag = saved;
  return k;
}

/* T as a type; *LEVEL is its universe. With PRETYPE, a pretype (sort SSet:
   I, or a function type over I) is also a type and *LEVEL is UINT64_MAX. */
static const Core *check_type_at(Checker *c, const Term *t, uint64_t *level, int pretype) {
  const Value *type = NULL;
  const Core *k = infer(c, t, &type);
  if (k == NULL)
    return NULL;
  if (pretype && val_is(type, OP_SSET)) {
    *level = UINT64_MAX;
    return k;
  }
  if (val_is(type, OP_SSET))
    return nul(FAIL(c, "TYPE_PRETYPE", "expected a type in a universe, found the pretype %s", show(c, here(c, k))));
  if (type == NULL || type->kind != VAL_UNIV)
    return nul(FAIL(c, "TYPE_MISMATCH", "expected a type, found a term of type %s", show(c, type)));
  *level = type->nat;
  return k;
}

static const Core *check_type(Checker *c, const Term *t, uint64_t *level) {
  return check_type_at(c, t, level, 0);
}

/* Compares the inferred *TYPE with HINT. */
static const Core *finish(Checker *c, const Core *k, const Value *hint, const Value **type) {
  if (k == NULL || *type == NULL)
    return NULL;
  if (hint != NULL && !expect(c, *type, hint))
    return NULL;
  *type = hint != NULL ? hint : *type;
  return k;
}

/* 1 when T is Option A. *ELEM is A. */
static int carrier_of(const Value *t, Carrier *carrier, const Value **elem) {
  if (!val_is(t, OP_OPTION))
    return 0;
  *carrier = CARRIER_OPTION;
  *elem = t->args[0];
  return 1;
}

/* SHAPE (Option) with the element ELEM. */
static const Value *carried(Checker *c, const Value *shape, const Value *elem) {
  if (shape == NULL)
    return NULL;
  return tyop(c, shape->op, 1, elem, NULL, NULL);
}

/* Checks T as a function from DOM. *RESULT is WANT, or the result type that
   T infers when WANT is NULL. */
static const Core *fn_into(Checker *c, const Term *t, const Value *dom, const Value *want, const Value **result) {
  const Value *ft = NULL;
  const Core *f;
  if (want != NULL) {
    *result = want;
    return check(c, t, val_arrow(c->m, dom, want));
  }
  f = infer(c, t, &ft);
  if (f == NULL)
    return NULL;
  if (ft == NULL || ft->kind != VAL_PI)
    return nul(FAIL(c, "TYPE_NOT_FUNCTION", "expected a function, found a term of type %s", show(c, ft)));
  *result = closure_apply(c->m, ft, val_var(c->m, c->level));
  /* Compare at two fresh arguments before the result leaves the binder.
     Reusing its level in expect would capture a dependent result. */
  if (!convert_at(c, c->level + 2u, *result,
                   closure_apply(c->m, ft, val_var(c->m, c->level + 1u))))
    return nul(FAIL(c, "TYPE_MISMATCH", "this operation needs a function with a non-dependent result type"));
  return expect(c, ft, val_arrow(c->m, dom, *result)) ? f : NULL;
}

/* The argument type of the function T, from its binder or its type. */
static const Value *lam_domain(Checker *c, const Term *t) {
  uint64_t level = 0;
  const Value *ft = NULL;
  if (t->kind == TERM_LAM && t->left != NULL)
    return here(c, check_type(c, t->left, &level));
  return try_infer(c, t, &ft) != NULL && ft != NULL && ft->kind == VAL_PI ? ft->dom : NULL;
}

/* The carried argument ARG of map, bind or filter. When ARG does not infer
   (`none`), its type comes from the expected type and from the binder of
   the function FN. */
static const Core *carried_arg(Call *k, const Term *arg, const Term *fn, const Value **type) {
  Checker *c = k->c;
  Carrier carrier = CARRIER_OPTION;
  const Value *elem = NULL;
  const Value *dom;
  const Core *x = try_infer(c, arg, type);
  if (x != NULL)
    return x;
  if (!carrier_of(k->hint, &carrier, &elem))
    return infer(c, arg, type);
  dom = lam_domain(c, fn);
  if (dom == NULL)
    return infer(c, arg, type);
  *type = carried(c, k->hint, dom);
  return check(c, arg, *type);
}

static const Core *need_type(Call *k, const char *what) {
  return nul(FAIL(k->c, "TYPE_INFER", "%s needs a declared type %s here", k->b->name, what));
}

static const Core *not_carrier(Call *k, const Value *type) {
  return nul(FAIL(k->c, "TYPE_MISMATCH", "%s needs a value of type Option A, found a term of type %s", k->b->name, show(k->c, type)));
}

/* 1 when T is Path A a b (a PathP over a constant line). */
static int path_like(Checker *c, const Value *t) {
  return val_is(t, OP_PATHP) && line_const(c, t->args[0]);
}

/* The type A of Path A a b. */
static const Value *path_base(Checker *c, const Value *t) {
  return apply_value(c->m, t->args[0], op0(c, OP_I0));
}

static const Core *infer_path(Call *k, uint32_t i, const Value **type) {
  const Core *e = infer(k->c, k->args[i], type);
  if (e == NULL)
    return NULL;
  return path_like(k->c, *type) ? e : nul(FAIL(k->c, "TYPE_MISMATCH", "%s needs a proof of type Path A a b, found a term of type %s", k->b->name, show(k->c, *type)));
}

static const Core *rule_type1(Call *k) {
  uint64_t level = 0;
  const Core *a = check_type(k->c, k->args[0], &level);
  k->type = val_univ(k->c->m, level);
  return op3(k->c, k->b->op, 0, 1, a, NULL, NULL);
}

/* Fin n: the Sigma (k : ℕ) × Path Bool (natLe (natAdd k 1) n) true of
   PLAN 2.2 (Q-S2d-1 (a), Q-S2d-2 (a)). No Op: the Core is the unfolded
   Sigma, so the messages print the Sigma. The path type is under the
   binder k: k is the index 0 and n moves up one level. */
static const Core *fin_core(Checker *c, const Core *n) {
  const Core *next = op3(c, OP_NAT_ADD, 0, 2, core_index(c, CORE_VAR, 0), core_leaf(c, CORE_NAT, 1u), NULL);
  const Core *bound = n == NULL ? NULL : op3(c, OP_NAT_LE, 0, 2, next, quote_value(c->m, c->level + 1u, here(c, n)), NULL);
  const Core *line = core_bind(c, CORE_LAM, "_", NULL, op3(c, OP_BOOL, 0, 0, NULL, NULL, NULL));
  const Core *path = op3(c, OP_PATHP, 0, 3, line, bound, op3(c, OP_TRUE, 0, 0, NULL, NULL, NULL));
  return core_bind(c, CORE_SIGMA, "k", op3(c, OP_NAT, 0, 0, NULL, NULL, NULL), path);
}

static const Core *rule_fin(Call *k) {
  k->type = val_univ(k->c->m, 0);
  return fin_core(k->c, check(k->c, k->args[0], nat_type(k->c)));
}

/* Path A: the constant line `λ _ → A` (there is no OP_PATH). */
static const Core *const_line(Checker *c, const Term *t, uint64_t *level) {
  const Core *a = check_type(c, t, level);
  return a == NULL ? NULL : core_bind(c, CORE_LAM, "_", NULL, quote_value(c->m, c->level + 1u, here(c, a)));
}

/* PathP A: A is a line of type I → Type n. */
static const Core *pathp_line(Checker *c, const Term *t, uint64_t *level) {
  const Value *lt = NULL;
  const Core *a = infer(c, t, &lt);
  const Value *cod = NULL;
  if (a == NULL)
    return NULL;
  if (lt != NULL && lt->kind == VAL_PI && convert_at(c, c->level, lt->dom, op0(c, OP_I)))
    cod = closure_apply(c->m, lt, val_var(c->m, c->level));
  if (cod == NULL || cod->kind != VAL_UNIV)
    return nul(FAIL(c, "TYPE_MISMATCH", "PathP needs a line of type I → Type n, found a term of type %s", show(c, lt)));
  *level = cod->nat;
  return a;
}

/* PathP A a b (part 0) and Path A a b (part 1). */
static const Core *rule_pathp(Call *k) {
  Checker *c = k->c;
  uint64_t level = 0;
  const Core *line = k->b->part == 1u ? const_line(c, k->args[0], &level) : pathp_line(c, k->args[0], &level);
  const Value *lv = here(c, line);
  const Core *a = lv == NULL ? NULL : check(c, k->args[1], apply_value(c->m, lv, op0(c, OP_I0)));
  const Core *b = a == NULL ? NULL : check(c, k->args[2], apply_value(c->m, lv, op0(c, OP_I1)));
  k->type = val_univ(c->m, level);
  return op3(c, OP_PATHP, 0, 3, line, a, b);
}

static const Core *rule_empty(Call *k) {
  if (!val_is(k->hint, k->b->result))
    return need_type(k, "Option A");
  k->type = k->hint;
  return op3(k->c, k->b->op, 0, 0, NULL, NULL, NULL);
}

static const Core *rule_some(Call *k) {
  Checker *c = k->c;
  const Value *a = val_is(k->hint, OP_OPTION) ? k->hint->args[0] : NULL;
  const Core *x = a != NULL ? check(c, k->args[0], a) : infer(c, k->args[0], &a);
  k->type = tyop(c, OP_OPTION, 1, a, NULL, NULL);
  return op3(c, OP_SOME, 0, 1, x, NULL, NULL);
}

static const Core *rule_pack(Call *k) {
  Checker *c = k->c;
  const Value *s = k->hint;
  const Core *w;
  const Core *p;
  if (s == NULL || s->kind != VAL_SIGMA)
    return need_type(k, "(x : A) × B");
  w = check(c, k->args[0], s->dom);
  p = w == NULL ? NULL : check(c, k->args[1], closure_apply(c->m, s, here(c, w)));
  k->type = s;
  return op3(c, OP_PACK, 0, 2, w, p, NULL);
}

/* The path lambda λ NAME → BODY (BODY is under one more interval binder). */
static const Core *path_lam(Checker *c, const char *name, const Core *body) {
  return op3(c, OP_PLAM, 0, 1, core_bind(c, CORE_LAM, name, NULL, body), NULL, NULL);
}

/* The interval variable with the de Bruijn index I. */
static const Core *ivar(Checker *c, uint32_t i) {
  return core_index(c, CORE_VAR, i);
}

/* P R under DEPTH new interval binders, where P has the type T = PathP A a0 a1.
   OP_PAPP takes the ends of the type of P (zk 1d263fb check.c 1640..1805). */
static const Core *path_at(Checker *c, uint32_t depth, const Core *p, const Value *t, const Core *r) {
  const Core *args[4];
  args[0] = quote_value(c->m, c->level + depth, here(c, p));
  args[1] = r;
  args[2] = quote_value(c->m, c->level + depth, t->args[1]);
  args[3] = quote_value(c->m, c->level + depth, t->args[2]);
  return core_op(c, OP_PAPP, 0, 0, args, 4u);
}

/* The cell [ FACE ↦ BRANCH , REST ] of a system. */
static const Core *sys_cell(Checker *c, const Core *face, const Core *branch, const Core *rest) {
  return op3(c, OP_SYSTEM, 0, 3, face, branch, rest);
}

/* The body of trans p q under i, where p : Path A a b and q : Path A b c:
   hcomp A (~ i ∨ i) (λ j → [ ~ i ↦ a , i ↦ q j ]) (p i). */
static const Core *trans_body(Checker *c, const Core *p, const Value *tp, const Core *q, const Value *tq) {
  const Core *args[4];
  const Core *nil = op3(c, OP_SYSNIL, 0, 0, NULL, NULL, NULL);
  const Core *rest = sys_cell(c, ivar(c, 1), path_at(c, 2u, q, tq, ivar(c, 0)), nil);
  const Core *side = sys_cell(c, op3(c, OP_INEG, 0, 1, ivar(c, 1), NULL, NULL), quote_value(c->m, c->level + 2u, tp->args[1]), rest);
  args[0] = quote_value(c->m, c->level + 1u, path_base(c, tp));
  args[1] = op3(c, OP_IMAX, 0, 2, op3(c, OP_INEG, 0, 1, ivar(c, 0), NULL, NULL), ivar(c, 0), NULL);
  args[2] = core_bind(c, CORE_LAM, "j", NULL, side);
  args[3] = path_at(c, 1u, p, tp, ivar(c, 0));
  return core_op(c, OP_HCOMP, 0, 0, args, 4u);
}

static const Core *rule_refl(Call *k) {
  Checker *c = k->c;
  const Value *e = k->hint;
  if (!path_like(c, e))
    return need_type(k, "Path A a b");
  if (!convert_at(c, c->level, e->args[1], e->args[2]))
    return nul(FAIL(c, "TYPE_REFL", "refl cannot prove the equality: the left side normalizes to %s, the right side normalizes to %s", show(c, e->args[1]), show(c, e->args[2])));
  k->type = e;
  return path_lam(c, "i", quote_value(c->m, c->level + 1u, e->args[1]));
}

static const Core *rule_sigma_part(Call *k) {
  Checker *c = k->c;
  const Value *st = NULL;
  const Core *s = infer(c, k->args[0], &st);
  const Core *w;
  if (s == NULL)
    return NULL;
  if (st == NULL || st->kind != VAL_SIGMA)
    return nul(FAIL(c, "TYPE_MISMATCH", "%s needs a value of a type (x : A) × B, found a term of type %s", k->b->name, show(c, st)));
  w = op3(c, OP_WITNESS, 0, 1, s, NULL, NULL);
  k->type = k->b->part == 0u ? st->dom : closure_apply(c->m, st, here(c, w));
  return k->b->part == 0u ? w : op3(c, OP_PAYLOAD, 0, 1, s, NULL, NULL);
}

/* option z f o */
static const Core *rule_option(Call *k) {
  Checker *c = k->c;
  const Value *ot = NULL;
  const Value *res = k->hint;
  const Core *o = infer(c, k->args[2], &ot);
  const Core *z;
  const Core *f;
  if (o == NULL)
    return NULL;
  if (!val_is(ot, OP_OPTION))
    return nul(FAIL(c, "TYPE_MISMATCH", "option needs a value of type Option A, found a term of type %s", show(c, ot)));
  z = res != NULL ? check(c, k->args[0], res) : infer(c, k->args[0], &res);
  f = z == NULL ? NULL : check(c, k->args[1], val_arrow(c->m, ot->args[0], res));
  k->type = res;
  return op3(c, OP_OPTION_ELIM, 0, 3, z, f, o);
}

static const Core *rule_pure(Call *k) {
  Carrier carrier = CARRIER_OPTION;
  const Value *a = NULL;
  if (!carrier_of(k->hint, &carrier, &a))
    return need_type(k, "Option A");
  k->type = k->hint;
  return op3(k->c, OP_PURE, carrier, 1, check(k->c, k->args[0], a), NULL, NULL);
}

/* map f m */
static const Core *rule_map(Call *k) {
  Checker *c = k->c;
  Carrier carrier = CARRIER_OPTION;
  Carrier hint_carrier = CARRIER_OPTION;
  const Value *mt = NULL;
  const Value *a = NULL;
  const Value *b = NULL;
  const Value *hint_elem = NULL;
  const Core *x = carried_arg(k, k->args[1], k->args[0], &mt);
  const Core *f;
  if (x == NULL)
    return NULL;
  if (!carrier_of(mt, &carrier, &a))
    return not_carrier(k, mt);
  f = fn_into(c, k->args[0], a, carrier_of(k->hint, &hint_carrier, &hint_elem) ? hint_elem : NULL, &b);
  k->type = carried(c, mt, b);
  return op3(c, OP_MAP, carrier, 2, f, x, NULL);
}

/* bind m f */
static const Core *rule_bind(Call *k) {
  Checker *c = k->c;
  Carrier carrier = CARRIER_OPTION;
  Carrier result_carrier = CARRIER_OPTION;
  const Value *mt = NULL;
  const Value *a = NULL;
  const Value *b = NULL;
  const Value *res = NULL;
  const Core *x = carried_arg(k, k->args[0], k->args[1], &mt);
  const Core *f;
  if (x == NULL)
    return NULL;
  if (!carrier_of(mt, &carrier, &a))
    return not_carrier(k, mt);
  f = fn_into(c, k->args[1], a, k->hint, &res);
  if (f == NULL)
    return NULL;
  if (!carrier_of(res, &result_carrier, &b) || result_carrier != carrier)
    return nul(FAIL(c, "TYPE_MISMATCH", "bind needs a function into the carrier of %s, found one into %s", show(c, mt), show(c, res)));
  k->type = res;
  return op3(c, OP_BIND, carrier, 2, x, f, NULL);
}

/* fold step z n (Nat). */
static const Core *fold_simple(Call *k, const Core *s) {
  Checker *c = k->c;
  const Value *res = k->hint;
  const Core *z = res != NULL ? check(c, k->args[1], res) : infer(c, k->args[1], &res);
  const Core *f = z == NULL ? NULL : check(c, k->args[0], val_arrow(c->m, res, res));
  k->used = 3u;
  k->type = res;
  return op3(c, OP_FOLD_NAT, 0, 3, f, z, s);
}

/* The type of the fold case of CTOR: one argument for each field (a
   recursive field arrives folded, so its type is RES), then RES. */
static const Value *case_type(Checker *c, const CtorInfo *ctor, const Env *env, const Value *res) {
  const Value *t = res;
  uint32_t i = ctor->field_count;
  while (i > 0u && t != NULL) {
    const FieldInfo *field = &ctor->fields[--i];
    t = val_arrow(c->m, field->recursive ? res : eval_core(c->m, env, field->type), t);
  }
  return t;
}

/* The Core of PathP (λ _ → LINE) X Y at level LVL (LINE is at LVL + 1). */
static const Core *path_core(Checker *c, uint32_t lvl, const Core *line, const Value *x, const Value *y) {
  const Core *cx = x == NULL ? NULL : quote_value(c->m, lvl, x);
  const Core *cy = y == NULL ? NULL : quote_value(c->m, lvl, y);
  return line == NULL || cx == NULL || cy == NULL ? NULL : op3(c, OP_PATHP, 0, 3, core_bind(c, CORE_LAM, "_", NULL, line), cx, cy);
}

/* T is the family FAM (at any arguments). */
static int is_family_core(const Core *t, uint32_t fam) {
  return t != NULL && t->kind == CORE_OP && t->op == OP_FAMILY && t->inst == fam;
}

/* The point term K through the fold cases: a field variable is itself
   (from ENV), and a point constructor of FAM applies its case in CASES to
   its fields (the parameters drop). */
static const Value *star(Checker *c, uint32_t fam, const Value *const *cases, const Env *env, const Core *k) {
  Machine *m = c->m;
  const FamilyInfo *f = &m->families[fam];
  const Value *acc;
  const Value *arg;
  uint32_t j;
  if (k->kind != CORE_OP || k->op != OP_CTOR || k->inst < f->first_ctor || k->inst >= f->first_ctor + f->ctor_count || m->ctors[k->inst].depth != 0u)
    return eval_core(m, env, k);
  acc = cases[k->inst - f->first_ctor];
  for (j = 0; j < m->ctors[k->inst].field_count && acc != NULL; j++) {
    arg = star(c, fam, cases, env, k->args[f->param_count + j]);
    acc = arg == NULL ? NULL : apply_value(m, acc, arg);
  }
  return acc;
}

static int family_has_path(const Machine *m, uint32_t fam);
static const Core *path_body(const Core *t);

/* The type of the fold case of CTOR over FAM, a family with a path
   constructor: a Pi over the fields into RES at depth 0, else Path RES a*
   b* (depth 1) or Path (Path RES x* y*) p* q* (depth 2), with the ends
   through the earlier CASES. A recursive field has the type RES; a path
   field Path FAM u v has the type Path RES u* v*. A field type sees the
   earlier fields, so each field is a fresh variable in ENV. */
static const Value *hit_case_type(Checker *c, uint32_t fam, const CtorInfo *ctor, const Value *const *cases, const Env *env, const Value *res) {
  Machine *m = c->m;
  const Core **doms = arena_alloc(m->arena, ((size_t)ctor->field_count + 1u) * sizeof *doms);
  uint32_t n = ctor->field_count;
  uint32_t lvl = c->level + n;
  const Value *dom;
  const Core *line;
  const Core *k;
  uint32_t i;
  if (doms == NULL) {
    FAIL(c, "OOM", "out of memory");
    return NULL;
  }
  for (i = 0; i < n; i++) {
    const Core *t = ctor->fields[i].type;
    int path = !ctor->fields[i].recursive && is_family_core(path_body(t), fam);
    dom = ctor->fields[i].recursive ? res : path ? NULL : eval_core(m, env, t);
    doms[i] = path ? path_core(c, c->level + i, quote_value(m, c->level + i + 1u, res), star(c, fam, cases, env, t->args[1]), star(c, fam, cases, env, t->args[2]))
              : dom == NULL ? NULL : quote_value(m, c->level + i, dom);
    env = env_push(m, env, val_var(m, c->level + i));
    if (doms[i] == NULL || env == NULL)
      return NULL;
  }
  line = ctor->depth == 2u ? path_core(c, lvl + 1u, quote_value(m, lvl + 2u, res), star(c, fam, cases, env, ctor->ends[2]), star(c, fam, cases, env, ctor->ends[3])) : quote_value(m, lvl + 1u, res);
  k = ctor->depth == 0u ? quote_value(m, lvl, res) : path_core(c, lvl, line, star(c, fam, cases, env, ctor->ends[0]), star(c, fam, cases, env, ctor->ends[1]));
  for (i = n; i > 0u && k != NULL; i--)
    k = core_bind(c, CORE_PI, ctor->fields[i - 1u].name, doms[i - 1u], k);
  return here(c, k);
}

/* fold case... x over a domain family: one case for each constructor. */
static const Core *fold_family(Call *k, const Core *s, const Value *st) {
  Checker *c = k->c;
  Machine *m = c->m;
  const FamilyInfo *f = &m->families[st->inst];
  const Env *env = env_of(c, st->args, st->argc);
  const Value *res = k->hint;
  const Core **cores = arena_alloc(m->arena, ((size_t)f->ctor_count + 2u) * sizeof *cores);
  const Value **vals = arena_alloc(m->arena, ((size_t)f->ctor_count + 1u) * sizeof *vals);
  const Value *want;
  int hit = family_has_path(m, st->inst);
  uint32_t j;
  if (cores == NULL || vals == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  if (res == NULL && m->ctors[f->first_ctor].field_count == 0u)
    cores[0] = infer(c, k->args[0], &res);
  if (res == NULL)
    return nul(FAIL(c, "TYPE_INFER", "fold over %s needs a declared result type here", f->name));
  /* a family with a path constructor gets the case types of
     hit_case_type, whose ends go through the earlier cases (VALS) */
  for (j = 0; j < f->ctor_count; j++) {
    want = hit ? hit_case_type(c, st->inst, &m->ctors[f->first_ctor + j], vals, env, res) : case_type(c, &m->ctors[f->first_ctor + j], env, res);
    cores[j] = cores[j] != NULL ? cores[j] : want == NULL ? NULL : check(c, k->args[j], want);
    vals[j] = cores[j] == NULL || !hit ? NULL : here(c, cores[j]);
    if (cores[j] == NULL || (hit && vals[j] == NULL))
      return NULL;
  }
  cores[f->ctor_count] = s;
  k->used = f->ctor_count + 1u;
  k->type = res;
  return core_op(c, OP_FOLD_FAMILY, st->inst, 0, cores, f->ctor_count + 1u);
}

static int simple_scrutinee(const Value *t) {
  return val_is(t, OP_NAT);
}

/* fold takes its scrutinee last, so the scrutinee type sets the arity. */
/* 1 when some constructor of FAM is a path constructor. */
static int family_has_path(const Machine *m, uint32_t fam) {
  const FamilyInfo *f = &m->families[fam];
  uint32_t j;
  for (j = 0; j < f->ctor_count; j++)
    if (m->ctors[f->first_ctor + j].depth > 0u)
      return 1;
  return 0;
}

/* M of fold M step base n over ℕ: a function from ℕ to a type. As
   try_infer, a failure leaves no diagnostic, and the call is then a fold
   step z n applied to more arguments. */
static const Core *try_nat_motive(Checker *c, const Term *t, const Value **motive) {
  Diag saved = *c->m->diag;
  const Value *nat = nat_type(c);
  const Value *ty = NULL;
  const Core *body = NULL;
  const Core *mc = NULL;
  if (t->kind == TERM_LAM && (t->left == NULL || annotation_ok(c, t->left, nat)) && push(c, t->name, nat)) {
    body = infer(c, t->right, &ty);
    pop(c);
    mc = body == NULL || ty == NULL || ty->kind != VAL_UNIV ? NULL : core_bind(c, CORE_LAM, t->name, NULL, body);
  } else if (t->kind != TERM_LAM) {
    mc = try_infer(c, t, &ty);
    if (mc == NULL || ty == NULL || ty->kind != VAL_PI || !convert_at(c, c->level, ty->dom, nat))
      mc = NULL;
    else {
      ty = closure_apply(c->m, ty, val_var(c->m, c->level));
      if (ty == NULL || ty->kind != VAL_UNIV)
        mc = NULL;
    }
  }
  *motive = here(c, mc);
  if (*motive == NULL)
    *c->m->diag = saved;
  return *motive == NULL ? NULL : mc;
}

/* fold M step base n over ℕ: base : M 0, step : (k : ℕ) → M k →
   M (natAdd k 1), and the result is M n. The layout is [M, step, base, n];
   inst 1 marks the motive. */
static const Core *fold_nat_motive(Call *k, const Core *mc, const Value *motive, const Core *s) {
  Checker *c = k->c;
  Machine *m = c->m;
  const Value *kv = val_var(m, c->level);
  const Env *kenv = env_push(m, c->env, kv);
  const Core *add = op3(c, OP_NAT_ADD, 0, 2, quote_value(m, c->level + 1u, kv), quote_value(m, 0u, val_nat(m, 1u)), NULL);
  const Value *next = add == NULL || kenv == NULL ? NULL : eval_core(m, kenv, add);
  const Value *mk = apply_value(m, motive, kv);
  const Value *mnext = next == NULL ? NULL : apply_value(m, motive, next);
  const Core *dk = mk == NULL ? NULL : quote_value(m, c->level + 1u, mk);
  const Core *rk = mnext == NULL ? NULL : quote_value(m, c->level + 2u, mnext);
  const Core *acc = dk == NULL || rk == NULL ? NULL : core_bind(c, CORE_PI, "acc", dk, rk);
  const Core *nat = quote_value(m, c->level, nat_type(c));
  const Value *want = acc == NULL || nat == NULL ? NULL : here(c, core_bind(c, CORE_PI, "k", nat, acc));
  const Value *zero = apply_value(m, motive, val_nat(m, 0u));
  const Core *cores[4];
  cores[0] = mc;
  cores[2] = zero == NULL ? NULL : check(c, k->args[2], zero);
  cores[1] = cores[2] == NULL || want == NULL ? NULL : check(c, k->args[1], want);
  cores[3] = s;
  k->used = 4u;
  k->type = cores[1] == NULL ? NULL : apply_value(m, motive, here(c, s));
  return k->type == NULL ? NULL : core_op(c, OP_FOLD_NAT, 1u, 0, cores, 4u);
}

static const Core *rule_fold(Call *k) {
  Checker *c = k->c;
  const Value *st = NULL;
  const Value *motive = NULL;
  const Core *s = k->argc >= 4u ? try_infer(c, k->args[3], &st) : NULL;
  const Core *mc = s != NULL && val_is(st, OP_NAT) ? try_nat_motive(c, k->args[0], &motive) : NULL;
  uint32_t fam;
  /* A motive fold takes four arguments; the rest of the spine applies
     its result. */
  if (mc != NULL)
    return fold_nat_motive(k, mc, motive, s);
  s = try_infer(c, k->args[k->argc - 1u], &st);
  fam = val_is(st, OP_FAMILY) ? st->inst : NO_FAMILY;
  if (s != NULL && fam != NO_FAMILY && c->m->families[fam].index_count > 0u)
    return nul(FAIL(c, "DOMAIN_FAMILY", "fold over %s is not in S2: a fold over an indexed family needs a dependent eliminator (S3)", c->m->families[fam].name));
  if (s != NULL && fam != NO_FAMILY && c->m->families[fam].ctor_count + 1u == k->argc) {
    k->hint = k->outer;
    return fold_family(k, s, st);
  }
  if (s != NULL && k->argc == 3u && simple_scrutinee(st)) {
    k->hint = k->outer;
    return fold_simple(k, s);
  }
  s = k->argc > 3u ? try_infer(c, k->args[2], &st) : NULL;
  if (s != NULL && simple_scrutinee(st)) {
    k->hint = NULL;
    return fold_simple(k, s);
  }
  s = infer(c, k->args[k->argc - 1u], &st);
  return s == NULL ? NULL : nul(FAIL(c, "TYPE_MISMATCH", "fold needs a last argument of type ℕ or a domain data type, and one case for each constructor; found %u arguments and a last argument of type %s", k->argc, show(c, st)));
}

static const Core *rule_symm(Call *k) {
  const Value *et = NULL;
  const Core *e = infer_path(k, 0, &et);
  if (e == NULL)
    return NULL;
  k->type = path_type(k->c, path_base(k->c, et), et->args[2], et->args[1]);
  return path_lam(k->c, "i", path_at(k->c, 1u, e, et, op3(k->c, OP_INEG, 0, 1, ivar(k->c, 0), NULL, NULL)));
}

static const Core *rule_trans(Call *k) {
  Checker *c = k->c;
  const Value *t1 = NULL;
  const Value *t2 = NULL;
  const Core *e1 = infer_path(k, 0, &t1);
  const Core *e2 = e1 == NULL ? NULL : infer_path(k, 1, &t2);
  if (e2 == NULL)
    return NULL;
  if (!convert_at(c, c->level, path_base(c, t1), path_base(c, t2)) || !convert_at(c, c->level, t1->args[2], t2->args[1]))
    return nul(FAIL(c, "TYPE_MISMATCH", "trans needs the right side of the first proof, %s, to equal the left side of the second, %s", show(c, t1->args[2]), show(c, t2->args[1])));
  k->type = path_type(c, path_base(c, t1), t1->args[1], t2->args[2]);
  return path_lam(c, "i", trans_body(c, e1, t1, e2, t2));
}

/* cong f e */
static const Core *rule_cong(Call *k) {
  Checker *c = k->c;
  const Value *et = NULL;
  const Value *bt = NULL;
  const Value *fv;
  const Value *hint_type = path_like(c, k->hint) ? path_base(c, k->hint) : NULL;
  const Core *e = infer_path(k, 1, &et);
  const Core *f = e == NULL ? NULL : fn_into(c, k->args[0], path_base(c, et), hint_type, &bt);
  if (f == NULL)
    return NULL;
  fv = here(c, f);
  k->type = path_type(c, bt, apply_value(c->m, fv, et->args[1]), apply_value(c->m, fv, et->args[2]));
  return path_lam(c, "i", core_app(c, quote_value(c->m, c->level + 1u, fv), path_at(c, 1u, e, et, ivar(c, 0))));
}

/* transport P e u */
static const Core *rule_transport(Call *k) {
  Checker *c = k->c;
  const Value *et = NULL;
  const Value *pt = NULL;
  const Value *cod;
  const Value *pv;
  const Core *e = infer_path(k, 1, &et);
  const Core *p = e == NULL ? NULL : infer(c, k->args[0], &pt);
  const Core *u;
  if (p == NULL)
    return NULL;
  cod = pt != NULL && pt->kind == VAL_PI ? closure_apply(c->m, pt, val_var(c->m, c->level)) : NULL;
  if (cod == NULL || cod->kind != VAL_UNIV || !convert_at(c, c->level, pt->dom, path_base(c, et)))
    return nul(FAIL(c, "TYPE_MISMATCH", "transport needs a family of types over %s, found a term of type %s", show(c, path_base(c, et)), show(c, pt)));
  pv = here(c, p);
  u = check(c, k->args[2], apply_value(c->m, pv, et->args[1]));
  k->type = apply_value(c->m, pv, et->args[2]);
  return op3(c, OP_TRANSP, 0, 3, core_bind(c, CORE_LAM, "i", NULL, core_app(c, quote_value(c->m, c->level + 1u, pv), path_at(c, 1u, e, et, ivar(c, 0)))), op3(c, OP_I0, 0, 0, NULL, NULL, NULL), u);
}

static const Core *rule_nat2(Call *k) {
  Checker *c = k->c;
  const Core *x = check(c, k->args[0], nat_type(c));
  const Core *y = x == NULL ? NULL : check(c, k->args[1], nat_type(c));
  k->type = tyop(c, k->b->result, 0, NULL, NULL, NULL);
  return op3(c, k->b->op, 0, 2, x, y, NULL);
}

/* flagIf b yes no */
static const Core *rule_flag_if(Call *k) {
  Checker *c = k->c;
  const Value *res = k->hint;
  const Core *b = check(c, k->args[0], bool_type(c));
  const Core *yes = b == NULL ? NULL : (res != NULL ? check(c, k->args[1], res) : infer(c, k->args[1], &res));
  const Core *no = yes == NULL ? NULL : check(c, k->args[2], res);
  k->type = res;
  return op3(c, OP_FLAG_IF, 0, 3, b, yes, no);
}

/* The motive of elim over D: P : D → Type 1, else P : D → Type 0. The
   one site that checks the motive. A failed first try leaves no
   diagnostic (quorum-lang 351cbf4 elim_motive). */
static const Core *elim_motive(Checker *c, const Term *t, const Value *d) {
  Diag saved = *c->m->diag;
  const Env *env = c->env;
  const Core *p = check(c, t, val_arrow(c->m, d, val_univ(c->m, 1u)));
  if (p != NULL)
    return p;
  *c->m->diag = saved;
  c->env = env;
  return check(c, t, val_arrow(c->m, d, val_univ(c->m, 0u)));
}

/* (a : A) → P (some a), with PV the value of the motive P. */
static const Value *elim_some(Checker *c, const Value *pv, const Value *a) {
  const Core *some = op3(c, OP_SOME, 0, 1, core_index(c, CORE_VAR, 0u), NULL, NULL);
  const Core *body = some == NULL ? NULL : core_app(c, core_index(c, CORE_VAR, 1u), some);
  const Env *env = body == NULL ? NULL : env_push(c->m, NULL, pv);
  return env == NULL ? NULL : val_closure(c->m, VAL_PI, "a", a, env, body);
}

/* elim P yes no b (Bool) or elim P z s o (Option A): the dependent
   eliminator (quorum-lang 351cbf4 rule_elim). Inst 1 retains the motive
   in [P, yes, no, x] so a neutral normal form remains well typed.
   More arguments apply to the result. */
static const Core *rule_elim(Call *k) {
  Checker *c = k->c;
  Machine *m = c->m;
  const Value *d = NULL;
  const Core *x = infer(c, k->args[3], &d);
  const Core *p;
  const Value *pv;
  const Core *yes;
  const Core *no;
  const Core *cores[4];
  int flag;
  if (x == NULL)
    return NULL;
  if (!val_is(d, OP_BOOL) && !val_is(d, OP_OPTION))
    return nul(FAIL(c, "TYPE_MISMATCH", "elim needs a last argument of type Bool or Option A, found a term of type %s; a ℕ takes fold M step base n", show(c, d)));
  flag = val_is(d, OP_BOOL);
  p = elim_motive(c, k->args[0], d);
  pv = here(c, p);
  yes = pv == NULL ? NULL : check(c, k->args[1], apply_value(m, pv, tyop(c, flag ? OP_TRUE : OP_NONE, 0, NULL, NULL, NULL)));
  no = yes == NULL ? NULL : check(c, k->args[2], flag ? apply_value(m, pv, tyop(c, OP_FALSE, 0, NULL, NULL, NULL)) : elim_some(c, pv, d->args[0]));
  if (no == NULL)
    return NULL;
  k->type = apply_value(m, pv, here(c, x));
  cores[0] = p;
  cores[1] = yes;
  cores[2] = no;
  cores[3] = x;
  return core_op(c, flag ? OP_FLAG_IF : OP_OPTION_ELIM, 1u, 0, cores, 4u);
}

/* sumFin n f : ℕ, f : Fin n → ℕ (auction-lang rule_all_fin). */
static const Core *rule_sum_fin(Call *k) {
  Checker *c = k->c;
  const Core *n = check(c, k->args[0], nat_type(c));
  const Value *fin = n == NULL ? NULL : here(c, fin_core(c, n));
  const Core *f = fin == NULL ? NULL : check(c, k->args[1], val_arrow(c->m, fin, nat_type(c)));
  k->type = nat_type(c);
  return op3(c, OP_SUM_FIN, 0, 2, n, f, NULL);
}

/* ~ r, r ∧ s and r ∨ s: each argument is in I, and so is the result. */
static const Core *rule_interval(Call *k) {
  Checker *c = k->c;
  const Value *iv = op0(c, OP_I);
  const Core *a = check(c, k->args[0], iv);
  const Core *b = k->b->arity > 1u && a != NULL ? check(c, k->args[1], iv) : NULL;
  k->type = iv;
  return op3(c, k->b->op, 0, k->b->arity, a, b, NULL);
}

/* Partial φ A: φ is in I and A is a type. A pretype, as I. */
static const Core *rule_partial(Call *k) {
  Checker *c = k->c;
  uint64_t level = 0;
  const Core *phi = check(c, k->args[0], op0(c, OP_I));
  const Core *a = phi == NULL ? NULL : check_type(c, k->args[1], &level);
  k->type = op0(c, OP_SSET);
  return op3(c, OP_PARTIAL, 0, 2, phi, a, NULL);
}

/* Face equality in the face context. */
static int same_face(Checker *c, const Value *a, const Value *b) {
  const Value *x = a == NULL || c->face == NULL ? a : tyop(c, OP_IMIN, 2, c->face, a, NULL);
  const Value *y = b == NULL || c->face == NULL ? b : tyop(c, OP_IMIN, 2, c->face, b, NULL);
  return x != NULL && y != NULL && face_equal(c->m, c->level, x, y);
}

#define CELLS_MAX FACE_MEETS_MAX

/* 1 when T is a system cell `φ ↦ u` applied to the other cells. */
static int is_system_cell(const Term *t) {
  return t->kind == TERM_APP && t->left->kind == TERM_APP && t->left->left->kind == TERM_APP
         && t->left->left->left->kind == TERM_VAR && strcmp(t->left->left->left->name, "↦") == 0;
}

/* T against WANT with FACE added to the face context. A face with no meet
   (it never holds) adds nothing, so that branch gets the usual checks. */
static const Core *check_on(Checker *c, const Term *t, const Value *want, const Core *face) {
  FaceMeets fm;
  const Value *saved = c->face;
  const Value *fv = here(c, face);
  const Value *now = fv == NULL || saved == NULL ? fv : tyop(c, OP_IMIN, 2, saved, fv, NULL);
  if (now == NULL)
    return NULL;
  c->face = face_meets(c->m, c->level, now, &fm) && fm.count == 0 ? saved : now;
  const Core *out = check(c, t, want);
  c->face = saved;
  return out;
}

/* The branches UA and UB agree on each meet of FA ∧ FB (and the face
   context). An atom in a meet stays free, so the test is conservative
   there. */
static int cells_agree(Checker *c, const Core *fa, const Core *ua, const Core *fb, const Core *ub) {
  FaceMeets fm;
  const Value *both = here(c, op3(c, OP_IMIN, 0, 2, fa, fb, NULL));
  const Value *meet = both == NULL || c->face == NULL ? both : tyop(c, OP_IMIN, 2, both, c->face, NULL);
  const Value *x = meet == NULL ? NULL : here(c, ua);
  const Value *y = x == NULL ? NULL : here(c, ub);
  if (y == NULL)
    return 0;
  if (!face_meets(c->m, c->level, meet, &fm))
    return FAIL(c, "TYPE_SYSTEM", "the overlap %s of two faces is not in the face bounds", show(c, meet));
  if (!conv_under_face(c->m, c->level, meet, x, y))
    return FAIL(c, "TYPE_SYSTEM", "two branches disagree where the faces meet at %s: %s against %s", show(c, meet),
                show(c, value_under_face(c->m, c->level, meet, x)), show(c, value_under_face(c->m, c->level, meet, y)));
  return 1;
}

/* [ φ1 ↦ u1 , ... ] against Partial ψ A: each face is in I, each branch is
   in A under its face, the faces join to ψ, and two branches agree where
   their faces meet. Two systems convert by structure, in order (no
   order-free conv_system). */
static const Core *rule_system(Call *k) {
  Checker *c = k->c;
  const Value *e = k->hint;
  const Term *face[CELLS_MAX];
  const Term *branch[CELLS_MAX];
  const Core *fk[CELLS_MAX];
  const Core *uk[CELLS_MAX];
  const Term *rest = k->b->arity == 3u ? k->args[2] : NULL;
  uint32_t n = 0;
  if (!val_is(e, OP_PARTIAL))
    return need_type(k, "Partial φ A");
  if (rest != NULL) {
    face[0] = k->args[0];
    branch[0] = k->args[1];
    n = 1;
  }
  for (; rest != NULL && is_system_cell(rest) && n < CELLS_MAX; rest = rest->right) {
    face[n] = rest->left->left->right;
    branch[n] = rest->left->right;
    n++;
  }
  if (rest != NULL && (rest->kind != TERM_VAR || strcmp(rest->name, "[]") != 0))
    return nul(FAIL(c, "TYPE_SYSTEM", "a system has at most %u cells", (unsigned)CELLS_MAX));
  const Value *join = op0(c, OP_I0);
  for (uint32_t i = 0; i < n; i++) {
    fk[i] = check(c, face[i], op0(c, OP_I));
    uk[i] = fk[i] == NULL ? NULL : check_on(c, branch[i], e->args[1], fk[i]);
    const Value *fv = uk[i] == NULL ? NULL : here(c, fk[i]);
    if (fv == NULL)
      return NULL;
    join = i == 0 ? fv : tyop(c, OP_IMAX, 2, join, fv, NULL);
  }
  if (!same_face(c, join, e->args[0]))
    return nul(FAIL(c, "TYPE_SYSTEM", "the system covers %s, the type needs %s", show(c, join), show(c, e->args[0])));
  for (uint32_t i = 0; i < n; i++)
    for (uint32_t j = i + 1u; j < n; j++)
      if (!cells_agree(c, fk[i], uk[i], fk[j], uk[j]))
        return NULL;
  const Core *acc = op3(c, OP_SYSNIL, 0, 0, NULL, NULL, NULL);
  for (uint32_t i = n; i > 0; i--)
    acc = op3(c, OP_SYSTEM, 0, 3, fk[i - 1u], uk[i - 1u], acc);
  k->type = e;
  return acc;
}

/* The line A of transp A r u: an unannotated `λ i → T` binds i : I, so the
   printed form checks again. Any other term goes to pathp_line. */
static const Core *line_of(Checker *c, const Term *t, uint64_t *level) {
  const Core *body;
  if (t->kind != TERM_LAM || t->left != NULL)
    return pathp_line(c, t, level);
  if (!push(c, t->name, op0(c, OP_I)))
    return NULL;
  body = check_type_at(c, t->right, level, 0);
  pop(c);
  return body == NULL ? NULL : core_bind(c, CORE_LAM, t->name, NULL, body);
}

/* The line LV is constant on each meet of R and the face context, so
   transp A r u = u where r holds. A face with no meet adds no condition.
   An atom in a meet stays free, so the test is conservative there. A face
   that is not in the bounds needs a constant line. */
static int line_const_on(Checker *c, const Value *lv, const Value *r) {
  FaceMeets fm;
  const Value *phi = r == NULL || c->face == NULL ? r : tyop(c, OP_IMIN, 2, r, c->face, NULL);
  const Value *x = phi == NULL ? NULL : apply_value(c->m, lv, val_var(c->m, c->level));
  const Value *y = x == NULL ? NULL : apply_value(c->m, lv, op0(c, OP_I0));
  if (y == NULL)
    return 0;
  if (conv_values(c->m, c->level + 1u, x, y))
    return 1;
  if (face_meets(c->m, c->level, phi, &fm) && (fm.count == 0u || conv_under_face(c->m, c->level + 1u, phi, x, y)))
    return 1;
  return FAIL(c, "TYPE_LINE", "transp needs a line that is constant where r holds, found the line %s", show(c, lv));
}

/* transp A r u: A : I → Type n, r : I and u : A i0, of type A i1. A is
   constant where r holds. */
static const Core *rule_transp(Call *k) {
  Checker *c = k->c;
  uint64_t level = 0;
  const Core *line = line_of(c, k->args[0], &level);
  const Value *lv = here(c, line);
  const Core *r = lv == NULL ? NULL : check(c, k->args[1], op0(c, OP_I));
  const Core *u;
  if (r == NULL || !line_const_on(c, lv, here(c, r)))
    return NULL;
  u = check(c, k->args[2], apply_value(c->m, lv, op0(c, OP_I0)));
  k->type = apply_value(c->m, lv, op0(c, OP_I1));
  return op3(c, OP_TRANSP, 0, 3, line, r, u);
}

/* Check a side face in the scope of its binder, then require independence
   from that binder before returning it in the outer context. */
static const Core *side_face(Checker *c, const Term *t, const char *name) {
  if (!push(c, name, op0(c, OP_I)))
    return NULL;
  const Core *inner = check(c, t, op0(c, OP_I));
  const Value *fv = here(c, inner);
  pop(c);
  if (fv == NULL)
    return NULL;
  const Env *env0 = env_push(c->m, c->env, op0(c, OP_I0));
  const Value *at0 = env0 == NULL ? NULL : eval_core(c->m, env0, inner);
  if (at0 == NULL)
    return NULL;
  if (!conv_values(c->m, c->level + 1u, fv, at0))
    return nul(FAIL(c, "TYPE_SYSTEM", "an hcomp face depends on its side binder %s", name));
  return quote_value(c->m, c->level, at0);
}

/* The side `λ j → [ φ1 ↦ u1 , ... ]` of hcomp against TY, with j : I. The
   faces are scoped under j and must be independent of it:
   FK gets the faces and *PHI their join. Then each branch is checked under
   j and its face, and two branches agree where their faces meet. A side
   that is not a system literal is refused. */
static const Core *side_of(Checker *c, const Term *t, const Value *ty, const Core **phi, const Core **fk, uint32_t *n) {
  const Term *s = t->kind == TERM_LAM ? t->right : NULL;
  const Value *fv[CELLS_MAX];
  const Term *branch[CELLS_MAX];
  const Core *inner[CELLS_MAX];
  const Core *uk[CELLS_MAX];
  const Core *body;
  uint32_t i;
  uint32_t q;
  *n = 0;
  *phi = op3(c, OP_I0, 0, 0, NULL, NULL, NULL);
  if (s == NULL || (!is_system_cell(s) && (s->kind != TERM_VAR || strcmp(s->name, "[]") != 0)))
    return nul(FAIL(c, "TYPE_SYSTEM", "hcomp needs a side λ j → [ φ ↦ u , ... ]"));
  if (t->left != NULL && !annotation_ok(c, t->left, op0(c, OP_I)))
    return NULL;
  for (; is_system_cell(s) && *n < CELLS_MAX; s = s->right) {
    fk[*n] = side_face(c, s->left->left->right, t->name);
    fv[*n] = here(c, fk[*n]);
    if (fv[*n] == NULL)
      return NULL;
    branch[*n] = s->left->right;
    *phi = *n == 0 ? fk[0] : op3(c, OP_IMAX, 0, 2, *phi, fk[*n], NULL);
    (*n)++;
  }
  if (s->kind != TERM_VAR || strcmp(s->name, "[]") != 0)
    return nul(FAIL(c, "TYPE_SYSTEM", "a system has at most %u cells", (unsigned)CELLS_MAX));
  if (!push(c, t->name, op0(c, OP_I)))
    return NULL;
  for (i = 0; i < *n; i++) {
    inner[i] = quote_value(c->m, c->level, fv[i]);
    uk[i] = inner[i] == NULL ? NULL : check_on(c, branch[i], ty, inner[i]);
    if (uk[i] == NULL) {
      pop(c);
      return NULL;
    }
  }
  for (i = 0; i < *n; i++)
    for (q = i + 1u; q < *n; q++)
      if (!cells_agree(c, inner[i], uk[i], inner[q], uk[q])) {
        pop(c);
        return NULL;
      }
  body = op3(c, OP_SYSNIL, 0, 0, NULL, NULL, NULL);
  for (i = *n; i > 0; i--)
    body = op3(c, OP_SYSTEM, 0, 3, inner[i - 1u], uk[i - 1u], body);
  pop(c);
  return core_bind(c, CORE_LAM, t->name, NULL, body);
}

/* The base condition of hcomp: where its face holds, each branch of SIDE
   at j = i0 is U0. */
static int side_base(Checker *c, const Core *side, const Core **fk, uint32_t n, const Core *u0) {
  const Core *i0 = op3(c, OP_I0, 0, 0, NULL, NULL, NULL);
  const Core *i1 = op3(c, OP_I1, 0, 0, NULL, NULL, NULL);
  const Core *s = side->b;
  uint32_t q;
  for (q = 0; q < n && s != NULL && s->kind == CORE_OP && s->op == OP_SYSTEM; q++, s = s->args[2])
    if (!cells_agree(c, fk[q], core_app(c, core_bind(c, CORE_LAM, side->name, NULL, s->args[1]), i0), i1, u0))
      return 0;
  return 1;
}

/* Only a type in a universe has hcomp. A function type is a pretype when
   its domain or its codomain is a pretype. */
static int hcomp_type(Checker *c, const Value *a) {
  const Value *point;
  if (a == NULL)
    return 0;
  if (val_is(a, OP_I) || val_is(a, OP_PARTIAL))
    return FAIL(c, "TYPE_PRETYPE", "hcomp needs a type in a universe, found the pretype %s", show(c, a));
  if (a->kind != VAL_PI)
    return 1;
  if (!hcomp_type(c, a->dom))
    return 0;
  point = val_var(c->m, c->m->fresh_level);
  return point != NULL && hcomp_type(c, closure_apply(c->m, a, point));
}

/* hcomp side u0: A is the hint, else the type of u0. φ is the join of the
   faces of the side; where a face holds, its branch at j = i0 is u0. The
   type is A. */
static const Core *rule_hcomp(Call *k) {
  Checker *c = k->c;
  const Value *a = k->hint;
  const Core *fk[CELLS_MAX];
  const Core *args[4];
  const Core *phi = NULL;
  const Core *side;
  const Core *u0 = NULL;
  uint32_t n = 0;
  if (a == NULL) {
    u0 = try_infer(c, k->args[1], &a);
    if (u0 == NULL)
      return need_type(k, "A");
  }
  if (!hcomp_type(c, a))
    return NULL;
  side = side_of(c, k->args[0], a, &phi, fk, &n);
  if (side == NULL)
    return NULL;
  if (u0 == NULL)
    u0 = check(c, k->args[1], a);
  if (u0 == NULL || !side_base(c, side, fk, n, u0))
    return NULL;
  k->type = a;
  args[0] = quote_value(c->m, c->level, a);
  args[1] = phi;
  args[2] = side;
  args[3] = u0;
  return core_op(c, OP_HCOMP, 0, 0, args, 4u);
}

static const Core *rule(Call *k) {
  switch (k->b->rule) {
  case RULE_TYPE0:
    k->type = val_univ(k->c->m, 0);
    return op3(k->c, k->b->op, 0, 0, NULL, NULL, NULL);
  case RULE_VALUE0:
    k->type = tyop(k->c, k->b->result, 0, NULL, NULL, NULL);
    return op3(k->c, k->b->op, 0, 0, NULL, NULL, NULL);
  case RULE_INTERVAL:
    return rule_interval(k);
  case RULE_PARTIAL:
    return rule_partial(k);
  case RULE_SYSTEM:
    return rule_system(k);
  case RULE_TRANSP:
    return rule_transp(k);
  case RULE_HCOMP:
    return rule_hcomp(k);
  case RULE_TYPE1:
    return rule_type1(k);
  case RULE_FIN:
    return rule_fin(k);
  case RULE_EMPTY:
    return rule_empty(k);
  case RULE_SOME:
    return rule_some(k);
  case RULE_PACK:
    return rule_pack(k);
  case RULE_REFL:
    return rule_refl(k);
  case RULE_SIGMA_PART:
    return rule_sigma_part(k);
  case RULE_OPTION:
    return rule_option(k);
  case RULE_PURE:
    return rule_pure(k);
  case RULE_MAP:
    return rule_map(k);
  case RULE_BIND:
    return rule_bind(k);
  case RULE_FOLD:
    return rule_fold(k);
  case RULE_SYMM:
    return rule_symm(k);
  case RULE_TRANS:
    return rule_trans(k);
  case RULE_CONG:
    return rule_cong(k);
  case RULE_TRANSPORT:
    return rule_transport(k);
  case RULE_NAT2:
    return rule_nat2(k);
  case RULE_FLAG_IF:
    return rule_flag_if(k);
  case RULE_ELIM:
    return rule_elim(k);
  case RULE_SUM_FIN:
    return rule_sum_fin(k);
  case RULE_PATHP:
    return rule_pathp(k);
  }
  return NULL;
}

/* Checks the family parameters ARGS. VALS gets their values. */
static int check_params(Checker *c, const FamilyInfo *f, const Term *const *args, const Core **cores, const Value **vals) {
  uint32_t i;
  for (i = 0; i < f->param_count; i++) {
    cores[i] = check(c, args[i], eval_core(c->m, env_of(c, vals, i), f->param_types[i]));
    vals[i] = here(c, cores[i]);
    if (vals[i] == NULL)
      return 0;
  }
  return 1;
}

/* The fields of a constructor of an indexed family are a telescope: a field
   type can use the earlier fields. A cell monad keeps fields in the context
   of its parameters only. */
static int telescope(const FamilyInfo *f) {
  return f->index_count > 0u && f->r1 != FAMILY_CM;
}

static const Core *family_app(Call *k, uint32_t fam) {
  Checker *c = k->c;
  const FamilyInfo *f = &c->m->families[fam];
  uint32_t p = f->param_count;
  const Core **cores = arena_alloc(c->m->arena, ((size_t)p + f->index_count + 1u) * sizeof *cores);
  const Value **vals = arena_alloc(c->m->arena, ((size_t)p + 1u) * sizeof *vals);
  uint32_t j;
  if (cores == NULL || vals == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  if (!check_params(c, f, k->args, cores, vals))
    return NULL;
  for (j = 0; j < f->index_count; j++) {
    cores[p + j] = check(c, k->args[p + j], eval_core(c->m, env_of(c, vals, p), f->index_types[j]));
    if (cores[p + j] == NULL)
      return NULL;
  }
  k->type = val_univ(c->m, f->universe);
  return core_op(c, OP_FAMILY, fam, 0, cores, p + f->index_count);
}

/* A constructor takes the family parameters, then its fields. Its type is
   the family at the parameters and at its result indices. */
/* A path constructor at its params and fields: OP_PCTOR under one OP_PLAM
   for each dimension. Its type is the constructor path type. */
static const Core *pctor_app(Call *k, uint32_t ctor, const Value *const *vals, uint32_t n) {
  Checker *c = k->c;
  const CtorInfo *ci = &c->m->ctors[ctor];
  uint32_t d = ci->depth;
  const Core **args = arena_alloc(c->m->arena, ((size_t)n + d + 1u) * sizeof *args);
  const Core *body;
  uint32_t i;
  if (args == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (i = 0; i < n; i++)
    args[i] = quote_value(c->m, c->level + d, vals[i]);
  for (i = 0; i < d; i++)
    args[n + i] = core_index(c, CORE_VAR, d - 1u - i);
  k->type = eval_core(c->m, env_of(c, vals, n), ci->type);
  body = core_op(c, OP_PCTOR, ctor, 0, args, n + d);
  for (i = 0; i < d; i++)
    body = op3(c, OP_PLAM, 0, 1, core_bind(c, CORE_LAM, i == 0u && d == 2u ? "j" : "i", NULL, body), NULL, NULL);
  return body;
}

static const Core *ctor_app(Call *k, uint32_t ctor) {
  Checker *c = k->c;
  const CtorInfo *ci = &c->m->ctors[ctor];
  const FamilyInfo *f = &c->m->families[ci->family];
  uint32_t p = f->param_count;
  uint32_t n = p + ci->field_count;
  int dep = telescope(f) || ci->depth > 0u;
  const Core **cores = arena_alloc(c->m->arena, ((size_t)n + 1u) * sizeof *cores);
  const Value **vals = arena_alloc(c->m->arena, ((size_t)n + 1u) * sizeof *vals);
  const Value **tvals = arena_alloc(c->m->arena, ((size_t)p + f->index_count + 1u) * sizeof *tvals);
  uint32_t j;
  if (cores == NULL || vals == NULL || tvals == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  if (!check_params(c, f, k->args, cores, vals))
    return NULL;
  for (j = 0; j < ci->field_count; j++) {
    cores[p + j] = check(c, k->args[p + j], eval_core(c->m, env_of(c, vals, dep ? p + j : p), ci->fields[j].type));
    if (cores[p + j] == NULL)
      return NULL;
    vals[p + j] = dep ? here(c, cores[p + j]) : NULL;
  }
  for (j = 0; j < p + f->index_count; j++)
    tvals[j] = j < p ? vals[j] : eval_core(c->m, env_of(c, vals, dep ? n : p), ci->indices[j - p]);
  if (ci->depth > 0u)
    return pctor_app(k, ctor, vals, n);
  k->type = val_op(c->m, VAL_OP, OP_FAMILY, ci->family, 0, tvals, p + f->index_count);
  return core_op(c, OP_CTOR, ctor, 0, cores, n);
}

/* A field of a one-constructor family is a projection. In a telescope the
   earlier fields are the projections of the same value. */
static const Core *proj_app(Call *k, uint32_t ctor, uint32_t field) {
  Checker *c = k->c;
  const CtorInfo *ci = &c->m->ctors[ctor];
  const FamilyInfo *f = &c->m->families[ci->family];
  uint32_t p = f->param_count;
  uint32_t n = telescope(f) ? p + field : p;
  const Value **vals = arena_alloc(c->m->arena, ((size_t)n + 1u) * sizeof *vals);
  const Value *xt = NULL;
  const Core *x = infer(c, k->args[0], &xt);
  uint32_t e;
  if (x == NULL)
    return NULL;
  if (!val_is(xt, OP_FAMILY) || xt->inst != ci->family)
    return nul(FAIL(c, "TYPE_MISMATCH", "%s needs a value of the family %s, found a term of type %s", ci->fields[field].name, f->name, show(c, xt)));
  if (vals == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (e = 0; e < n; e++)
    vals[e] = e < p ? xt->args[e] : here(c, core_op(c, OP_PROJ, ctor, e - p, &x, 1));
  k->type = eval_core(c->m, env_of(c, vals, n), ci->fields[field].type);
  return core_op(c, OP_PROJ, ctor, field, &x, 1);
}

static int find_known(Checker *c, const char *name, Head *h) {
  const Machine *m = c->m;
  uint32_t i;
  uint32_t j;
  for (i = 0; i < m->family_count; i++) {
    if (strcmp(m->families[i].name, name) == 0) {
      h->kind = HEAD_FAMILY;
      h->index = i;
      c->self_uses += i == c->self;
      return 1;
    }
  }
  for (i = 0; i < m->ctor_count; i++) {
    if (strcmp(m->ctors[i].name, name) == 0) {
      h->kind = HEAD_CTOR;
      h->index = i;
      return 1;
    }
  }
  for (i = 0; i < m->ctor_count; i++) {
    for (j = 0; m->families[m->ctors[i].family].ctor_count == 1u && j < m->ctors[i].field_count; j++) {
      if (strcmp(m->ctors[i].fields[j].name, name) == 0) {
        h->kind = HEAD_PROJ;
        h->index = i;
        h->field = j;
        return 1;
      }
    }
  }
  for (i = 0; i < BUILTIN_COUNT; i++) {
    if (strcmp(BUILTINS[i].name, name) == 0) {
      h->kind = HEAD_BUILTIN;
      h->builtin = &BUILTINS[i];
      return 1;
    }
  }
  return 0;
}

/* Core names that S1 removed. FIX is the name that replaces it, or NULL
   when no name does; HINT goes into the message. */
typedef struct {
  const char *name;
  const char *fix;
  const char *hint;
  const char *stage; /* the step that removed NAME */
} Dropped;

static const Dropped DROPPED[] = {
  {"Flag", "Bool", "use Bool", "S1"},
  {"flagYes", "true", "use true", "S1"},
  {"flagNo", "false", "use false", "S1"},
  {"Prod", NULL, "write the product of A and B as A × B", "S1"},
  {"pair", "pack", "use pack (a term of A × B is pack a b)", "S1"},
  {"first", "witness", "use witness", "S1"},
  {"second", "payload", "use payload", "S1"},
  {"List", NULL, "use a vector Fin n → A", "S1"},
  {"nil", NULL, "use a vector Fin n → A", "S1"},
  {"cons", NULL, "use a vector Fin n → A", "S1"},
  {"filter", NULL, "no core name replaces it", "S1"},
  {"unfold", NULL, "no core name replaces it before S2", "S1"},
  {"Eq", "Path", "use Path A x y", "S2"},
  {"Sum", NULL, "use Option or a base sum", "S2d"},
  {"inl", NULL, "use Option or a base sum", "S2d"},
  {"inr", NULL, "use Option or a base sum", "S2d"},
  {"either", NULL, "use Option or a base sum", "S2d"}
};

/* An unknown name at HEAD. When it names this declaration or a later one,
   the use is recursion (rule R3). The span is the name; main.c adds the
   file. */
static int unknown_name(Checker *c, const Term *head) {
  const char *name = head->name;
  const Dropped *drop = NULL;
  const char *p;
  int width = 0;
  size_t i;
  Span span;
  for (i = c->decl_index; i < c->decls->count; i++) {
    if (strcmp(c->decls->items[i].name, name) == 0)
      return FAIL(c, "REFUSE_REC", "%s is this definition or a later one; a definition can use only earlier ones (rule R3)", name);
  }
  for (i = 0; i < sizeof DROPPED / sizeof DROPPED[0]; i++) {
    if (drop == NULL && strcmp(DROPPED[i].name, name) == 0) drop = &DROPPED[i];
  }
  for (p = name; *p != '\0'; p++) width += ((unsigned char)*p & 0xC0u) != 0x80u;
  span = (Span){NULL, head->line, head->col, head->line, head->col + width};
  if (drop == NULL)
    return diag_fail_at(c->m->diag, "NAME_UNKNOWN", c->m->def, span, NULL, name, NULL, "unknown name %s", name);
  return diag_fail_at(c->m->diag, "NAME_UNKNOWN", c->m->def, span, drop->fix, name, drop->fix, "unknown name %s (%s removed it); %s", name, drop->stage, drop->hint);
}

/* Name resolution order: locals (the innermost first), definitions,
   families, constructors, projections, core names. */
static int resolve_head(Checker *c, const Term *head, Head *h) {
  memset(h, 0, sizeof *h);
  h->kind = HEAD_TERM;
  if (head->kind != TERM_VAR || find_local(c, head->name) != NULL || find_def(c->m, head->name) != NO_DEF)
    return 1;
  return find_known(c, head->name, h) ? 1 : unknown_name(c, head);
}

static uint32_t known_arity(const Checker *c, const Head *h) {
  const CtorInfo *ci = h->kind == HEAD_CTOR ? &c->m->ctors[h->index] : NULL;
  switch (h->kind) {
  case HEAD_TERM:
    return 0u;
  case HEAD_FAMILY:
    return c->m->families[h->index].param_count + c->m->families[h->index].index_count;
  case HEAD_CTOR:
    return ci == NULL ? 0u : c->m->families[ci->family].param_count + ci->field_count;
  case HEAD_PROJ:
    return 1u;
  case HEAD_BUILTIN:
    return h->builtin->arity;
  }
  return 0u;
}

static const char *known_name(const Checker *c, const Head *h) {
  switch (h->kind) {
  case HEAD_TERM:
    return "?";
  case HEAD_FAMILY:
    return c->m->families[h->index].name;
  case HEAD_CTOR:
    return c->m->ctors[h->index].name;
  case HEAD_PROJ:
    return c->m->ctors[h->index].fields[h->field].name;
  case HEAD_BUILTIN:
    return h->builtin->name;
  }
  return "?";
}

static const Core *known_core(Call *k, const Head *h) {
  switch (h->kind) {
  case HEAD_TERM:
    return nul(FAIL(k->c, "INTERNAL", "a plain term reached the core name rules"));
  case HEAD_FAMILY:
    return family_app(k, h->index);
  case HEAD_CTOR:
    return ctor_app(k, h->index);
  case HEAD_PROJ:
    return proj_app(k, h->index, h->field);
  case HEAD_BUILTIN:
    return rule(k);
  }
  return NULL;
}

/* An under-applied core name checked against a function type: WHOLE becomes
   `fun x => WHOLE x`, with a name that the lexer cannot make. */
static const Core *eta(Checker *c, const Term *whole, const Value *hint, const Value **type) {
  char *name = arena_alloc(c->m->arena, 24);
  Term *x = arena_alloc(c->m->arena, sizeof *x);
  Term *app = arena_alloc(c->m->arena, sizeof *app);
  Term *lam = arena_alloc(c->m->arena, sizeof *lam);
  if (name == NULL || x == NULL || app == NULL || lam == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  c->eta_count++;
  snprintf(name, 24, "%%eta%u", c->eta_count);
  x->kind = TERM_VAR;
  x->name = name;
  app->kind = TERM_APP;
  app->left = whole;
  app->right = x;
  lam->kind = TERM_LAM;
  lam->name = name;
  lam->right = app;
  return check_lam(c, lam, hint, type);
}

/* p r, where P has the type PT = PathP A a b and r : I. The ends a and b
   ride in the core, as values carry no types. *TY becomes A r. */
static const Core *path_app(Checker *c, const Core *p, const Value *pt, const Term *arg, const Value **ty) {
  const Core *r = check(c, arg, op0(c, OP_I));
  const Core *args[4];
  if (r == NULL)
    return NULL;
  args[0] = p;
  args[1] = r;
  args[2] = quote_value(c->m, c->level, pt->args[1]);
  args[3] = quote_value(c->m, c->level, pt->args[2]);
  *ty = apply_value(c->m, pt->args[0], here(c, r));
  return core_op(c, OP_PAPP, 0, 0, args, 4);
}

/* Applies K of type *TY to ARG. A path takes a point of I. */
static const Core *apply_one(Checker *c, const Core *k, const Value **ty, const Term *arg) {
  const Value *t = *ty;
  const Core *a;
  if (val_is(t, OP_PATHP))
    return path_app(c, k, t, arg, ty);
  if (t == NULL || t->kind != VAL_PI)
    return nul(FAIL(c, "TYPE_NOT_FUNCTION", "a term of type %s is applied to an argument", show(c, t)));
  a = check(c, arg, t->dom);
  *ty = closure_apply(c->m, t, here(c, a));
  return core_app(c, k, a);
}

/* Applies K of type TY to ARGS, one by one. */
static const Core *apply_rest(Checker *c, const Core *k, const Value *ty, const Term *const *args, uint32_t argc, const Value *hint, const Value **type) {
  uint32_t i;
  for (i = 0; i < argc && k != NULL; i++)
    k = apply_one(c, k, &ty, args[i]);
  *type = ty;
  return finish(c, k, hint, type);
}

static const Core *var_core(Checker *c, const char *name, const Value **type) {
  const Scope *s = find_local(c, name);
  uint32_t d = find_def(c->m, name);
  if (s != NULL) {
    *type = s->type;
    return core_index(c, CORE_VAR, c->level - 1u - s->level);
  }
  if (d != NO_DEF) {
    *type = c->m->defs[d].type;
    return core_index(c, CORE_GLOBAL, d);
  }
  return nul(FAIL(c, "NAME_UNKNOWN", "unknown name %s", name));
}

static const Core *apply_term(Checker *c, const Term *head, const Term *const *args, uint32_t argc, const Value *hint, const Value **type) {
  const Value *ty = NULL;
  const Core *k = head->kind == TERM_VAR ? var_core(c, head->name, &ty) : infer(c, head, &ty);
  return apply_rest(c, k, ty, args, argc, hint, type);
}

static const Core *apply_known(Checker *c, const Term *whole, const Head *h, const Term *const *args, uint32_t argc, const Value *hint, const Value **type) {
  uint32_t arity = known_arity(c, h);
  Call k;
  const Core *core;
  if (argc < arity && hint != NULL && hint->kind == VAL_PI)
    return eta(c, whole, hint, type);
  if (argc < arity)
    return nul(FAIL(c, "TYPE_ARITY", "%s needs %u arguments, found %u", known_name(c, h), arity, argc));
  memset(&k, 0, sizeof k);
  k.c = c;
  k.b = h->builtin;
  k.args = args;
  k.argc = argc;
  k.hint = argc == arity ? hint : NULL;
  k.outer = hint;
  k.used = arity;
  core = known_core(&k, h);
  return apply_rest(c, core, k.type, args + k.used, argc - k.used, hint, type);
}

static const Core *elab_spine(Checker *c, const Term *t, const Value *hint, const Value **type) {
  const Term **args;
  const Term *head = t;
  uint32_t argc = 0;
  uint32_t i;
  Head h;
  while (head->kind == TERM_APP) {
    argc++;
    head = head->left;
  }
  if (argc > SPINE_MAX)
    return nul(FAIL(c, "TYPE_ARITY", "an application has more than %u arguments", SPINE_MAX));
  args = arena_alloc(c->m->arena, ((size_t)argc + 1u) * sizeof *args);
  if (args == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (head = t, i = argc; head->kind == TERM_APP; head = head->left)
    args[--i] = head->right;
  if (!resolve_head(c, head, &h))
    return NULL;
  return h.kind == HEAD_TERM ? apply_term(c, head, args, argc, hint, type) : apply_known(c, t, &h, args, argc, hint, type);
}

static int annotation_ok(Checker *c, const Term *ann, const Value *dom) {
  uint64_t level = 0;
  const Value *v = here(c, check_type_at(c, ann, &level, 1));
  if (v == NULL)
    return 0;
  return convert_at(c, c->level, v, dom) ? 1 : FAIL(c, "TYPE_MISMATCH", "the binder type %s differs from the expected type %s", show(c, v), show(c, dom));
}

/* λ i → t against E = PathP A a b: t checks against A i under i : I, then
   the ends of the path must be a and b. */
static const Core *check_plam(Checker *c, const Term *t, const Value *e, const Value **type) {
  const Core *body;
  const Core *lam;
  const Value *f;
  const Value *at0;
  const Value *at1;
  if (t->left != NULL && !annotation_ok(c, t->left, op0(c, OP_I)))
    return NULL;
  if (!push(c, t->name, op0(c, OP_I)))
    return NULL;
  body = check(c, t->right, apply_value(c->m, e->args[0], c->env->value));
  pop(c);
  lam = core_bind(c, CORE_LAM, t->name, NULL, body);
  f = here(c, lam);
  if (f == NULL)
    return NULL;
  at0 = apply_value(c->m, f, op0(c, OP_I0));
  at1 = apply_value(c->m, f, op0(c, OP_I1));
  if (!convert_at(c, c->level, at0, e->args[1]))
    return nul(FAIL(c, "TYPE_ENDPOINT", "the path at i0 normalizes to %s, the type needs %s", show(c, at0), show(c, e->args[1])));
  if (!convert_at(c, c->level, at1, e->args[2]))
    return nul(FAIL(c, "TYPE_ENDPOINT", "the path at i1 normalizes to %s, the type needs %s", show(c, at1), show(c, e->args[2])));
  *type = e;
  return op3(c, OP_PLAM, 0, 1, lam, NULL, NULL);
}

static const Core *check_lam(Checker *c, const Term *t, const Value *hint, const Value **type) {
  const Core *body;
  if (val_is(hint, OP_PATHP))
    return check_plam(c, t, hint, type);
  if (hint->kind != VAL_PI)
    return nul(FAIL(c, "TYPE_MISMATCH", "a function is not a term of type %s", show(c, hint)));
  if (t->left != NULL && !annotation_ok(c, t->left, hint->dom))
    return NULL;
  if (!push(c, t->name, hint->dom))
    return NULL;
  body = check(c, t->right, closure_apply(c->m, hint, c->env->value));
  pop(c);
  *type = hint;
  return core_bind(c, CORE_LAM, t->name, NULL, body);
}

static const Core *infer_lam(Checker *c, const Term *t, const Value **type) {
  uint64_t level = 0;
  const Value *dom;
  const Value *bt = NULL;
  const Core *body;
  const Core *quoted;
  if (t->left == NULL)
    return nul(FAIL(c, "TYPE_INFER", "λ %s → ... needs a binder type here", t->name));
  dom = here(c, check_type_at(c, t->left, &level, 1));
  if (!push(c, t->name, dom))
    return NULL;
  body = infer(c, t->right, &bt);
  quoted = body == NULL ? NULL : quote_value(c->m, c->level, bt);
  pop(c);
  *type = val_closure(c->m, VAL_PI, t->name, dom, c->env, quoted);
  return core_bind(c, CORE_LAM, t->name, NULL, body);
}

/* (x : A) -> B and Sigma (x : A) B. */
static const Core *elab_binder(Checker *c, const Term *t, CoreKind kind, const Value *hint, const Value **type) {
  uint64_t la = 0;
  uint64_t lb = 0;
  int pretype = kind == CORE_PI; /* a Pi over I is a pretype; a Sigma refuses one */
  const Core *a = check_type_at(c, t->left, &la, pretype);
  const Core *b;
  if (a == NULL || !push(c, t->name, here(c, a)))
    return NULL;
  b = check_type_at(c, t->right, &lb, pretype);
  pop(c);
  *type = la == UINT64_MAX || lb == UINT64_MAX ? op0(c, OP_SSET) : val_univ(c->m, la > lb ? la : lb);
  return finish(c, core_bind(c, kind, t->name, a, b), hint, type);
}

static const Core *elab_inner(Checker *c, const Term *t, const Value *hint, const Value **type) {
  switch (t->kind) {
  case TERM_VAR:
  case TERM_APP:
    return elab_spine(c, t, hint, type);
  case TERM_NAT:
    *type = nat_type(c);
    return finish(c, core_leaf(c, CORE_NAT, t->nat), hint, type);
  case TERM_LAM:
    return hint != NULL ? check_lam(c, t, hint, type) : infer_lam(c, t, type);
  case TERM_PI:
    return elab_binder(c, t, CORE_PI, hint, type);
  case TERM_SIGMA:
    return elab_binder(c, t, CORE_SIGMA, hint, type);
  case TERM_UNIV:
    if (t->nat >= UNIVERSE_MAX)
      return nul(FAIL(c, "TYPE_UNIVERSE", "Type %llu is above the largest universe Type %u", (unsigned long long)t->nat, UNIVERSE_MAX - 1u));
    *type = val_univ(c->m, t->nat + 1u);
    return finish(c, core_leaf(c, CORE_UNIV, t->nat), hint, type);
  }
  return NULL;
}

static const Core *elab(Checker *c, const Term *t, const Value *hint, const Value **type) {
  const Core *k;
  if (c->depth >= CHECK_DEPTH_LIMIT)
    return nul(FAIL(c, "CHECK_DEPTH", "the term is nested deeper than %u levels", CHECK_DEPTH_LIMIT));
  c->depth++;
  k = elab_inner(c, t, hint, type);
  c->depth--;
  return k;
}

/* NULL when NAME is free, else the reason it is taken (rule R4). */
static const char *name_owner(const Checker *c, const char *name) {
  const Machine *m = c->m;
  uint32_t i;
  uint32_t j;
  for (i = 0; i < sizeof RESERVED / sizeof RESERVED[0]; i++) {
    if (strcmp(RESERVED[i], name) == 0)
      return "a reserved name";
  }
  for (i = 0; i < BUILTIN_COUNT; i++) {
    if (strcmp(BUILTINS[i].name, name) == 0)
      return "a core name";
  }
  for (i = 0; i < m->family_count; i++) {
    if (strcmp(m->families[i].name, name) == 0)
      return "a domain family";
  }
  for (i = 0; i < m->ctor_count; i++) {
    for (j = 0; j < m->ctors[i].field_count; j++) {
      if (strcmp(m->ctors[i].fields[j].name, name) == 0)
        return "a domain field";
    }
    if (strcmp(m->ctors[i].name, name) == 0)
      return "a domain constructor";
  }
  i = find_def(m, name);
  if (i == NO_DEF)
    return NULL;
  return m->defs[i].origin == ORIGIN_DOMAIN ? "a domain definition" : "already defined";
}

static int name_free(Checker *c, const char *name) {
  const char *why = name_owner(c, name);
  return why == NULL ? 1 : FAIL(c, "REFUSE_NAME", "%s is %s (rule R4)", name, why);
}

/* T ends in a universe, so a parameter of type T is a type argument. */
static int cell_type_argument(const Core *t) {
  while (t != NULL && t->kind == CORE_PI)
    t = t->b;
  return t != NULL && t->kind == CORE_UNIV;
}

static int has_type_param(const FamilyInfo *f) {
  uint32_t k;
  for (k = 0; k < f->param_count; k++)
    if (cell_type_argument(f->param_types[k]))
      return 1;
  return 0;
}

/* The R1 class of a family, after normalizing its parameter types.
   check_family refuses program base data and type indices. */
static FamilyClass family_class(const Decl *d, const FamilyInfo *f) {
  if (d->origin == ORIGIN_DOMAIN)
    return FAMILY_BASE;
  return has_type_param(f) ? FAMILY_CM : FAMILY_INDEXED;
}

/* A field type is in the context of the family parameters only, so the
   parameter K is the variable with the index param_count - 1 - K. */

/* The type parameter K that T names, else param_count. */
static uint32_t cell_param(const FamilyInfo *f, const Core *t) {
  uint32_t p = f->param_count;
  uint32_t k;
  if (t == NULL || t->kind != CORE_VAR || t->index >= p)
    return p;
  k = p - 1u - t->index;
  return f->param_types[k]->kind == CORE_UNIV ? k : p;
}

/* T is the family FAM at its own parameters, in order, under EARLIER
   fields, then at any indices. */
static int own_family(const Checker *c, uint32_t fam, const Core *t, uint32_t earlier) {
  uint32_t p = c->m->families[fam].param_count;
  uint32_t i;
  if (t == NULL || t->kind != CORE_OP || t->op != OP_FAMILY || t->inst != fam || t->argc != p + c->m->families[fam].index_count)
    return 0;
  for (i = 0; i < p; i++)
    if (t->args[i] == NULL || t->args[i]->kind != CORE_VAR || t->args[i]->index != earlier + p - 1u - i)
      return 0;
  return 1;
}

/* T has no free variable at DEPTH binders, so it names no parameter. */
static int cell_closed(const Core *t, uint32_t depth) {
  uint32_t i;
  if (t == NULL)
    return 1;
  switch (t->kind) {
  case CORE_VAR:
    return t->index < depth;
  case CORE_GLOBAL:
  case CORE_NAT:
  case CORE_TRAP:
  case CORE_UNIV:
    return 1;
  case CORE_LAM:
  case CORE_PI:
  case CORE_SIGMA:
    return cell_closed(t->a, depth) && cell_closed(t->b, depth + 1u);
  case CORE_APP:
    return cell_closed(t->a, depth) && cell_closed(t->b, depth);
  case CORE_OP:
    break;
  }
  for (i = 0; i < t->argc; i++)
    if (!cell_closed(t->args[i], depth))
      return 0;
  return 1;
}

/* A family with a type argument has only universe parameters (rule R1). */
static int cell_header(Checker *c, const Decl *d, const FamilyInfo *f) {
  uint32_t k;
  for (k = 0; k < f->param_count; k++)
    if (f->param_types[k]->kind != CORE_UNIV)
      return FAIL(c, "REFUSE_CELL", "%s has a type argument and the parameter %s is not a universe; a cell signature has only type parameters (rule R1)", f->name, d->params[k].name);
  return 1;
}

/* The fields of a cocell signature (rule R1): the one observation `A`, a
   field `D A`, or a labelled field `E → D A` with E closed. */
typedef enum {
  COCELL_OBSERVATION,
  COCELL_COFIELD,
  COCELL_LABELLED
} CocellField;

/* T is the application of the variable of index F to the variable of
   index A. */
static int cocell_app(const Core *t, uint32_t f, uint32_t a) {
  return t != NULL && t->kind == CORE_APP && t->a->kind == CORE_VAR && t->a->index == f && t->b->kind == CORE_VAR && t->b->index == a;
}

/* The kind of the field J with the normal type T, in the context D, A,
   then the J earlier fields: A is the variable J and D is the variable
   J + 1. A labelled field is an arrow written in the field. */
static int cocell_kind(const Field *field, const Core *t, uint32_t j, CocellField *out) {
  if (t->kind == CORE_VAR && t->index == j) {
    *out = COCELL_OBSERVATION;
    return 1;
  }
  if (cocell_app(t, j + 1u, j)) {
    *out = COCELL_COFIELD;
    return 1;
  }
  if (field->type->kind == TERM_PI && t->kind == CORE_PI && cell_closed(t->a, 0u) && cocell_app(t->b, j + 2u, j + 1u)) {
    *out = COCELL_LABELLED;
    return 1;
  }
  return 0;
}

/* The shape check of `codata D (A : Type n) : Type where` and its one
   clause `c : fields → D A` (rule R1): one type parameter, no index, one
   clause that is not a path constructor, and each field a CocellField,
   with exactly one observation. The fields are checked with
   D : Type n → Type n and A as locals. The universe of the parameter goes
   to *N, the field kinds to KINDS and the universe of each label type to
   LABELS, with its closed normal form in LABEL_TYPES. */
static int cocell_shape(Checker *c, const Decl *d, uint64_t *n, CocellField *kinds, uint64_t *labels, const Core **label_types) {
  Machine *m = c->m;
  const Ctor *ctor = d->ctors;
  Term *arrow = arena_alloc(m->arena, sizeof *arrow);
  const Core *pt, *pn, *dt;
  uint64_t level = 0;
  uint32_t j, observations = 0;
  if (arrow == NULL)
    return FAIL(c, "OOM", "out of memory");
  if (d->param_count != 1u)
    return FAIL(c, "REFUSE_CELL", "%s is not a cocell signature: it has %u parameters; a cocell signature has one type parameter (rule R1)", d->name, (unsigned)d->param_count);
  if (d->index_count != 0u)
    return FAIL(c, "REFUSE_CELL", "%s is not a cocell signature: it has an index (rule R1)", d->name);
  if (d->ctor_count != 1u || ctor->result != NULL)
    return FAIL(c, "REFUSE_CELL", "%s is not a cocell signature: it must have one clause, and the clause is not a path constructor (rule R1)", d->name);
  pt = check_type(c, d->params[0].type, &level);
  if (pt == NULL)
    return 0;
  pn = quote_value(m, c->level, here(c, pt));
  if (pn == NULL)
    return FAIL(c, "OOM", "out of memory");
  if (pn->kind != CORE_UNIV)
    return FAIL(c, "REFUSE_CELL", "%s is not a cocell signature: the parameter %s is not a universe; a cocell signature has one type parameter (rule R1)", d->name, d->params[0].name);
  *n = pn->nat;
  memset(arrow, 0, sizeof *arrow);
  arrow->kind = TERM_PI;
  arrow->line = d->line;
  arrow->col = d->col;
  arrow->left = d->params[0].type;
  arrow->right = d->params[0].type;
  dt = check_type(c, arrow, &level);
  if (dt == NULL || !push(c, d->name, here(c, dt)) || !push(c, d->params[0].name, here(c, pt)))
    return 0;
  for (j = 0; j < ctor->field_count; j++) {
    const Field *field = &ctor->fields[j];
    const Core *k = check_type(c, field->type, &level);
    const Core *t = k == NULL ? NULL : quote_value(m, c->level, here(c, k));
    if (k == NULL)
      return 0;
    if (t == NULL)
      return FAIL(c, "OOM", "out of memory");
    if (!cocell_kind(field, t, j, &kinds[j]))
      return FAIL(c, "REFUSE_CELL", "%s is not a cocell signature: the field %s must be the observation %s, a field %s %s or a labelled field E → %s %s with E closed (rule R1)",
                  d->name, field->name, d->params[0].name, d->name, d->params[0].name, d->name, d->params[0].name);
    labels[j] = 0;
    if (kinds[j] == COCELL_LABELLED && check_type(c, field->type->left, &labels[j]) == NULL)
      return 0;
    if (labels[j] > *n)
      return FAIL(c, "REFUSE_CELL", "%s is not a cocell signature: the label type of %s is in Type %llu, above Type %llu of %s (rule R1)",
                  d->name, field->name, (unsigned long long)labels[j], (unsigned long long)*n, d->params[0].name);
    /* cocell_kind proved this domain closed. Keep its checked normal form
       and do not resolve the source again in a different scope. */
    label_types[j] = kinds[j] == COCELL_LABELLED ? t->a : NULL;
    observations += kinds[j] == COCELL_OBSERVATION;
    if (!push(c, field->name, here(c, k)))
      return 0;
  }
  if (observations != 1u)
    return FAIL(c, "REFUSE_CELL", "%s is not a cocell signature: it has %u observations; a cocell signature has exactly one (rule R1)", d->name, (unsigned)observations);
  for (j = 0; j < ctor->field_count; j++) {
    const char *name = ctor->fields[j].name;
    uint32_t earlier;
    if (strcmp(name, d->name) == 0 || strcmp(name, ctor->name) == 0)
      return FAIL(c, "REFUSE_NAME", "%s is the codata type name or the clause name (rule R4)", name);
    for (earlier = 0; earlier < j; earlier++)
      if (strcmp(name, ctor->fields[earlier].name) == 0)
        return FAIL(c, "REFUSE_NAME", "%s is already a field of this clause (rule R4)", name);
    if (!name_free(c, name))
      return 0;
  }
  if (strcmp(ctor->name, d->name) == 0)
    return FAIL(c, "REFUSE_NAME", "%s is the codata type name (rule R4)", ctor->name);
  return name_free(c, ctor->name);
}

/* An index type that ends in a universe is a type index. A cell signature
   binds its type arguments as uniform parameters (rule R1). */
static int type_index(Checker *c, const FamilyInfo *f) {
  uint32_t k;
  for (k = 0; k < f->index_count; k++) {
    const Value *type = here(c, f->index_types[k]);
    const Core *normal = type == NULL ? NULL : quote_value(c->m, c->level, type);
    if (normal == NULL)
      return 0;
    if (cell_type_argument(normal))
      return FAIL(c, "REFUSE_CELL", "%s has a type index; a cell signature binds its type arguments as uniform parameters (rule R1)", f->name);
  }
  return 1;
}

/* The body of the line of a path type, under one binder; NULL when T is
   not a path type. */
static const Core *path_body(const Core *t) {
  return t != NULL && t->kind == CORE_OP && t->op == OP_PATHP && t->argc == 3u && t->args[0] != NULL && t->args[0]->kind == CORE_LAM ? t->args[0]->b : NULL;
}

/* A field of a cell monad: a type parameter (the one field of its
   constructor), closed data, the family at its own parameters, or a path
   in the family (under EARLIER fields). */
static int cell_field(Checker *c, uint32_t fam, const FieldInfo *out, size_t field_count, uint32_t earlier) {
  const FamilyInfo *f = &c->m->families[fam];
  const Core *t = out->type;
  const Core *line = path_body(t);
  if (t != NULL && t->kind == CORE_VAR && t->index >= earlier && t->index - earlier < f->param_count)
    return field_count == 1u ? 1 : FAIL(c, "REFUSE_CELL", "%s is not a cell signature: the field %s has a type parameter as its type, so it must be the one field of its constructor (rule R1)", f->name, out->name);
  if (own_family(c, fam, t, earlier) || (line != NULL && own_family(c, fam, line, earlier + 1u)) || (c->self_uses == 0u && cell_closed(t, earlier)))
    return 1;
  return FAIL(c, "REFUSE_CELL", "%s is not a cell signature: the field %s must be a type parameter, closed data, %s at its own parameters or a path in %s (rule R1)", f->name, out->name, f->name, f->name);
}

/* Each type parameter has one generator: one constructor with one field
   of that type (rule R1). */
static int cell_generators(Checker *c, const Decl *d, uint32_t fam) {
  const Machine *m = c->m;
  const FamilyInfo *f = &m->families[fam];
  uint32_t k;
  for (k = 0; k < f->param_count; k++) {
    unsigned count = 0;
    uint32_t j;
    for (j = 0; j < f->ctor_count; j++) {
      const CtorInfo *ci = &m->ctors[f->first_ctor + j];
      count += ci->depth == 0u && ci->field_count == 1u && cell_param(f, ci->fields[0].type) == k;
    }
    if (count == 0u)
      return FAIL(c, "REFUSE_CELL", "%s is not a cell signature: the type parameter %s occurs in no constructor (rule R1)", f->name, d->params[k].name);
    if (count > 1u)
      return FAIL(c, "REFUSE_CELL", "%s is not a cell signature: the type parameter %s has %u generators (rule R1)", f->name, d->params[k].name, count);
  }
  return 1;
}

/* A point term over the N fields (under DEPTH interval binders): a
   recursive field, or a point constructor of FAM at the params with point
   terms as its recursive arguments and fields as its other arguments. */
static int point_term(const Checker *c, uint32_t fam, const FieldInfo *fields, uint32_t n, const Core *k, uint32_t depth) {
  const Machine *m = c->m;
  uint32_t p = m->families[fam].param_count;
  const CtorInfo *ci;
  uint32_t i;
  if (k != NULL && k->kind == CORE_VAR)
    return k->index >= depth && k->index - depth < n && fields[n - 1u - (k->index - depth)].recursive;
  if (k == NULL || k->kind != CORE_OP || k->op != OP_CTOR || k->inst >= m->ctor_count)
    return 0;
  ci = &m->ctors[k->inst];
  if (ci->family != fam || ci->depth != 0u || k->argc != p + ci->field_count)
    return 0;
  for (i = 0; i < p; i++)
    if (k->args[i] == NULL || k->args[i]->kind != CORE_VAR || k->args[i]->index != depth + n + p - 1u - i)
      return 0;
  for (i = 0; i < ci->field_count; i++) {
    const Core *x = k->args[p + i];
    int ok = ci->fields[i].recursive ? point_term(c, fam, fields, n, x, depth)
                                     : x != NULL && x->kind == CORE_VAR && x->index >= depth && x->index - depth < n;
    if (!ok)
      return 0;
  }
  return 1;
}

/* A path constructor: its type is Path T s t, where T is the family at its
   params and s, t are point terms (depth 1), or Path (Path T x y) p q, where
   p, q are path fields (depth 2). A 3-cell gets REFUSE_CELL. The ends x, y
   of a square leave the line binder by evaluation. The fields are pushed. */
static int check_pctor(Checker *c, uint32_t fam, const Ctor *ctor, const FieldInfo *fields, CtorInfo *ci) {
  Machine *m = c->m;
  const char *name = m->families[fam].name;
  uint32_t n = (uint32_t)ctor->field_count;
  uint64_t level = 0;
  const Core *t = check_type(c, ctor->result, &level);
  const Core *k = t;
  const Core *inner = path_body(t);
  const Core **ends = arena_alloc(m->arena, 5u * sizeof *ends);
  uint32_t depth = 0;
  uint32_t i;
  if (t == NULL)
    return 0;
  if (ends == NULL)
    return FAIL(c, "OOM", "out of memory");
  while (k != NULL && !own_family(c, fam, k, n + depth)) {
    k = path_body(k);
    depth++;
  }
  if (k == NULL || depth == 0u)
    return FAIL(c, "REFUSE_CELL", "%s is not a cell signature: the path constructor %s must have the type Path T s t, where T is %s at its own parameters (rule R1)", name, ctor->name, name);
  if (depth > 2u)
    return FAIL(c, "REFUSE_CELL", "%s is not a cell signature: the path constructor %s has dimension %u; only 1 and 2 are available (rule R1)", name, ctor->name, (unsigned)depth);
  ends[0] = t->args[1];
  ends[1] = t->args[2];
  if (depth == 1u && !(point_term(c, fam, fields, n, ends[0], 0u) && point_term(c, fam, fields, n, ends[1], 0u)))
    return FAIL(c, "REFUSE_CELL", "%s is not a cell signature: an end of the path constructor %s must be a point term (rule R1)", name, ctor->name);
  for (i = 0; depth == 2u && i < 2u; i++)
    if (ends[i] == NULL || ends[i]->kind != CORE_VAR || ends[i]->index >= n)
      return FAIL(c, "REFUSE_CELL", "%s is not a cell signature: the ends p, q of the square %s must be fields of the constructor (rule R1)", name, ctor->name);
  for (i = 0; depth == 2u && i < 2u; i++) {
    const Value *v;
    if (!point_term(c, fam, fields, n, inner->args[1u + i], 1u))
      return FAIL(c, "REFUSE_CELL", "%s is not a cell signature: an end of the path constructor %s must be a point term (rule R1)", name, ctor->name);
    if (!push(c, "_", val_op(m, VAL_OP, OP_I, 0, 0, NULL, 0)))
      return 0;
    v = here(c, inner->args[1u + i]);
    pop(c);
    ends[2u + i] = quote_value(m, c->level, v);
  }
  ci->depth = depth;
  ci->type = t;
  ci->ends = ends;
  return 1;
}

/* A recursive field must be the whole field type: the family at its own
   parameters (under EARLIER fields), then at any indices. A field of a cell
   monad must pass cell_field first. */
static int check_field(Checker *c, uint32_t fam, const Field *field, size_t field_count, uint32_t earlier, FieldInfo *out) {
  uint64_t level = 0;
  const Core *t;
  if (!name_free(c, field->name))
    return 0;
  c->self_uses = 0;
  t = check_type(c, field->type, &level);
  if (t == NULL)
    return 0;
  if (c->m->families[fam].r1 == FAMILY_CM) {
    t = quote_value(c->m, c->level, here(c, t));
    if (t == NULL)
      return 0;
  }
  if (level > c->m->families[fam].universe)
    c->m->families[fam].universe = level;
  out->name = field->name;
  out->type = t;
  out->recursive = c->self_uses > 0u;
  if (c->m->families[fam].r1 == FAMILY_CM && !cell_field(c, fam, out, field_count, earlier))
    return 0;
  if (c->m->families[fam].r1 == FAMILY_CM && own_family(c, fam, path_body(t), earlier + 1u))
    out->recursive = 0;
  if (out->recursive && !own_family(c, fam, t, earlier))
    return FAIL(c, "DOMAIN_FAMILY", "the recursive field %s must have the type %s itself, at the parameters of the family", field->name, c->m->families[fam].name);
  return 1;
}

/* In a telescope each field is in scope in the later fields and in the
   result indices. */
static int check_ctor(Checker *c, uint32_t fam, const Ctor *ctor) {
  Machine *m = c->m;
  const FamilyInfo *f = &m->families[fam];
  int dep = telescope(f) || ctor->result != NULL;
  FieldInfo *fields = arena_alloc(m->arena, (ctor->field_count + 1u) * sizeof *fields);
  const Core **indices = arena_alloc(m->arena, (f->index_count + 1u) * sizeof *indices);
  const Value **itypes = arena_alloc(m->arena, (f->index_count + 1u) * sizeof *itypes);
  CtorInfo *ci = &m->ctors[m->ctor_count];
  size_t j;
  if (fields == NULL || indices == NULL || itypes == NULL)
    return FAIL(c, "OOM", "out of memory");
  if (!name_free(c, ctor->name))
    return 0;
  if (ctor->result != NULL && f->r1 != FAMILY_CM)
    return FAIL(c, "DOMAIN_FAMILY", "the path constructor %s is in S2 only in a cell signature (rule R1)", ctor->name);
  for (j = 0; j < f->index_count; j++)
    itypes[j] = here(c, f->index_types[j]);
  for (j = 0; j < ctor->field_count; j++) {
    size_t earlier;
    const char *name = ctor->fields[j].name;
    if (strcmp(name, ctor->name) == 0)
      return FAIL(c, "REFUSE_NAME", "%s is the constructor name (rule R4)", name);
    for (earlier = 0; earlier < j; earlier++)
      if (strcmp(name, ctor->fields[earlier].name) == 0)
        return FAIL(c, "REFUSE_NAME", "%s is already a field of this constructor (rule R4)", name);
    if (!check_field(c, fam, &ctor->fields[j], ctor->field_count, dep ? (uint32_t)j : 0u, &fields[j]))
      return 0;
    if (dep && !push(c, name, here(c, fields[j].type)))
      return 0;
  }
  for (j = 0; j < f->index_count; j++) {
    indices[j] = check(c, ctor->indices[j], itypes[j]);
    if (indices[j] == NULL)
      return 0;
  }
  ci->depth = 0;
  ci->type = NULL;
  ci->ends = NULL;
  if (ctor->result != NULL && !check_pctor(c, fam, ctor, fields, ci))
    return 0;
  for (j = 0; dep && j < ctor->field_count; j++)
    pop(c);
  ci->name = ctor->name;
  ci->family = fam;
  ci->fields = fields;
  ci->field_count = (uint32_t)ctor->field_count;
  ci->indices = indices;
  m->ctor_count++;
  return 1;
}

/* A family is in scope in its own fields, so a field can be recursive. A
   program family with a type parameter must be a cell signature. */
static int check_family(Checker *c, const Decl *d) {
  Machine *m = c->m;
  uint32_t fam = m->family_count;
  FamilyInfo *f = &m->families[fam];
  const Core **params = arena_alloc(m->arena, (d->param_count + 1u) * sizeof *params);
  const Core **indices = arena_alloc(m->arena, (d->index_count + 1u) * sizeof *indices);
  size_t i;
  if (params == NULL || indices == NULL)
    return FAIL(c, "OOM", "out of memory");
  for (i = 0; i < d->param_count; i++) {
    uint64_t level = 0;
    const Core *type = check_type(c, d->params[i].type, &level);
    params[i] = type == NULL ? NULL : quote_value(m, c->level, here(c, type));
    if (params[i] == NULL || !push(c, d->params[i].name, here(c, params[i])))
      return 0;
  }
  for (i = 0; i < d->index_count; i++) {
    uint64_t level = 0;
    indices[i] = check_type(c, d->indices[i], &level);
    if (indices[i] == NULL)
      return 0;
  }
  f->name = d->name;
  f->param_types = params;
  f->param_count = (uint32_t)d->param_count;
  f->index_types = indices;
  f->index_count = (uint32_t)d->index_count;
  f->first_ctor = m->ctor_count;
  f->ctor_count = (uint32_t)d->ctor_count;
  f->r1 = family_class(d, f);
  m->family_count++;
  if (!type_index(c, f))
    return 0;
  if (d->origin == ORIGIN_PROGRAM && f->r1 != FAMILY_CM)
    return FAIL(c, "REFUSE_DATA", "a program cannot declare data with no type argument; a base type comes only from domain/domain.lang (rule R2)");
  if (f->r1 == FAMILY_CM && !cell_header(c, d, f))
    return 0;
  c->self = fam;
  for (i = 0; i < d->ctor_count; i++) {
    if (!check_ctor(c, fam, &d->ctors[i]))
      return 0;
  }
  return f->r1 != FAMILY_CM || cell_generators(c, d, fam);
}

static int check_def(Checker *c, const Decl *d) {
  Machine *m = c->m;
  uint64_t level = 0;
  /* A Pi over I (a pretype) is a legal signature, as in Cubical Agda: a
     line, or a family of partial elements. A bare pretype is not. */
  const Core *t = check_type_at(c, d->type, &level, d->type->kind == TERM_PI);
  const Value *tv = here(c, t);
  const Core *body = check(c, d->body, tv);
  DefInfo *info = &m->defs[m->def_count];
  if (body == NULL)
    return 0;
  info->name = d->name;
  info->origin = d->origin;
  info->body = body;
  info->type = tv;
  info->value = NULL;
  m->def_count++;
  return 1;
}

/* The declarations that a codata D (A : Type n) with the clause c gives
   (S2c U3). They are AST terms at the position of D, checked as domain
   declarations, so family_class gives D_pos FAMILY_BASE:
   - D_f_E : Type k, the label type of each labelled field f;
   - data D_pos : Type where D_here, D_f (rest : D_pos) for a field D A,
     D_f (label : D_f_E) (rest : D_pos) for a labelled field;
   - D : Type n → Type n, D A = (p : D_pos) → A;
   - the destructor f of each field;
   - D_walk, the fold over a position that applies the steps;
   - the corecursor c : (A S : Type n) → (S → A) → steps → S → D A.
   The ctor field names have a `.`, so a program cannot name them. */
typedef struct {
  Checker *c;
  const Decl *d;
  const CocellField *kinds;
  uint32_t count;
  uint64_t n;
  const char *pos, *here, *walk;
  const char **ctor;  /* D_f, NULL for the observation */
  const char **etype; /* D_f_E, NULL for a field with no label */
  const char **step;  /* t_f, NULL for the observation */
  const char *A, *S, *o, *seed, *p, *s, *ih, *e, *x;
} Cogen;

static Term *cg_node(const Cogen *g, TermKind kind, const char *name, const Term *left, const Term *right) {
  Term *t = arena_alloc(g->c->m->arena, sizeof *t);
  if (t == NULL)
    return NULL;
  memset(t, 0, sizeof *t);
  t->kind = kind;
  t->line = g->d->line;
  t->col = g->d->col;
  t->name = name;
  t->left = left;
  t->right = right;
  return t;
}

static const Term *cg_var(const Cogen *g, const char *name) {
  return name == NULL ? NULL : cg_node(g, TERM_VAR, name, NULL, NULL);
}

static const Term *cg_app(const Cogen *g, const Term *f, const Term *a) {
  return f == NULL || a == NULL ? NULL : cg_node(g, TERM_APP, NULL, f, a);
}

static const Term *cg_lam(const Cogen *g, const char *x, const Term *type, const Term *body) {
  return x == NULL || type == NULL || body == NULL ? NULL : cg_node(g, TERM_LAM, x, type, body);
}

/* X is NULL for an arrow. */
static const Term *cg_pi(const Cogen *g, const char *x, const Term *dom, const Term *cod) {
  return dom == NULL || cod == NULL ? NULL : cg_node(g, TERM_PI, x, dom, cod);
}

static const Term *cg_univ(const Cogen *g, uint64_t n) {
  Term *t = cg_node(g, TERM_UNIV, NULL, NULL, NULL);
  if (t != NULL)
    t->nat = n;
  return t;
}

static const char *cg_join(const Cogen *g, const char *x, const char *y) {
  size_t lx, ly;
  char *s;
  if (x == NULL || y == NULL)
    return NULL;
  lx = strlen(x);
  ly = strlen(y);
  s = arena_alloc(g->c->m->arena, lx + ly + 1u);
  if (s == NULL)
    return NULL;
  memcpy(s, x, lx);
  memcpy(s + lx, y, ly);
  s[lx + ly] = '\0';
  return s;
}

/* The binder BASE, primes, SUFFIX that is not D and does not start with
   D_, so it does not hide a generated global. */
static const char *cg_binder(const Cogen *g, const char *base, const char *suffix) {
  size_t ld = strlen(g->d->name);
  const char *b = cg_join(g, base, suffix);
  while (b != NULL && (strcmp(b, g->d->name) == 0 || (strncmp(b, g->d->name, ld) == 0 && b[ld] == '_'))) {
    base = cg_join(g, base, "'");
    b = cg_join(g, base, suffix);
  }
  return b;
}

static int cg_names(Cogen *g) {
  const Ctor *ctor = g->d->ctors;
  const char *d = g->d->name;
  Arena *a = g->c->m->arena;
  uint32_t j;
  g->ctor = arena_alloc(a, (g->count + 1u) * sizeof *g->ctor);
  g->etype = arena_alloc(a, (g->count + 1u) * sizeof *g->etype);
  g->step = arena_alloc(a, (g->count + 1u) * sizeof *g->step);
  if (g->ctor == NULL || g->etype == NULL || g->step == NULL)
    return 0;
  for (j = 0; j < g->count; j++) {
    int cofield = g->kinds[j] != COCELL_OBSERVATION;
    g->ctor[j] = cofield ? cg_join(g, cg_join(g, d, "_"), ctor->fields[j].name) : NULL;
    g->etype[j] = g->kinds[j] == COCELL_LABELLED ? cg_join(g, g->ctor[j], "_E") : NULL;
    g->step[j] = cofield ? cg_binder(g, "t", cg_join(g, "_", ctor->fields[j].name)) : NULL;
    if ((cofield && (g->ctor[j] == NULL || g->step[j] == NULL)) || (g->kinds[j] == COCELL_LABELLED && g->etype[j] == NULL))
      return 0;
  }
  g->pos = cg_join(g, d, "_pos");
  g->here = cg_join(g, d, "_here");
  g->walk = cg_join(g, d, "_walk");
  g->A = cg_binder(g, "A", "");
  g->S = cg_binder(g, "S", "");
  g->o = cg_binder(g, "o", "");
  g->seed = cg_binder(g, "seed", "");
  g->p = cg_binder(g, "p", "");
  g->s = cg_binder(g, "s", "");
  g->ih = cg_binder(g, "ih", "");
  g->e = cg_binder(g, "e", "");
  g->x = cg_binder(g, "x", "");
  return g->pos != NULL && g->here != NULL && g->walk != NULL && g->A != NULL && g->S != NULL && g->o != NULL && g->seed != NULL && g->p != NULL && g->s != NULL && g->ih != NULL && g->e != NULL && g->x != NULL;
}

/* Before each generated declaration: the state of check_program for a new
   declaration (cocell_shape pushed the parameter and the fields), then
   the name check. */
static int cg_start(const Cogen *g, const char *name) {
  Checker *c = g->c;
  c->scope = NULL;
  c->env = NULL;
  c->level = 0;
  c->depth = 0;
  c->self = NO_FAMILY;
  c->m->def = name;
  return name == NULL ? FAIL(c, "OOM", "out of memory") : name_free(c, name);
}

static int cg_def(const Cogen *g, const char *name, const Term *type, const Term *body) {
  Decl d;
  if (type == NULL || body == NULL)
    return FAIL(g->c, "OOM", "out of memory");
  if (!cg_start(g, name))
    return 0;
  memset(&d, 0, sizeof d);
  d.kind = DECL_DEF;
  d.origin = ORIGIN_DOMAIN;
  d.line = d.end_line = g->d->line;
  d.col = d.end_col = g->d->col;
  d.name = name;
  d.type = type;
  d.body = body;
  return check_def(g->c, &d);
}

/* The label type D_f_E, the twin of intent-lang check_label: its closed
   normal form from cocell_shape, registered directly. */
static int cg_label(const Cogen *g, uint32_t j, uint64_t level, const Core *type) {
  Machine *m = g->c->m;
  DefInfo *info = &m->defs[m->def_count];
  const Value *u = val_univ(m, level);
  if (!cg_start(g, g->etype[j]))
    return 0;
  if (u == NULL)
    return FAIL(g->c, "OOM", "out of memory");
  info->name = g->etype[j];
  info->origin = ORIGIN_DOMAIN;
  info->body = type;
  info->type = u;
  info->value = NULL;
  m->def_count++;
  return 1;
}

static int cg_pos(const Cogen *g) {
  Arena *a = g->c->m->arena;
  Ctor *ctors = arena_alloc(a, (g->count + 1u) * sizeof *ctors);
  const Term *pos = cg_var(g, g->pos);
  Decl d;
  uint32_t j, k = 1;
  if (ctors == NULL || pos == NULL)
    return FAIL(g->c, "OOM", "out of memory");
  memset(ctors, 0, (g->count + 1u) * sizeof *ctors);
  ctors[0].name = g->here;
  ctors[0].line = g->d->line;
  ctors[0].col = g->d->col;
  for (j = 0; j < g->count; j++) {
    int labelled = g->kinds[j] == COCELL_LABELLED;
    Field *fields;
    if (g->kinds[j] == COCELL_OBSERVATION)
      continue;
    fields = arena_alloc(a, 2u * sizeof *fields);
    if (fields == NULL || (labelled && (fields[0].type = cg_var(g, g->etype[j])) == NULL))
      return FAIL(g->c, "OOM", "out of memory");
    /* Field names are global, so each position constructor needs its own. */
    fields[labelled].name = cg_join(g, g->ctor[j], ".rest");
    if (fields[labelled].name == NULL)
      return FAIL(g->c, "OOM", "out of memory");
    if (labelled) {
      fields[0].name = cg_join(g, g->ctor[j], ".label");
      if (fields[0].name == NULL)
        return FAIL(g->c, "OOM", "out of memory");
    }
    fields[labelled].type = pos;
    fields[0].line = fields[1].line = g->d->line;
    fields[0].col = fields[1].col = g->d->col;
    ctors[k].name = g->ctor[j];
    ctors[k].fields = fields;
    ctors[k].field_count = labelled ? 2u : 1u;
    ctors[k].line = g->d->line;
    ctors[k].col = g->d->col;
    k++;
  }
  if (!cg_start(g, g->pos))
    return 0;
  memset(&d, 0, sizeof d);
  d.kind = DECL_FAMILY;
  d.origin = ORIGIN_DOMAIN;
  d.line = d.end_line = g->d->line;
  d.col = d.end_col = g->d->col;
  d.name = g->pos;
  d.type = cg_univ(g, 0);
  d.ctors = ctors;
  d.ctor_count = k;
  return d.type == NULL ? FAIL(g->c, "OOM", "out of memory") : check_family(g->c, &d);
}

/* D A, for the binder A. */
static const Term *cg_carrier_at(const Cogen *g) {
  return cg_app(g, cg_var(g, g->d->name), cg_var(g, g->A));
}

/* D : (A : Type n) → Type n, the body λ (A : Type n) → (p : D_pos) → A. */
static int cg_carrier(const Cogen *g) {
  return cg_def(g, g->d->name, cg_pi(g, g->A, cg_univ(g, g->n), cg_univ(g, g->n)),
                cg_lam(g, g->A, cg_univ(g, g->n), cg_pi(g, g->p, cg_var(g, g->pos), cg_var(g, g->A))));
}

/* The destructor of the field J: the observation x D_here, the field
   λ p → x (D_f p), the labelled field λ e → λ p → x (D_f e p). */
static int cg_dtor(const Cogen *g, uint32_t j) {
  const char *name = g->d->ctors[0].fields[j].name;
  const Term *u = cg_univ(g, g->n);
  const Term *at = cg_carrier_at(g);
  const Term *pos = cg_var(g, g->pos);
  const Term *x = cg_var(g, g->x);
  const Term *p = cg_var(g, g->p);
  const Term *type = NULL, *body = NULL;
  switch (g->kinds[j]) {
  case COCELL_OBSERVATION:
    type = cg_pi(g, g->x, at, cg_var(g, g->A));
    body = cg_lam(g, g->x, at, cg_app(g, x, cg_var(g, g->here)));
    break;
  case COCELL_COFIELD:
    type = cg_pi(g, g->x, at, at);
    body = cg_lam(g, g->x, at, cg_lam(g, g->p, pos, cg_app(g, x, cg_app(g, cg_var(g, g->ctor[j]), p))));
    break;
  case COCELL_LABELLED:
    type = cg_pi(g, g->x, at, cg_pi(g, g->e, cg_var(g, g->etype[j]), at));
    body = cg_lam(g, g->x, at, cg_lam(g, g->e, cg_var(g, g->etype[j]), cg_lam(g, g->p, pos, cg_app(g, x, cg_app(g, cg_app(g, cg_var(g, g->ctor[j]), cg_var(g, g->e)), p)))));
    break;
  }
  return cg_def(g, name, cg_pi(g, g->A, u, type), cg_lam(g, g->A, u, body));
}

/* S → S. */
static const Term *cg_endo(const Cogen *g) {
  return cg_pi(g, NULL, cg_var(g, g->S), cg_var(g, g->S));
}

/* The type of the step of the field J: S → S, or S → D_f_E → S. */
static const Term *cg_step_type(const Cogen *g, uint32_t j) {
  return g->kinds[j] == COCELL_LABELLED ? cg_pi(g, NULL, cg_var(g, g->S), cg_pi(g, NULL, cg_var(g, g->etype[j]), cg_var(g, g->S))) : cg_endo(g);
}

/* The step binders from the field J on, around INNER: a Pi telescope, or
   lambdas when LAM. */
static const Term *cg_steps(const Cogen *g, uint32_t j, const Term *inner, int lam) {
  const Term *rest;
  if (j == g->count)
    return inner;
  rest = cg_steps(g, j + 1u, inner, lam);
  if (g->kinds[j] == COCELL_OBSERVATION)
    return rest;
  return lam ? cg_lam(g, g->step[j], cg_step_type(g, j), rest) : cg_pi(g, g->step[j], cg_step_type(g, j), rest);
}

/* The fold case of the field J: the step first, then the rest of the
   walk (tailLaw by refl needs this order). */
static const Term *cg_case(const Cogen *g, uint32_t j) {
  const Term *s = cg_var(g, g->s);
  const Term *t = cg_app(g, cg_var(g, g->step[j]), s);
  const Term *k = cg_lam(g, g->ih, cg_endo(g), cg_lam(g, g->s, cg_var(g, g->S), cg_app(g, cg_var(g, g->ih), g->kinds[j] == COCELL_LABELLED ? cg_app(g, t, cg_var(g, g->e)) : t)));
  return g->kinds[j] == COCELL_LABELLED ? cg_lam(g, g->e, cg_var(g, g->etype[j]), k) : k;
}

/* D_walk : (S : Type n) → steps → (p : D_pos) → S → S, the body
   λ S steps p → fold (λ (s : S) → s) cases p. */
static int cg_walk(const Cogen *g) {
  const Term *u = cg_univ(g, g->n);
  const Term *f = cg_app(g, cg_var(g, "fold"), cg_lam(g, g->s, cg_var(g, g->S), cg_var(g, g->s)));
  uint32_t j;
  for (j = 0; j < g->count; j++)
    if (g->kinds[j] != COCELL_OBSERVATION)
      f = cg_app(g, f, cg_case(g, j));
  f = cg_app(g, f, cg_var(g, g->p));
  return cg_def(g, g->walk, cg_pi(g, g->S, u, cg_steps(g, 0, cg_pi(g, g->p, cg_var(g, g->pos), cg_endo(g)), 0)),
                cg_lam(g, g->S, u, cg_steps(g, 0, cg_lam(g, g->p, cg_var(g, g->pos), f), 1)));
}

/* The corecursor c : (A : Type n) → (S : Type n) → (o : S → A) → steps →
   (seed : S) → D A, the body λ A S o steps seed → λ (p : D_pos) →
   o (D_walk S steps p seed). */
static int cg_corec(const Cogen *g) {
  const Term *u = cg_univ(g, g->n);
  const Term *ot = cg_pi(g, NULL, cg_var(g, g->S), cg_var(g, g->A));
  const Term *w = cg_app(g, cg_var(g, g->walk), cg_var(g, g->S));
  const Term *body;
  uint32_t j;
  for (j = 0; j < g->count; j++)
    if (g->kinds[j] != COCELL_OBSERVATION)
      w = cg_app(g, w, cg_var(g, g->step[j]));
  w = cg_app(g, cg_app(g, w, cg_var(g, g->p)), cg_var(g, g->seed));
  body = cg_lam(g, g->seed, cg_var(g, g->S), cg_lam(g, g->p, cg_var(g, g->pos), cg_app(g, cg_var(g, g->o), w)));
  return cg_def(g, g->d->ctors[0].name,
                cg_pi(g, g->A, u, cg_pi(g, g->S, u, cg_pi(g, g->o, ot, cg_steps(g, 0, cg_pi(g, g->seed, cg_var(g, g->S), cg_carrier_at(g)), 0)))),
                cg_lam(g, g->A, u, cg_lam(g, g->S, u, cg_lam(g, g->o, ot, cg_steps(g, 0, body, 1)))));
}

/* A codata declaration (rule R1): the CC criterion, then the generated
   declarations in the order of Cogen. */
static int check_cofamily(Checker *c, const Decl *d) {
  Machine *m = c->m;
  size_t count = d->ctor_count == 1u ? d->ctors[0].field_count : 0u;
  CocellField *kinds = arena_alloc(m->arena, (count + 1u) * sizeof *kinds);
  uint64_t *labels = arena_alloc(m->arena, (count + 1u) * sizeof *labels);
  const Core **label_types = arena_alloc(m->arena, (count + 1u) * sizeof *label_types);
  Cogen g;
  uint32_t j;
  if (kinds == NULL || labels == NULL || label_types == NULL)
    return FAIL(c, "OOM", "out of memory");
  memset(&g, 0, sizeof g);
  if (!cocell_shape(c, d, &g.n, kinds, labels, label_types))
    return 0;
  g.c = c;
  g.d = d;
  g.kinds = kinds;
  g.count = (uint32_t)count;
  if (!cg_names(&g))
    return FAIL(c, "OOM", "out of memory");
  for (j = 0; j < g.count; j++)
    if (kinds[j] == COCELL_LABELLED && !cg_label(&g, j, labels[j], label_types[j]))
      return 0;
  if (!cg_pos(&g) || !cg_carrier(&g))
    return 0;
  for (j = 0; j < g.count; j++)
    if (!cg_dtor(&g, j))
      return 0;
  return cg_walk(&g) && cg_corec(&g);
}

static int check_decl(Checker *c, const Decl *d) {
  if (d->kind == DECL_AXIOM)
    return FAIL(c, "REFUSE_AXIOM", "an axiom has no proof (rule R2)");
  if (d->is_rec)
    return FAIL(c, "REFUSE_REC", "a rec declaration is refused (rule R3); use fold");
  if (!name_free(c, d->name))
    return 0;
  if (d->kind == DECL_COFAMILY)
    return check_cofamily(c, d);
  return d->kind == DECL_FAMILY ? check_family(c, d) : check_def(c, d);
}

int check_program(Arena *arena, const DeclList *decls, Machine *m, Diag *diag) {
  Checker c;
  size_t ctors = 0;
  size_t defs = decls->count;
  size_t families = decls->count;
  size_t i;
  memset(m, 0, sizeof *m);
  c.face = NULL;
  m->arena = arena;
  m->diag = diag;
  m->fuel = EVAL_FUEL_STEPS;
  /* A codata with F fields gives one family (D_pos), at most 1 + F
     constructors and at most 3 + 2F definitions (check_cofamily). */
  for (i = 0; i < decls->count; i++) {
    const Decl *d = &decls->items[i];
    size_t f = d->kind == DECL_COFAMILY && d->ctor_count == 1u ? d->ctors[0].field_count : 0u;
    ctors += d->ctor_count;
    if (d->kind == DECL_COFAMILY) {
      families += 1u;
      ctors += 1u + f;
      defs += 3u + 2u * f;
    }
  }
  m->defs = arena_alloc(arena, (defs + 1u) * sizeof *m->defs);
  m->families = arena_alloc(arena, (families + 1u) * sizeof *m->families);
  m->ctors = arena_alloc(arena, (ctors + 1u) * sizeof *m->ctors);
  if (m->defs == NULL || m->families == NULL || m->ctors == NULL)
    return diag_fail(diag, "OOM", NULL, "out of memory");
  m->def_cap = (uint32_t)defs;
  m->family_cap = (uint32_t)families;
  m->ctor_cap = (uint32_t)ctors;
  memset(&c, 0, sizeof c);
  c.m = m;
  c.decls = decls;
  for (i = 0; i < decls->count; i++) {
    c.decl_index = i;
    c.scope = NULL;
    c.env = NULL;
    c.level = 0;
    c.depth = 0;
    c.self = NO_FAMILY;
    m->def = decls->items[i].name;
    m->checked = i;
    m->decl_failed = 1;
    if (!check_decl(&c, &decls->items[i]))
      return 0;
  }
  m->def = NULL;
  m->checked = decls->count;
  m->decl_failed = 0;
  return 1;
}

int entry_of(Machine *m, const Value *type, Entry *out) {
  const Value *t = type;
  memset(out, 0, sizeof *out);
  while (t != NULL && t->kind == VAL_PI && out->param_count < ENTRY_PARAMS_MAX) {
    if (!val_is(t->dom, OP_NAT) && !val_is(t->dom, OP_BOOL))
      return 0;
    out->param_bool[out->param_count] = val_is(t->dom, OP_BOOL);
    t = closure_apply(m, t, val_var(m, out->param_count));
    out->param_count++;
  }
  out->result_bool = val_is(t, OP_BOOL);
  return val_is(t, OP_NAT) || val_is(t, OP_BOOL);
}

static int parse_u64(const char *s, uint64_t *out) {
  uint64_t n = 0;
  size_t i;
  if (s[0] == '\0')
    return 0;
  for (i = 0; s[i] != '\0'; i++) {
    unsigned d = (unsigned)(unsigned char)s[i] - (unsigned)'0';
    if (d > 9u || n > (UINT64_MAX - d) / 10u)
      return 0;
    n = n * 10u + d;
  }
  *out = n;
  return 1;
}

/* An entry argument: a Nat in decimal, or a Bool as 0 or 1. */
static const Value *entry_arg(Machine *m, const char *text, int is_bool) {
  uint64_t n = 0;
  if (!parse_u64(text, &n) || (is_bool && n > 1u)) {
    diag_fail(m->diag, "EVAL_ARGS", m->def, "'%s' is not a %s", text, is_bool ? "Bool (0 or 1)" : "ℕ (0 to 2^64-1)");
    return NULL;
  }
  return is_bool ? val_make(m, n == 1u ? OP_TRUE : OP_FALSE, 0, 0, NULL, NULL, NULL) : val_nat(m, n);
}

static int print_result(Machine *m, const char *name, const Value *v, FILE *out, FILE *err) {
  switch (v->kind) {
  case VAL_NAT:
    fprintf(out, "%llu\n", (unsigned long long)v->nat);
    return 0;
  case VAL_TRAP:
    fputs("trap\n", out);
    fprintf(err, "langc: EVAL_OVERFLOW: %s: a ℕ operation overflowed past 2^64-1\n", name);
    return 0;
  case VAL_OP:
    if (val_is(v, OP_TRUE) || val_is(v, OP_FALSE)) {
      fprintf(out, "%d\n", val_is(v, OP_TRUE));
      return 0;
    }
    break;
  case VAL_UNIV:
  case VAL_LAM:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    break;
  }
  diag_fail(m->diag, "EVAL_ENTRY", name, "the entry did not compute a number");
  return 1;
}

int eval_command(Machine *m, const char *name, char *const *args, int arg_count, FILE *out, FILE *err) {
  uint32_t d = find_def(m, name);
  Entry entry;
  const Value *v;
  char *buf;
  uint32_t i;
  m->def = name;
  if (d == NO_DEF) {
    diag_fail(m->diag, "EVAL_ENTRY", name, "no definition has this name");
    return 1;
  }
  v = def_value(m, d);
  if (v == NULL)
    return 1;
  if (!entry_of(m, m->defs[d].type, &entry) && arg_count != 0) {
    diag_fail(m->diag, "EVAL_ARGS", name, "the definition is not an entry, so it takes no arguments");
    return 2;
  }
  if (!entry_of(m, m->defs[d].type, &entry)) {
    buf = arena_alloc(m->arena, PRINT_MAX);
    if (buf == NULL)
      return diag_fail(m->diag, "OOM", name, "out of memory") + 1;
    if (!value_display(m, v, buf, PRINT_MAX)) {
      diag_fail(m->diag, "EVAL_PRINT", name, "the normal form exceeds the printer limits");
      return 1;
    }
    fprintf(out, "%s\n", buf);
    return 0;
  }
  if ((uint32_t)arg_count != entry.param_count) {
    diag_fail(m->diag, "EVAL_ARGS", name, "the entry takes %u arguments, found %d", entry.param_count, arg_count);
    return 2;
  }
  for (i = 0; i < entry.param_count && v != NULL; i++) {
    const Value *arg = entry_arg(m, args[i], entry.param_bool[i]);
    v = arg == NULL ? NULL : apply_value(m, v, arg);
  }
  if (v == NULL)
    return strcmp(m->diag->code != NULL ? m->diag->code : "", "EVAL_ARGS") == 0 ? 2 : 1;
  return print_result(m, name, v, out, err);
}
