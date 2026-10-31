#ifndef FRONT_SEARCH_H
#define FRONT_SEARCH_H

#include "front/core.h"

/* The bounded search core (S4 design sec 2). It runs the guest values of
   a replicated form: NAMEBot, NAMEMerge, NAMEInv and each NAMEWord. It
   holds no copy of the parts. The order is the 5.2 order, the enumerator
   is a depth first odometer: no seed, no clock, no floats. */

#define SEARCH_CAP 100000u
#define SEARCH_ARGS_MAX 8u

/* The guest values of one replicated form. An op parameter type is Fin
   SIZE (an atoms name, Replica or Fin m), so the argument j is pack j p,
   with p the proof of the first replica name. */
typedef struct {
  uint32_t bot;   /* the def index of NAMEBot */
  uint32_t merge; /* the def index of NAMEMerge */
  uint32_t inv;   /* the def index of NAMEInv */
  const uint32_t *words; /* the def index of NAMEWord, one for each op */
  const uint32_t *arity; /* the parameter count of each op */
  const uint32_t *const *sizes; /* the size of each parameter type of each op */
  uint32_t op_count;
  const Value *proof; /* the payload of the first replica name */
} SearchForm;

/* P (synced prefix at replica 0), M (ops for each replica), N (replicas). */
typedef struct {
  uint32_t prefix;
  uint32_t ops;
  uint32_t replicas;
} SearchBounds;

/* An op instance: SEG 0 is the prefix (at replica 0), SEG r + 1 is the
   branch of replica r. AFTER is the local state after the instance. */
typedef struct {
  uint32_t seg;
  uint32_t op;
  uint32_t args[SEARCH_ARGS_MAX];
  const Value *after;
} SearchStep;

typedef enum {
  SEARCH_KEPT,      /* each kept schedule keeps the invariant at the merge */
  SEARCH_VIOLATION, /* the first violation in the 5.2 order is in STEPS */
  SEARCH_ABOVE_CAP  /* the pair visits more than SEARCH_CAP schedules */
} SearchResult;

/* K is the count of enabled, I-valid schedules with O1 in one branch and
   O2 in a different branch. A violation keeps its schedule (the 4.6
   fields: the steps with the state after each, and the merged state). */
typedef struct {
  SearchResult result;
  uint64_t kept;   /* K */
  uint64_t visits; /* the schedules visited, for SEARCH_CAP */
  const SearchStep *steps;
  uint32_t step_count;
  const Value *merged;
} SearchOutcome;

/* Searches the pair (O1, O2) in the bounds B. Returns 0 after a
   diagnostic (an evaluation failure, or INTERNAL). */
int search_pair(Machine *m, const SearchForm *f, SearchBounds b, uint32_t o1, uint32_t o2, SearchOutcome *out);

#endif
