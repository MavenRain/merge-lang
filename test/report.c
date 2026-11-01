/* Check the op pair rows of check_program (S6-DESIGN sec 3 and 4) on the examples. */
#define _POSIX_C_SOURCE 200809L
#include "front/check.h"
#include "front/front.h"
#include <stdio.h>
#include <string.h>

#define COUNT(x) ((uint32_t)(sizeof(x) / sizeof((x)[0])))

/* One expected row. CEX NULL: the row has no counterexample. */
typedef struct {
  const char *a, *b;
  RowClass cls;
  const char *part, *invariant, *rule;
  RowMark mark;
  const char *code, *fix;
  const RowCex *cex;
} WantRow;

static const char *show(const char *text) { return text == NULL ? "(null)" : text; }

static int same(const char *got, const char *want) {
  return got == NULL ? want == NULL : want != NULL && strcmp(got, want) == 0;
}

static int row_ok(const PairRow *r, const char *form, const WantRow *w) {
  return same(r->form, form) && same(r->a, w->a) && same(r->b, w->b) && r->cls == w->cls && same(r->part, w->part) &&
         same(r->invariant, w->invariant) && same(r->rule, w->rule) && r->mark == w->mark && same(r->code, w->code) &&
         same(r->fix, w->fix) && (r->cex == NULL) == (w->cex == NULL);
}

static int cex_ok(const RowCex *g, const RowCex *w) {
  uint32_t i, j;
  int ok = g->prefix == w->prefix && g->ops == w->ops && g->replicas == w->replicas &&
           g->step_count == w->step_count && same(g->ancestor, w->ancestor) && same(g->merged, w->merged) &&
           same(g->inv_name, w->inv_name) && same(g->inv_values, w->inv_values);
  for (i = 0; ok && i < w->replicas; i++) ok = same(g->replica_names[i], w->replica_names[i]);
  for (i = 0; ok && i < w->step_count; i++) {
    const RowStep *s = &g->steps[i], *t = &w->steps[i];
    ok = s->seg == t->seg && same(s->op, t->op) && s->arg_count == t->arg_count && same(s->state, t->state);
    for (j = 0; ok && j < t->arg_count; j++) ok = same(s->args[j], t->args[j]);
  }
  return ok;
}

static void row_print(const PairRow *r) {
  uint32_t i, j;
  const RowCex *x = r->cex;
  fprintf(stderr, "  row %s %s %s: class %d, part %s, invariant %s, rule %s, mark %d, code %s, fix %s\n", show(r->form),
          show(r->a), show(r->b), (int)r->cls, show(r->part), show(r->invariant), show(r->rule), (int)r->mark,
          show(r->code), show(r->fix));
  if (x == NULL) return;
  fprintf(stderr, "  cex prefix %u, ops %u, replicas %u, ancestor %s, merged %s, %s: %s\n", (unsigned)x->prefix,
          (unsigned)x->ops, (unsigned)x->replicas, show(x->ancestor), show(x->merged), show(x->inv_name),
          show(x->inv_values));
  for (i = 0; i < x->step_count; i++) {
    fprintf(stderr, "  step seg %u: %s", (unsigned)x->steps[i].seg, show(x->steps[i].op));
    for (j = 0; j < x->steps[i].arg_count; j++) fprintf(stderr, " %s", show(x->steps[i].args[j]));
    fprintf(stderr, "   %s\n", show(x->steps[i].state));
  }
}

/* Runs check_program on PATH (from the repo root). WANT_OK 0: the check
   refuses with CODE. Compares the rows with WANT; adds one to *CHECKS for
   each row and one for each counterexample. */
