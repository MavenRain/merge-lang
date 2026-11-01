/* The Wasm merge engine (S7 design sec 2 to 5): the entries NAME.apply,
   NAME.merge, NAME.check, NAME.encode, NAME.decode and NAME.bot of each
   replicated form, as kit IR. merge-lang only, not SHARED. */
#ifndef LANG_ENGINE_H
#define LANG_ENGINE_H

#include <stdio.h>

#include "front/check.h"
#include "ir.h"

/* The number of engine entries of one form. */
#define ENGINE_FUNCS 6u

/* Writes the ENGINE_FUNCS entries of FORM to OUT (keeps_heap = 1), with the
   IR in ARENA. Returns 1, or 0 after an INTERNAL line on ERR. */
int engine_funcs(Arena *arena, const EngineForm *form, IrFunc *out, FILE *err);

/* Appends the engine entries of each replicated form of DECLS to PROG (a
   new array in the arena of M), after check_program or check_program_sim.
   Returns 1, or 0 after a diagnostic or an INTERNAL line on ERR. */
int engine_program(Machine *m, const DeclList *decls, IrProgram *prog, FILE *err);

#endif
