/* Exercise empty replica sets and the facts in counterexample transcripts. */
#define _POSIX_C_SOURCE 200809L
#include "front/check.h"
#include "front/front.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int sim_case(const char *label, const char *source, SearchBounds bounds, uint64_t kept,
                    const char *const *want, const char *absent) {
  Arena arena;
  Diag diag;
  Front front;
  Machine machine;
  SearchOutcome out;
  FILE *capture = NULL;
  int saved = -1, ok = 0, printed;
  size_t i, n;
  char text[8192];
  arena_init(&arena, (size_t)1 << 28);
  diag_init(&diag);
  if (!front_load(&arena, label, source, strlen(source), &front, &diag) ||
      !check_program_sim(&arena, &front.decls, &machine, &diag)) goto done;
  for (i = 0; i < front.decls.count && front.decls.items[i].kind != DECL_REPLICATED; i++) {}
  if (i == front.decls.count || !sim_form(&machine, &front.decls, i, bounds, &out)) goto done;
  if (out.kept != kept || out.result != (want == NULL ? SEARCH_KEPT : SEARCH_VIOLATION)) goto done;
  if (want == NULL) {
    ok = 1;
    goto done;
  }
  capture = tmpfile();
  if (capture == NULL || fflush(stdout) != 0 || (saved = dup(STDOUT_FILENO)) < 0 ||
      dup2(fileno(capture), STDOUT_FILENO) < 0) goto done;
  printed = sim_print(&machine, &front.decls, i, bounds, &out);
  if (fflush(stdout) != 0 || dup2(saved, STDOUT_FILENO) < 0) goto done;
  close(saved);
  saved = -1;
  if (!printed || fseek(capture, 0, SEEK_SET) != 0) goto done;
  n = fread(text, 1, sizeof text - 1u, capture);
  if (ferror(capture) || !feof(capture)) goto done;
  text[n] = '\0';
  for (i = 0; want[i] != NULL; i++) {
    if (strstr(text, want[i]) == NULL) {
      fprintf(stderr, "FAIL sim %s: missing %s\n", label, want[i]);
      goto done;
    }
  }
  if (absent != NULL && strstr(text, absent) != NULL) goto done;
  ok = 1;
done:
  if (saved >= 0) {
    (void)fflush(stdout);
    (void)dup2(saved, STDOUT_FILENO);
    close(saved);
  }
  if (capture != NULL) fclose(capture);
  if (!ok) {
    fprintf(stderr, "FAIL sim %s\n", label);
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

int main(void) {
  char tags[4096];
  size_t used;
  const char *inventory =
      "replicas A B\n\nreplicated Shop\n  state stock : Counter (init 1)\n"
      "  op sell := dec stock 1\n  op refill := restock stock 1\n  invariant nonneg stock\n";
  const char *prefix =
      "replicas A B\n\nreplicated Shop\n  state stock : Counter (init 0)\n"
      "  op sell := dec stock 1\n  op refill := restock stock 1\n  invariant nonneg stock\n";
  const char *unrelated_inv =
      "replicas A B\n\natoms Tag := urgent\n\nreplicated Tags\n"
      "  state tags : GSet Tag\n  op tag (t : Tag) := add tags t\n\n"
      "TagsInv : Tags → Bool\nTagsInv s = false\n";
  const char *multi =
      "replicas A B\n\nreplicated Shop\n  state stock : Counter (init 1)\n"
      "  op refill := restock stock 1\n  op sell := dec stock 2\n  invariant nonneg stock\n";
  const char *wide =
      "replicas A B\n\nreplicated Shop\n  state reserve : Counter (init 9223372036854775808)\n"
      "  state stock : Counter (init 1)\n  op sell := dec stock 1\n"
      "  invariant nonneg reserve\n  invariant nonneg stock\n";
  const char *escrow =
      "replicas A B\n\nreplicated Shop\n  state reserve : Counter (init 9223372036854775808)\n"
      "  state rights : Escrow\n  state stock : Counter (init 1)\n  op sell := dec stock 1\n"
      "  invariant nonneg reserve\n  invariant nonneg stock\n"
      "  allocate reserve by escrow (A ↦ 0, B ↦ 9223372036854775808)\n";
  const char *const prefix_want[] = {"prefix: refill   stock 0 -> 1\n", "REFUTED invariant nonneg stock: stock = -1\n",
                                     "fix: allocate stock by escrow (A ↦ 0, B ↦ 0)\n", NULL};
  const char *const multi_want[] = {"A: refill   stock 1 -> 2\n", "B: sell   stock 2 -> 0",
                                    "schedule: needs coordination (REFUSE_NEEDS_COORDINATION)\n", NULL};
  const char *const wide_want[] = {"ancestor: reserve 9223372036854775808 (init 9223372036854775808)",
                                   "REFUTED invariant nonneg stock: stock = -1\n",
                                   "fix: allocate stock by escrow (A ↦ 1, B ↦ 0)\n", NULL};
  const char *const tags_want[] = {"REFUTED invariant nonneg stock: stock = -1\n", NULL};
  const char *const escrow_want[] = {"reserve 9223372036854775808 (init 9223372036854775808)   rights 0 (init 0)",
                                     "REFUTED invariant nonneg stock: stock = -1\n", NULL};
  int ok = 1;
  ok &= sim_case("zero replicas", inventory, (SearchBounds){0, 2, 0}, 1, NULL, NULL);
  ok &= sim_case("zero replicas with prefix", inventory, (SearchBounds){1, 2, 0}, 3, NULL, NULL);
  ok &= sim_case("unrelated Inv definition", unrelated_inv, (SearchBounds){1, 2, 2}, 18, NULL, NULL);
  ok &= sim_case("allocation after prefix", prefix, (SearchBounds){1, 2, 2}, 14, prefix_want, NULL);
  ok &= sim_case("multi-step classification", multi, (SearchBounds){0, 2, 2}, 15, multi_want, "refill ∥ refill");
  ok &= sim_case("wide stock", wide, (SearchBounds){1, 2, 2}, 4, wide_want, "REFUTED invariant nonneg reserve");
  ok &= sim_case("escrow stock", escrow, (SearchBounds){1, 2, 2}, 4, escrow_want, "REFUTED invariant nonneg reserve");
  used = (size_t)snprintf(tags, sizeof tags, "replicas A B\n\natoms Tag := t0");
  for (int j = 1; j < 20; j++) used += (size_t)snprintf(tags + used, sizeof tags - used, " ∣ t%d", j);
  used += (size_t)snprintf(tags + used, sizeof tags - used,
                          "\n\nreplicated Shop\n  state tags : GSet Tag\n  state stock : Counter (init 1)\n  op sell := ");
  for (int j = 0; j < 20; j++) used += (size_t)snprintf(tags + used, sizeof tags - used, "add tags t%d; ", j);
  (void)snprintf(tags + used, sizeof tags - used, "dec stock 1\n  invariant nonneg stock\n");
  ok &= sim_case("complete set rendering", tags, (SearchBounds){1, 2, 2}, 4, tags_want, "...");
  if (ok) puts("sim regressions: 8 checks");
  return ok ? 0 : 1;
}
