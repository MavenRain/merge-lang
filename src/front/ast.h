/* The syntax tree. SHARED by the tcc kits. Terms and types share one syntax. */
#ifndef LANG_FRONT_AST_H
#define LANG_FRONT_AST_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  TERM_VAR,   /* name */
  TERM_NAT,   /* nat */
  TERM_APP,   /* left applied to right */
  TERM_LAM,   /* fun (name : left) => right; left is NULL for a bare binder */
  TERM_PI,    /* (name : left) -> right; name is NULL for left -> right */
  TERM_SIGMA, /* Sigma (name : left) right, or (name : left) * right */
  TERM_UNIV   /* Type nat */
} TermKind;

typedef struct Term Term;
struct Term {
  TermKind kind;
  int line;
  int col;
  const char *name;
  uint64_t nat;
  const Term *left;
  const Term *right;
};

/* A named field of a constructor, or a parameter of a family. */
typedef struct {
  const char *name;
  const Term *type;
  int line;
  int col;
} Field;

typedef struct {
  const char *name;
  const Field *fields;
  size_t field_count;
  const Term **indices; /* the result indices: one for each index of the family */
  const Term *result;   /* a path constructor: its type Path (F A) s t; NULL for a point constructor */
  int line;
  int col;
} Ctor;

typedef enum {
  DECL_DEF,    /* [rec] name : type, then the clause name args = body */
  DECL_AXIOM,  /* postulate name : type */
  DECL_FAMILY,  /* data name params : Type where, then one constructor a line */
  DECL_COFAMILY, /* codata name params : Type where, then the one corecursor clause */
  DECL_REPLICAS, /* replicas NAME+ (the names) */
  DECL_ATOMS,    /* atoms name := m ∣ .. (the names: all names or all numerals) */
  DECL_REPLICATED /* replicated name, then one item a line */
} DeclKind;

/* A name in a form, or a numeral when TEXT is NULL, with its position. */
typedef struct {
  const char *text;
  uint64_t nat;
  int line;
  int col;
} FormName;

typedef enum {
  ITEM_STATE,     /* state name : type [(init nat)] */
  ITEM_OP,        /* op name params := body */
  ITEM_INVARIANT, /* invariant body */
  ITEM_ALLOCATE   /* allocate name by escrow (shares), each share NAME ↦ nat */
} ItemKind;

/* One step of an op script (Q41 (a)): `x ← read P` or `x ← stock P`
   (NAME is the read variable) or a write term (NAME is NULL). */
typedef struct {
  const char *name;
  int stock;
  FormName part;
  const Term *write;
  int line;
  int col;
} OpStep;

/* One item of a `replicated` form. STEPS are the op steps before the last;
   BODY is the last step, a write. */
typedef struct {
  ItemKind kind;
  FormName name;
  const Term *type;
  int has_init;
  uint64_t init;
  const Field *params;
  size_t param_count;
  const Term *body;
  const OpStep *steps;
  size_t step_count;
  const FormName *shares;
  size_t share_count;
  int line;
  int col;
} FormItem;

/* A family is legal only in the domain file (rule R1). */
typedef enum {
  ORIGIN_DOMAIN,
  ORIGIN_PROGRAM
} Origin;

typedef struct {
  DeclKind kind;
  Origin origin;
  int is_rec;
  int line;
  int col;
  int end_line; /* the last token of the declaration */
  int end_col;
  const char *name;
  const Term *type;
  const Term *body;
  const Field *params;
  size_t param_count;
  const Term **indices; /* the index types of `data F params : A1 → .. → Type` */
  size_t index_count;
  const Ctor *ctors;
  size_t ctor_count;
  const FormName *names; /* a replicas or atoms form */
  size_t name_count;
  const FormItem *items; /* a replicated form */
  size_t item_count;
} Decl;

/* Domain declarations come first, then the program. */
typedef struct {
  Decl *items;
  size_t count;
  size_t cap;
} DeclList;

#endif
