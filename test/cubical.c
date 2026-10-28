/* Check face restrictions, binder scope and normalized source round trips. */
#include "front/check.h"
#include "front/front.h"
#include <stdio.h>
#include <string.h>

static int load(Arena *arena, const char *source, Machine *machine, Diag *diag) {
  Front front;
  return front_load(arena, "cubical-regression", source, strlen(source), &front, diag)
      && check_program(arena, &front.decls, machine, diag);
}

static int check_case(const char *label, const char *source, const char *want) {
  Arena arena;
  Diag diag;
  Machine machine;
  arena_init(&arena, (size_t)1 << 27);
  diag_init(&diag);
  int checked = load(&arena, source, &machine, &diag);
  int ok = want == NULL ? checked : !checked && diag.code != NULL && strcmp(diag.code, want) == 0;
  if (!ok) {
    fprintf(stderr, "FAIL cubical %s: wanted %s, got %s\n", label,
            want == NULL ? "ok" : want, checked ? "ok" : diag.code == NULL ? "no diagnostic" : diag.code);
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int round_trip(const char *label, const char *signature, const char *body, int display) {
  Arena arena;
  Diag diag;
  Machine machine;
  char source[32768];
  char normal[16384];
  char again[65536];
  int ok = 0;
  arena_init(&arena, (size_t)1 << 27);
  diag_init(&diag);
  int len = snprintf(source, sizeof source, "value : %s\nvalue = %s\n", signature, body);
  if (len < 0 || (size_t)len >= sizeof source || !load(&arena, source, &machine, &diag)) goto done;
  const Value *v = def_value(&machine, machine.def_count - 1u);
  if (v == NULL) goto done;
  if (display) {
    if (!value_display(&machine, v, normal, sizeof normal)) goto done;
  } else if (!value_print(&machine, NULL, 0, v, normal, sizeof normal)) goto done;
  len = snprintf(again, sizeof again, "%s\nagain : %s\nagain = %s\n", source, signature, normal);
  if (len < 0 || (size_t)len >= sizeof again) goto done;
  diag_init(&diag);
  if (!load(&arena, again, &machine, &diag)) goto done;
  ok = conv_values(&machine, 0, def_value(&machine, machine.def_count - 2u),
                              def_value(&machine, machine.def_count - 1u));
done:
  if (!ok) {
    fprintf(stderr, "FAIL cubical round trip %s\n", label);
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

int main(void) {
  static const struct { const char *label; const char *source; const char *code; } checks[] = {
    {"cell universe alias",
     "Small : Type 1\nSmall = Type 0\n"
     "data Box (A : Small) : Type where\n  box : (item : A) → Box A\n"
     "value : Box ℕ\nvalue = box ℕ 7\n", NULL},
    {"cell reduced universe",
     "data Box (A : ((λ (u : Unit) → Type 0) unit)) : Type where\n  box : (item : A) → Box A\n", NULL},
    {"cell generator type alias",
     "Identity : (T : Type 0) → Type 0\nIdentity T = T\n"
     "data Box (A : Type 0) : Type where\n  box : (item : Identity A) → Box A\n"
     "value : Box ℕ\nvalue = box ℕ 7\n", NULL},
    {"cell recursive type alias",
     "Identity : (T : Type 0) → Type 0\nIdentity T = T\n"
     "data Box (A : Type 0) : Type where\n  box : (item : A) → Box A\n  againBox : (boxRest : Identity (Box A)) → Box A\n", NULL},
    {"cell higher kind alias refused",
     "Kind : Type 1\nKind = Unit → Type 0\n"
     "data Bad (F : Kind) : Type where\n  bad : (item : F unit) → Bad F\n", "REFUSE_CELL"},
    {"base value parameter alias refused",
     "Number : Type 0\nNumber = ℕ\n"
     "data Bad (n : Number) : Type where\n  bad : Bad n\n", "REFUSE_DATA"},
    {"cell cons alias refused",
     "Identity : (T : Type 0) → Type 0\nIdentity T = T\n"
     "data Bad (A : Type 0) : Type where\n  bad : (item : Identity A) → (badRest : Bad A) → Bad A\n", "REFUSE_CELL"},
    {"indexed cell result scope",
     "data Indexed (A : Type 0) : Path (Type 0) A A → Type where\n"
     "  inject : (a : A) → Indexed A refl\n"
     "value : Indexed ℕ refl\nvalue = inject ℕ 0\n", NULL},
    {"reducible type index",
     "data HiddenIndex (A : Type 0) : ((λ (u : Unit) → Type 0) unit) → Type where\n"
     "  generate : (a : A) → HiddenIndex A ℕ\n", "REFUSE_CELL"},
    {"reducible higher kind index",
     "data HiddenKind (A : Type 0) : (Unit → ((λ (u : Unit) → Type 0) unit)) → Type where\n"
     "  generate : (a : A) → HiddenKind A (λ u → ℕ)\n", "REFUSE_CELL"},
    {"shadowed side face", "value : (j : I) → ℕ\nvalue j = hcomp (λ j → [ j ↦ 0 ]) 0\n", "TYPE_SYSTEM"},
    {"renamed side face", "value : (j : I) → ℕ\nvalue j = hcomp (λ k → [ k ↦ 0 ]) 0\n", "TYPE_SYSTEM"},
    {"side face application", "value : (f : I → I) → ℕ\nvalue f = hcomp (λ j → [ f j ↦ 0 ]) 0\n", "TYPE_SYSTEM"},
    {"outer side face", "value : (j : I) → ℕ\nvalue j = hcomp (λ k → [ j ↦ 0 ]) 0\n", NULL},
    {"constant side face", "value : ℕ\nvalue = hcomp (λ j → [ j ∧ i0 ↦ 0 ]) 0\n", NULL},
    {"shadowed interval annotation", "value : (I : Type 0) → ℕ\nvalue I = hcomp (λ (j : I) → [ i1 ↦ 0 ]) 0\n", "TYPE_MISMATCH"},
    {"wrong side annotation", "value : ℕ\nvalue = hcomp (λ (j : ℕ) → [ i1 ↦ 0 ]) 0\n", "TYPE_MISMATCH"},
    {"interval annotation alias", "value : ℕ\nvalue = hcomp (λ (j : (λ (u : Unit) → I) unit) → [ i1 ↦ 0 ]) 0\n", NULL},
    {"refl under face", "value : (p : Path ℕ 0 1) → (i : I) → Partial i (Path ℕ (p i) 1)\nvalue p i = [ i ↦ refl ]\n", NULL},
    {"path endpoints under face", "value : (p : Path ℕ 0 1) → (i : I) → Partial i (Path ℕ (p i) 1)\nvalue p i = [ i ↦ (λ j → 1) ]\n", NULL},
    {"constant line under face", "value : (e : Path (Type 0) ℕ ℕ) → (i : I) → Partial (~ i) (PathP (λ (j : I) → e (i ∧ j)) 0 (transp (λ j → e (i ∧ j)) i0 0))\nvalue e i = [ ~ i ↦ refl ]\n", NULL},
    {"trans under face", "value : (p : Path ℕ 0 1) → (i : I) → (q : Path ℕ 0 (p i)) → (r : Path ℕ 1 2) → Partial i (Path ℕ 0 2)\nvalue p i q r = [ i ↦ trans q r ]\n", NULL},
    {"annotation under face", "value : (e : Path (Type 0) ℕ ℕ) → (i : I) → Partial i ((x : e i) → ℕ)\nvalue e i = [ i ↦ (λ (x : ℕ) → 0) ]\n", NULL},
    {"refl on wrong face", "value : (p : Path ℕ 0 1) → (i : I) → Partial (~ i) (Path ℕ (p i) 1)\nvalue p i = [ ~ i ↦ refl ]\n", "TYPE_REFL"},
    {"endpoints on wrong face", "value : (p : Path ℕ 0 1) → (i : I) → Partial (~ i) (Path ℕ (p i) 1)\nvalue p i = [ ~ i ↦ (λ j → 1) ]\n", "TYPE_ENDPOINT"},
    {"refl on both faces", "value : (p : Path ℕ 0 1) → (i : I) → Partial (i ∨ ~ i) (Path ℕ (p i) 1)\nvalue p i = [ i ∨ ~ i ↦ refl ]\n", "TYPE_REFL"},
    {"trans on wrong face", "value : (p : Path ℕ 0 1) → (i : I) → (q : Path ℕ 0 (p i)) → (r : Path ℕ 1 2) → Partial (~ i) (Path ℕ 0 2)\nvalue p i q r = [ ~ i ↦ trans q r ]\n", "TYPE_MISMATCH"},
    {"squash at i0", "data Trunc (A : Type 0) : Type where\n  inc : (a : A) → Trunc A\n  squash : (x : Trunc A) → (y : Trunc A) → Path (Trunc A) x y\nvalue : (A : Type 0) → (u : Trunc A) → (v : Trunc A) → Path (Trunc A) (squash A u v i0) u\nvalue A u v = refl\n", NULL},
    {"squash at i1", "data Trunc (A : Type 0) : Type where\n  inc : (a : A) → Trunc A\n  squash : (x : Trunc A) → (y : Trunc A) → Path (Trunc A) x y\nvalue : (A : Type 0) → (u : Trunc A) → (v : Trunc A) → Path (Trunc A) (squash A u v i1) v\nvalue A u v = refl\n", NULL},
    {"codata Stream", "codata Stream (A : Type 0) : Type where\n  corec : (head : A) → (tail : Stream A) → Stream A\n", NULL},
    {"codata labelled field", "codata Stream (A : Type 0) : Type where\n  corec : (head : A) → (pick : ℕ → Stream A) → Stream A\n", NULL},
    {"codata head law",
     "codata Stream (A : Type 0) : Type where\n  corec : (head : A) → (tail : Stream A) → Stream A\n"
     "headLaw : (A : Type 0) → (S : Type 0) → (o : S → A) → (t : S → S) → (seed : S) → Path A (head A (corec A S o t seed)) (o seed)\n"
     "headLaw A S o t seed = refl\n", NULL},
    {"codata tail law",
     "codata Stream (A : Type 0) : Type where\n  corec : (head : A) → (tail : Stream A) → Stream A\n"
     "tailLaw : (A : Type 0) → (S : Type 0) → (o : S → A) → (t : S → S) → (seed : S) → Path (Stream A) (tail A (corec A S o t seed)) (corec A S o t (t seed))\n"
     "tailLaw A S o t seed = refl\n", NULL},
    {"codata then def label",
     "codata Stream (A : Type 0) : Type where\n  corec : (head : A) → (tail : Stream A) → Stream A\n"
     "label : ℕ\nlabel = 0\n", NULL},
    {"codata field named Stream",
     "codata Stream (A : Type 0) : Type where\n  corec : (head : A) → (Stream : Stream A) → Stream A\n", "REFUSE_NAME"},
    {"codata field two times",
     "codata Stream (A : Type 0) : Type where\n  corec : (head : A) → (tail : Stream A) → (tail : Stream A) → Stream A\n", "REFUSE_NAME"},
    {"codata generated name taken",
     "Stream_pos : ℕ\nStream_pos = 0\n"
     "codata Stream (A : Type 0) : Type where\n  corec : (head : A) → (tail : Stream A) → Stream A\n", "REFUSE_NAME"},
    {"codata two recursive fields",
     "codata Branches (A : Type 0) : Type where\n  grow : (observe : A) → (left : Branches A) → (right : Branches A) → Branches A\n"
     "value : Path ℕ (observe ℕ (right ℕ (left ℕ (grow ℕ ℕ (λ (s : ℕ) → s) (λ (s : ℕ) → natAdd s 1) (λ (s : ℕ) → natMul s 2) 3)))) 8\n"
     "value = refl\n", NULL},
    {"codata mixed recursive fields",
     "codata Route (A : Type 0) : Type where\n  collect : (viaNat : ℕ → Route A) → (seen : A) → (nextRoute : Route A) → (viaBool : Bool → Route A) → Route A\n"
     "route : Route ℕ\nroute = collect ℕ ℕ (λ (s : ℕ) → s) (λ (s : ℕ) (e : ℕ) → natAdd s e) (λ (s : ℕ) → natMul s 2) (λ (s : ℕ) (e : Bool) → natMul s 3) 3\n"
     "value : Path ℕ (seen ℕ (viaBool ℕ (nextRoute ℕ (viaNat ℕ route 5)) true)) 48\n"
     "value = refl\n", NULL},
    {"Fin unfolds to a Sigma",
     "value : Path (Type 0) (Fin 2) ((k : ℕ) × Path Bool (natLe (natAdd k 1) 2) true)\nvalue = refl\n", NULL},
    {"Fin value below the bound", "value : Fin 2\nvalue = pack 1 refl\n", NULL},
    {"Fin value at the bound", "value : Fin 2\nvalue = pack 2 refl\n", "TYPE_REFL"},
    {"Fin with a neutral bound", "value : (n : ℕ) → (x : Fin n) → Path Bool (natLe (natAdd (witness x) 1) n) true\nvalue n x = payload x\n",
     NULL}
  };
  static const struct { const char *label; const char *signature; const char *body; } rounds[] = {
    {"integer zero", "ℤ", "pos 0"},
    {"integer positive", "ℤ", "pos 4"},
    {"integer positive boundary", "ℤ", "pos 18446744073709551615"},
    {"integer negative", "ℤ", "negsuc 0"},
    {"integer negative boundary", "ℤ", "negsuc 18446744073709551615"},
    {"integer option", "Option ℤ", "some (negsuc 2)"},
    {"integer function", "Unit → ℤ", "λ u → negsuc 0"},
    {"integer path type", "Type 0", "Path ℤ (pos 0) (pos 0)"},
    {"hcomp", "(p : Path ℕ 0 0) → (i : I) → ℕ", "λ p i → hcomp (λ j → [ i ↦ p j ]) 0"},
    {"Path type", "Type 0", "Path ℕ 0 0"},
    {"varying PathP type", "(e : Path (Type 0) ℕ ℕ) → Type 0", "λ e → PathP (λ (i : I) → e i) 0 0"},
    {"hcomp path", "(p : Path ℕ 0 0) → (i : I) → Path ℕ 0 0", "λ p i → hcomp (λ j → [ i ↦ p ]) p"},
    {"hcomp Sigma", "(n : ℕ) → (i : I) → ℕ × ℕ", "λ n i → hcomp (λ j → [ i ↦ pack 1 n ]) (pack 1 n)"},
    {"hcomp universe", "(i : I) → Type 0", "λ i → hcomp (λ j → [ i ↦ ℕ ]) ℕ"},
    {"neutral transp", "(e : Path (Type 0) ℕ ℕ) → ℕ → ℕ", "λ e n → transp (λ i → e i) i0 n"},
    {"nested Path type", "Type 0", "Path (Path ℕ 0 0) refl refl"},
    {"named Sigma type", "Type 0", "(n : ℕ) × PathP (λ (_ : I) → ℕ) n n"},
    {"product type", "Type 0", "ℕ × Bool"},
    {"polymorphic Pi type", "Type 1", "(A : Type 0) → (A → A) → A → A"},
    {"function left of product", "Type 0", "(ℕ → ℕ) × ℕ"},
    {"function right of product", "Type 0", "ℕ × (ℕ → ℕ)"},
    {"product left of product", "Type 0", "(ℕ × ℕ) × ℕ"},
    {"named Sigma with Pi body", "Type 0", "(n : ℕ) × (ℕ → PathP (λ (_ : I) → ℕ) n n)"},
    {"nested named Sigma", "Type 0", "(n : ℕ) × (k : ℕ) × PathP (λ (_ : I) → ℕ) n k"},
    {"product right of arrow", "Type 0", "ℕ → ℕ × ℕ"},
    {"product left of arrow", "Type 0", "(ℕ × ℕ) → ℕ"},
    {"universe left of product", "Type 1", "(Type 0) × ℕ"},
    {"universe left of arrow", "Type 1", "(Type 0) → ℕ"},
    {"shadowed Sigma binder", "Type 0", "(n : ℕ) × (n : ℕ) × Path ℕ n n"},
    {"annotated lambda", "ℕ → ℕ", "λ (x : ℕ) → natAdd x 1"}
  };
  static const struct { const char *label; const char *signature; const char *body; } displays[] = {
    {"display positive function", "Unit → ℤ", "λ u → pos 4"},
    {"display negative function", "Unit → ℤ", "λ u → negsuc 0"},
    {"display negative boundary function", "Unit → ℤ", "λ u → negsuc 18446744073709551615"},
    {"display integer path type", "Type 0", "Path ℤ (pos 0) (pos 0)"},
    {"display function inside data", "ℤ × (Unit → ℤ)", "pack (pos 4) (λ u → negsuc 0)"},
    {"display neutral integer comparison", "ℤ → Bool", "λ x → intLe x (negsuc 0)"}
  };
  int ok = 1;
  size_t i;
  for (i = 0; i < sizeof checks / sizeof checks[0]; i++)
    ok &= check_case(checks[i].label, checks[i].source, checks[i].code);
  for (i = 0; i < sizeof rounds / sizeof rounds[0]; i++)
    ok &= round_trip(rounds[i].label, rounds[i].signature, rounds[i].body, 0);
  for (i = 0; i < sizeof displays / sizeof displays[0]; i++)
    ok &= round_trip(displays[i].label, displays[i].signature, displays[i].body, 1);
  if (ok) printf("cubical regressions: %zu checks, %zu round trips passed\n",
                 sizeof checks / sizeof checks[0],
                 sizeof rounds / sizeof rounds[0] + sizeof displays / sizeof displays[0]);
  return ok ? 0 : 1;
}