static int report_case(const char *path, int want_ok, const char *code, const char *form, const WantRow *want,
                       uint32_t want_count, int *checks) {
  static char text[1 << 20];
  Arena arena;
  Diag diag;
  Front front;
  Machine machine;
  FILE *in = fopen(path, "rb");
  size_t len;
  uint32_t i;
  int checked = -1, ok = 0;
  if (in == NULL) {
    fprintf(stderr, "FAIL report %s: cannot read the file\n", path);
    return 0;
  }
  len = fread(text, 1, sizeof text, in);
  fclose(in);
  if (len == sizeof text) {
    fprintf(stderr, "FAIL report %s: the file is too large\n", path);
    return 0;
  }
  arena_init(&arena, (size_t)1 << 28);
  diag_init(&diag);
  memset(&machine, 0, sizeof machine);
  if (!front_load(&arena, path, text, len, &front, &diag)) goto done;
  checked = check_program(&arena, &front.decls, &machine, &diag);
  if (checked != want_ok || (!checked && !same(diag.code, code)) || machine.row_count != want_count) goto done;
  for (i = 0; i < want_count; i++) {
    if (!row_ok(&machine.rows[i], form, &want[i]) ||
        (want[i].cex != NULL && !cex_ok(machine.rows[i].cex, want[i].cex)))
      goto done;
    *checks += want[i].cex == NULL ? 1 : 2;
  }
  ok = 1;
done:
  if (!ok) {
    fprintf(stderr, "FAIL report %s: check %d, %u rows\n", path, checked, (unsigned)machine.row_count);
    if (diag.set) diag_print(&diag, stderr);
    for (i = 0; i < machine.row_count; i++) row_print(&machine.rows[i]);
  }
  arena_release(&arena);
  return ok;
}

int main(void) {
  static const char *const one[] = {"1"};
  static const char *const names[] = {"A", "B"};
  static const RowStep steps[] = {{1, "sell", one, 1, "stock 0 (init 1, decrements A 1)"},
                                  {2, "sell", one, 1, "stock 0 (init 1, decrements B 1)"}};
  static const RowCex cex = {1, 2, 2, names, steps, COUNT(steps), "stock 1 (init 1)",
                             "stock -1 (init 1, decrements A 1, B 1)", "nonneg stock", "stock = -1"};
  const char *fix = "declare an allocation rule, for example `allocate stock by escrow (...)`, or remove the invariant";
  const char *inv = "nonneg stock", *rule = "allocate stock by escrow (A ↦ 1, B ↦ 0)";
  const WantRow owner[] = {
      {"assign", "assign", ROW_EXPLICIT_CONFLICT, "owner", NULL, NULL, MARK_PROVED, NULL, NULL, NULL}};
  const WantRow tags[] = {{"tag", "tag", ROW_INDEPENDENT, NULL, NULL, NULL, MARK_PROVED, NULL, NULL, NULL}};
  const WantRow inventory[] = {{"sell", "sell", ROW_NEEDS_COORDINATION, NULL, inv, NULL, MARK_REFUTED,
                                "REFUSE_NEEDS_COORDINATION", fix, &cex}};
  const WantRow escrow[] = {{"sell", "sell", ROW_INDEPENDENT, NULL, inv, rule, MARK_PROVED, NULL, NULL, NULL},
                            {"sell", "refill", ROW_INDEPENDENT, NULL, inv, rule, MARK_PROVED, NULL, NULL, NULL},
                            {"refill", "refill", ROW_INDEPENDENT, NULL, inv, rule, MARK_PROVED, NULL, NULL, NULL}};
  int checks = 0, ok = 1;
  ok &= report_case("examples/owner.merge", 1, NULL, "Task", owner, COUNT(owner), &checks);
  ok &= report_case("examples/tags.merge", 1, NULL, "Tags", tags, COUNT(tags), &checks);
  ok &= report_case("examples/inventory.merge", 0, "REFUSE_NEEDS_COORDINATION", "Shop", inventory, COUNT(inventory),
                    &checks);
  ok &= report_case("examples/inventory-escrow.merge", 1, NULL, "Shop", escrow, COUNT(escrow), &checks);
  if (ok) printf("report rows: %d checks\n", checks);
  return ok ? 0 : 1;
}
