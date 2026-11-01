/* The type checker, the refusals R1-R4 and the eval command. SHARED by the
   tcc kits. */
#ifndef LANG_FRONT_CHECK_H
#define LANG_FRONT_CHECK_H

#include "front/core.h"
#include "front/search.h"

#define ENTRY_PARAMS_MAX 16u

/* An entry: a program definition of type Nat, Bool, or a function of Nat and
   Bool arguments to a Nat or Bool result. */
typedef struct {
  uint32_t param_count;
  int param_bool[ENTRY_PARAMS_MAX];
  int result_bool;
} Entry;

/* Checks the domain declarations, then the program, into M. Returns 1, or 0
   after a diagnostic. */
int check_program(Arena *arena, const DeclList *decls, Machine *m, Diag *diag);
/* check_program in sim mode (langc sim, Q57 (a)): a replicated form gets
   no form_search, so REFUSE_NEEDS_COORDINATION does not occur. */
int check_program_sim(Arena *arena, const DeclList *decls, Machine *m, Diag *diag);
/* The sim search (S5 design sec 2, Q56 (a)) of the replicated form at
   index I of DECLS, after check_program_sim: each op in each branch, in
   the bounds B. Returns 1, or 0 after a diagnostic. */
int sim_form(Machine *m, const DeclList *decls, size_t i, SearchBounds b, SearchOutcome *out);
/* The sim transcript after the header (S5 design sec 4) of the violation
   OUT of the form at decl I, on stdout. Returns 0 after a diagnostic. */
int sim_print(Machine *m, const DeclList *decls, size_t i, SearchBounds b, const SearchOutcome *out);
/* The engine view of a replicated form (S7 design sec 3 and 4), for
   src/engine.c and test/engine.c. */
#define ENGINE_PACK_MAX 4096u

typedef enum {
  ENGINE_GSET,
  ENGINE_MVREG,
  ENGINE_COUNTER,
  ENGINE_ESCROW
} EnginePartKind;

/* A state line: the atoms count N (GSet, MVReg), the init S0 (Counter,
   Escrow) and the share of each replica in the replicas order (Escrow;
   else NULL). */
typedef struct {
  EnginePartKind kind;
  uint64_t n;
  uint64_t s0;
  const uint64_t *split;
} EnginePart;

typedef enum {
  ENGINE_ADD,
  ENGINE_WRITE,
  ENGINE_DEC,
  ENGINE_RESTOCK,
  ENGINE_TRANSFER
} EngineWord;

/* One write step: WORD on PART. ARGS and TO contain the evaluated member,
   amount or transfer target for each packed op argument, in pack order.
   TO is NULL except for a transfer. */
typedef struct {
  EngineWord word;
  size_t part;
  const uint64_t *args;
  const uint64_t *to;
} EngineStep;

/* An op: the size of each finite parameter and the write steps in order. */
typedef struct {
  const char *name;
  uint32_t param_count;
  const uint32_t *sizes;
  const EngineStep *steps;
  size_t step_count;
} EngineOp;

/* The form NAME on R replicas: the parts and the ops in declaration
   order, the part of each `nonneg` invariant (declaration order) and the
   SearchForm of search_pair. */
typedef struct {
  const char *name;
  uint64_t r;
  const EnginePart *parts;
  size_t part_count;
  const EngineOp *ops;
  size_t op_count;
  const size_t *nonneg;
  size_t inv_count;
  SearchForm search;
} EngineForm;

/* The engine view of the replicated form at index I of DECLS, after
   check_program or check_program_sim. An op with a read step is INTERNAL.
   Returns 1, or 0 after a diagnostic. */
int engine_form(Machine *m, const DeclList *decls, size_t i, EngineForm *out);
/* Returns 1 when TYPE is the type of an entry. */
int entry_of(Machine *m, const Value *type, Entry *out);
/* Evaluates the definition NAME on ARGS and prints the result. Returns 0, 1
   after a diagnostic, or 2 after an EVAL_ARGS diagnostic. */
int eval_command(Machine *m, const char *name, char *const *args, int arg_count, FILE *out, FILE *err);

#endif
