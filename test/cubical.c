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
  if (checked && (machine.def_count > machine.def_cap || machine.family_count > machine.family_cap || machine.ctor_count > machine.ctor_cap)) {
    fprintf(stderr, "FAIL cubical %s: checker table capacity exceeded (%u/%u defs, %u/%u families, %u/%u ctors)\n",
            label, machine.def_count, machine.def_cap, machine.family_count, machine.family_cap, machine.ctor_count, machine.ctor_cap);
    ok = 0;
  }
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
    {"generated op definitions fit checker tables",
     "replicas A B\n\natoms Tag := t\n\nreplicated Box\n  state tags : GSet Tag\n"
     "  op put0 := add tags t\n  op put1 := add tags t\n  op put2 := add tags t\n"
     "  op put3 := add tags t\n  op put4 := add tags t\n  op put5 := add tags t\n"
     "  op put6 := add tags t\n  op put7 := add tags t\n  op put8 := add tags t\n"
     "  op put9 := add tags t\n  op put10 := add tags t\n  op put11 := add tags t\n"
     "  op put12 := add tags t\n  op put13 := add tags t\n", NULL},
    {"MVReg writes do not cover a Counter decrement",
     "replicas A B\n\natoms User := ana\n\n"
     "replicated Stock\n  state count : Counter (init 1)\n  state owner : MVReg User\n"
     "  op take := write owner ana; dec count 1\n  invariant nonneg count\n",
     "REFUSE_NEEDS_COORDINATION"},
    {"fold maps path fields without path constructors",
     "data W (A : Type 0) : Type where\n"
     "  wPure : (wOf : A) → W A\n"
     "  wLink : (wLeft : W A) → (wProof : Path (W A) wLeft wLeft) → W A\n\n"
     "extract : I → W ℕ → ℕ\n"
     "extract i s = fold (λ (a : ℕ) → a) (λ (x : ℕ) (p : Path ℕ x x) → p i) s\n\n"
     "value : I → ℕ\nvalue i = extract i (wLink ℕ (wPure ℕ 3) refl)\n\n"
     "claimed : (i : I) → Path ℕ (value i) 3\nclaimed i = refl\n", NULL},
    {"read binders stay distinct and preserve shadowing",
     "replicas A B\n\natoms User := ana ∣ ben ∣ cy\n\n"
     "pick : Read 3 → User\n"
     "pick v = fold ana (λ (x : Fin 3) → x) (λ (c : (s : GSet 3) × Path Bool (natLe 2 (gsetCount 3 s)) true) → cy) v\n\n"
     "replicated Desk\n  state owner : MVReg User\n  state tags : GSet User\n"
     "  op claim := x ← read owner; x' ← read tags; add tags (pick x)\n"
     "  op shadow := x ← read owner; x ← read tags; add tags (pick x)\n"
     "  op bind (x : User) := x' ← read owner; x'' ← read tags; add tags x\n"
     "  op assign := write owner ben\n\n"
     "run : Option Desk → Desk\nrun o = elim (λ (p : Option Desk) → Desk) DeskBot (λ (s : Desk) → s) o\n\n"
     "claimed : Path Bool (payload (run (DeskClaim A (run (DeskAssign A DeskBot)))) ben) true\nclaimed = refl\n\n"
     "shadowed : Path Bool (payload (run (DeskShadow A (run (DeskAssign A DeskBot)))) ana) true\nshadowed = refl\n\n"
     "bound : Path Bool (payload (run (DeskBind A cy DeskBot)) cy) true\nbound = refl\n", NULL},
    {"runner helpers preserve public run operations",
     "replicas A B\n\natoms Tag := red ∣ blue\n\nreplicated Tags\n"
     "  state tags : GSet Tag\n  op run := add tags red\n  op run' := add tags blue\n\n"
     "first : Path (Option Tags) (TagsRun A TagsBot) (some (gsetAdd 2 red (gsetBot 2)))\nfirst = refl\n\n"
     "second : Path (Option Tags) (TagsRun' B TagsBot) (some (gsetAdd 2 blue (gsetBot 2)))\nsecond = refl\n", NULL},
    {"script helpers preserve later public operations",
     "replicas A B\n\natoms Tag := red ∣ blue\n\nreplicated Tags\n"
     "  state tags : GSet Tag\n  op tag := add tags red\n"
     "  op tagScript := add tags blue\n  op tagScript' := add tags blue\n\n"
     "first : Path (Option Tags) (TagsTag A TagsBot) (some (gsetAdd 2 red (gsetBot 2)))\nfirst = refl\n\n"
     "second : Path (Option Tags) (TagsTagScript B TagsBot) (some (gsetAdd 2 blue (gsetBot 2)))\nsecond = refl\n\n"
     "third : Path (Option Tags) (TagsTagScript' B TagsBot) (some (gsetAdd 2 blue (gsetBot 2)))\nthird = refl\n", NULL},
    {"script helpers preserve earlier public operations",
     "replicas A B\n\natoms Tag := red ∣ blue\n\nreplicated Tags\n"
     "  state tags : GSet Tag\n  op tagScript := add tags blue\n  op tag := add tags red\n\n"
     "value : Path (Option Tags) (TagsTag A TagsBot) (some (gsetAdd 2 red (gsetBot 2)))\nvalue = refl\n", NULL},
    {"fresh script helpers cannot capture scoped parameters",
     "replicas A B\n\natoms Tag := red ∣ blue\n\nreplicated Tags\n"
     "  state tags : GSet Tag\n  op tag (TagsTagScript : Tag) := add tags TagsTagScript\n"
     "  op tagScript := add tags blue\n\n"
     "value : Path (Option Tags) (TagsTag A blue TagsBot) (some (gsetAdd 2 blue (gsetBot 2)))\nvalue = refl\n", NULL},
    {"duplicate state names refused",
     "replicas A B\n\nreplicated Shop\n"
     "  state stock : Counter (init 1)\n  state stock : Counter (init 7)\n"
     "  invariant nonneg stock\n", "REFUSE_NAME"},
    {"numeric atoms stay members in a GSet op",
     "replicas A B\n\natoms Tag := 10 ∣ 20\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  op tag (t : Tag) := add tags t\n\n"
     "value : Path (Option Shop) (ShopTag A (pack 1 refl) ShopBot) (some (gsetAdd 2 (pack 1 refl) (gsetBot 2)))\n"
     "value = refl\n", NULL},
    {"numeric atoms stay members in an MVReg op",
     "replicas A B\n\natoms User := 10 ∣ 20\n\nreplicated Shop\n"
     "  state owner : MVReg User\n  op assign (u : User) := write owner u\n\n"
     "value : Path (Option Shop) (ShopAssign A (pack 1 refl) ShopBot) (some (mvregWrite 2 2 A (pack 1 refl) (mvregBot 2 2)))\n"
     "value = refl\n", NULL},
    {"op parameter cannot capture some",
     "replicas A B\n\natoms Tag := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  op tag (some : Tag) := add tags some\n\n"
     "value : Path (Option Shop) (ShopTag A work ShopBot) (some (gsetAdd 2 work (gsetBot 2)))\nvalue = refl\n", NULL},
    {"op parameter cannot capture a domain operation",
     "replicas A B\n\natoms Tag := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  op tag (gsetAdd : Tag) := add tags gsetAdd\n\n"
     "value : Path (Option Shop) (ShopTag A work ShopBot) (some (gsetAdd 2 work (gsetBot 2)))\nvalue = refl\n", NULL},
    {"op parameter cannot capture a tuple projection",
     "replicas A B\n\natoms Tag := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  state stock : Counter\n"
     "  op tag (witness : Tag) := add tags witness\n", NULL},
    {"partial op parameter cannot capture payload",
     "replicas A B\n\natoms Amount := 1 ∣ 2\n\nreplicated Shop\n"
     "  state tags : Counter\n  state stock : Counter (init 3)\n"
     "  op sell (payload : Amount) := dec stock payload\n", NULL},
    {"op parameter cannot capture its amount conversion",
     "replicas A B\n\natoms Amount := 1 ∣ 3\n\nreplicated Shop\n"
     "  state stock : Counter (init 5)\n  op sell (AmountValue : Amount) := dec stock AmountValue\n\n"
     "value : Path (Option Shop) (ShopSell A (pack 1 refl) ShopBot) (counterDec 2 5 A 3 (counterBot 2))\nvalue = refl\n", NULL},
    {"generated replica binder cannot capture an atoms type",
     "replicas A B\n\natoms i := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet i\n  op tag (t : i) := add tags t\n", NULL},
    {"generated state binder cannot capture an argument annotation",
     "replicas A B\n\natoms s := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet s\n  op tag (t : s) := add tags ((λ (x : s) → x) t)\n", NULL},
    {"generated motive binder cannot capture the shop type",
     "replicas A B\n\natoms Amount := 1\n\nreplicated o\n"
     "  state tags : Counter\n  state stock : Counter (init 1)\n"
     "  op sell (k : Amount) := dec stock k\n", NULL},
    {"operation word is outside the parameter scope",
     "replicas A B\n\natoms Tag := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  op tag (add : Tag) := add tags add\n\n"
     "value : Path (Option Shop) (ShopTag A work ShopBot) (some (gsetAdd 2 work (gsetBot 2)))\nvalue = refl\n", NULL},
    {"state label is outside the parameter scope",
     "replicas A B\n\natoms Tag := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  op tag (tags : Tag) := add tags tags\n\n"
     "value : Path (Option Shop) (ShopTag A work ShopBot) (some (gsetAdd 2 work (gsetBot 2)))\nvalue = refl\n", NULL},
    {"renamed parameter avoids nested argument binders",
     "replicas A B\n\natoms Tag := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  op tag (t : Tag) := add tags ((λ (t' : Tag) → t) urgent)\n\n"
     "value : Path (Option Shop) (ShopTag A work ShopBot) (some (gsetAdd 2 work (gsetBot 2)))\nvalue = refl\n", NULL},
    {"argument binder still shadows an op parameter",
     "replicas A B\n\natoms Tag := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  op tag (t : Tag) := add tags ((λ (t : Tag) → t) urgent)\n\n"
     "value : Path (Option Shop) (ShopTag A work ShopBot) (some (gsetAdd 2 urgent (gsetBot 2)))\nvalue = refl\n", NULL},
    {"last repeated op parameter binds the argument",
     "replicas A B\n\natoms Tag := urgent ∣ work\n\nreplicated Shop\n"
     "  state tags : GSet Tag\n  op tag (t : Tag) (t : Tag) := add tags t\n\n"
     "value : Path (Option Shop) (ShopTag A urgent work ShopBot) (some (gsetAdd 2 work (gsetBot 2)))\nvalue = refl\n", NULL},
    {"op without parameters",
     "replicas A B\n\nreplicated Shop\n  state stock : Counter (init 1)\n  op sell := dec stock 1\n", NULL},
    {"natMax at nested open successors",
     "value : (a : ℕ) → (b : ℕ) → Path ℕ (natMax (natAdd (natAdd a 1) 1) (natAdd (natAdd b 1) 1)) (natAdd (natAdd (natMax a b) 1) 1)\nvalue a b = refl\n", NULL},
    {"natMax with a larger left offset",
     "value : (a : ℕ) → (b : ℕ) → Path ℕ (natMax (natAdd (natAdd a 1) 1) (natAdd b 1)) (natAdd (natMax (natAdd a 1) b) 1)\nvalue a b = refl\n", NULL},
    {"natMax with a larger right offset",
     "value : (a : ℕ) → (b : ℕ) → Path ℕ (natMax (natAdd a 1) (natAdd (natAdd b 1) 1)) (natAdd (natMax a (natAdd b 1)) 1)\nvalue a b = refl\n", NULL},
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
    /* Builtin ℕ lemmas (Q53 (b)): per lemma a row that types and a misuse row. */
    {"natEqRefl at a literal", "value : Path Bool (natEq 5 5) true\nvalue = natEqRefl 5\n", NULL},
    {"natEqRefl at a Bool", "value : Path Bool (natEq 0 0) true\nvalue = natEqRefl true\n", "TYPE_MISMATCH"},
    {"natEqSound", "value : (a : ℕ) → (b : ℕ) → (e : Path Bool (natEq a b) true) → Path ℕ a b\nvalue a b e = natEqSound a b e\n", NULL},
    {"natEqSound from natLe", "value : (a : ℕ) → (b : ℕ) → (e : Path Bool (natLe a b) true) → Path ℕ a b\nvalue a b e = natEqSound a b e\n", "TYPE_MISMATCH"},
    {"natLeRefl", "value : (k : ℕ) → Path Bool (natLe k k) true\nvalue k = natLeRefl k\n", NULL},
    {"natLeRefl at a wrong type", "value : (k : ℕ) → Path Bool (natLe (natAdd k 1) k) true\nvalue k = natLeRefl k\n", "TYPE_MISMATCH"},
    {"natLeTrans",
     "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → (p : Path Bool (natLe a b) true) → (q : Path Bool (natLe b c) true) → Path Bool (natLe a c) true\n"
     "value a b c p q = natLeTrans a b c p q\n", NULL},
    {"natLeTrans with q not from b",
     "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → (p : Path Bool (natLe a b) true) → (q : Path Bool (natLe a c) true) → Path Bool (natLe a c) true\n"
     "value a b c p q = natLeTrans a b c p q\n", "TYPE_MISMATCH"},
    {"natLeMaxL", "value : (a : ℕ) → (b : ℕ) → Path Bool (natLe a (natMax a b)) true\nvalue a b = natLeMaxL a b\n", NULL},
    {"natLeMaxL at the right side", "value : (a : ℕ) → (b : ℕ) → Path Bool (natLe b (natMax a b)) true\nvalue a b = natLeMaxL a b\n", "TYPE_MISMATCH"},
    {"natLeMaxR", "value : (a : ℕ) → (b : ℕ) → Path Bool (natLe b (natMax a b)) true\nvalue a b = natLeMaxR a b\n", NULL},
    {"natLeMaxR at a path", "value : (a : ℕ) → (e : Path ℕ a a) → Path Bool (natLe a (natMax a a)) true\nvalue a e = natLeMaxR a e\n", "TYPE_MISMATCH"},
    {"natMaxLeast",
     "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → (p : Path Bool (natLe a c) true) → (q : Path Bool (natLe b c) true) → Path Bool (natLe (natMax a b) c) true\n"
     "value a b c p q = natMaxLeast a b c p q\n", NULL},
    {"natMaxLeast with p reversed",
     "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → (p : Path Bool (natLe c a) true) → (q : Path Bool (natLe b c) true) → Path Bool (natLe (natMax a b) c) true\n"
     "value a b c p q = natMaxLeast a b c p q\n", "TYPE_MISMATCH"},
    {"natLeAddMono",
     "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → (d : ℕ) → (p : Path Bool (natLe a b) true) → (q : Path Bool (natLe c d) true) → Path Bool (natLe (natAdd a c) (natAdd b d)) true\n"
     "value a b c d p q = natLeAddMono a b c d p q\n", NULL},
    {"natLeAddMono with q from p",
     "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → (d : ℕ) → (p : Path Bool (natLe a b) true) → (q : Path Bool (natLe a b) true) → Path Bool (natLe (natAdd a c) (natAdd b d)) true\n"
     "value a b c d p q = natLeAddMono a b c d p q\n", "TYPE_MISMATCH"},
    {"natLeSucc", "value : (k : ℕ) → Path Bool (natLe 1 (natAdd k 1)) true\nvalue k = natLeSucc k\n", NULL},
    {"natLeSucc at a wrong type", "value : (k : ℕ) → Path Bool (natLe 2 (natAdd k 1)) true\nvalue k = natLeSucc k\n", "TYPE_MISMATCH"},
    {"natEqSuccNe", "value : (k : ℕ) → Path Bool (natEq (natAdd k 1) k) false\nvalue k = natEqSuccNe k\n", NULL},
    {"natEqSuccNe at true", "value : (k : ℕ) → Path Bool (natEq (natAdd k 1) k) true\nvalue k = natEqSuccNe k\n", "TYPE_MISMATCH"},
    {"sumFinMono",
     "value : (r : ℕ) → (f : Fin r → ℕ) → (g : Fin r → ℕ) → (p : (j : Fin r) → Path Bool (natLe (f j) (g j)) true) → Path Bool (natLe (sumFin r f) (sumFin r g)) true\n"
     "value r f g p = sumFinMono r f g p\n", NULL},
    {"sumFinMono with p reversed",
     "value : (r : ℕ) → (f : Fin r → ℕ) → (g : Fin r → ℕ) → (p : (j : Fin r) → Path Bool (natLe (g j) (f j)) true) → Path Bool (natLe (sumFin r f) (sumFin r g)) true\n"
     "value r f g p = sumFinMono r f g p\n", "TYPE_MISMATCH"},
    {"natLeMulTwo",
     "value : (a : ℕ) → (b : ℕ) → (p : Path Bool (natLe a b) true) → Path Bool (natLe (natMul 2 a) (natMul 2 b)) true\n"
     "value a b p = natLeMulTwo a b p\n", NULL},
    {"natLeMulTwo with p reversed",
     "value : (a : ℕ) → (b : ℕ) → (p : Path Bool (natLe b a) true) → Path Bool (natLe (natMul 2 a) (natMul 2 b)) true\n"
     "value a b p = natLeMulTwo a b p\n", "TYPE_MISMATCH"},
    {"natLeAntisym",
     "value : (a : ℕ) → (b : ℕ) → (p : Path Bool (natLe a b) true) → (q : Path Bool (natLe b a) true) → Path Bool (natEq a b) true\n"
     "value a b p q = natLeAntisym a b p q\n", NULL},
    {"natLeAntisym with q not from b a",
     "value : (a : ℕ) → (b : ℕ) → (p : Path Bool (natLe a b) true) → (q : Path Bool (natLe a b) true) → Path Bool (natEq a b) true\n"
     "value a b p q = natLeAntisym a b p q\n", "TYPE_MISMATCH"},
    {"natEqSym",
     "value : (a : ℕ) → (b : ℕ) → Path Bool (natEq a b) (natEq b a)\nvalue a b = natEqSym a b\n", NULL},
    {"natEqSym at a wrong end",
     "value : (a : ℕ) → (b : ℕ) → Path Bool (natEq a b) true\nvalue a b = natEqSym a b\n", "TYPE_MISMATCH"},
    {"natMulTwoSucc",
     "value : (k : ℕ) → Path ℕ (natMul 2 (natAdd k 1)) (natAdd (natMul 2 k) 2)\nvalue k = natMulTwoSucc k\n", NULL},
    {"natMulTwoSucc at a wrong end",
     "value : (k : ℕ) → Path ℕ (natMul 2 (natAdd k 1)) (natAdd (natMul 2 k) 1)\nvalue k = natMulTwoSucc k\n", "TYPE_MISMATCH"},
    {"sumFinLe",
     "value : (r : ℕ) → (f : Fin r → ℕ) → (k : Fin r) → Path Bool (natLe (f k) (sumFin r f)) true\n"
     "value r f k = sumFinLe r f k\n", NULL},
    {"sumFinLe with the sum on the left",
     "value : (r : ℕ) → (f : Fin r → ℕ) → (k : Fin r) → Path Bool (natLe (sumFin r f) (f k)) true\n"
     "value r f k = sumFinLe r f k\n", "TYPE_MISMATCH"},
    {"sumFinLeTwo",
     "value : (r : ℕ) → (f : Fin r → ℕ) → (k : Fin r) → (l : Fin r) → (h : Path Bool (natEq (witness k) (witness l)) false) → Path Bool (natLe (natAdd (f k) (f l)) (sumFin r f)) true\n"
     "value r f k l h = sumFinLeTwo r f k l h\n", NULL},
    {"sumFinLeTwo with h true",
     "value : (r : ℕ) → (f : Fin r → ℕ) → (k : Fin r) → (l : Fin r) → (h : Path Bool (natEq (witness k) (witness l)) true) → Path Bool (natLe (natAdd (f k) (f l)) (sumFin r f)) true\n"
     "value r f k l h = sumFinLeTwo r f k l h\n", "TYPE_MISMATCH"},
    {"natEqRefl ends", "value : (k : ℕ) → Path Bool (natEq k k) true\nvalue k = natEqRefl k\n", NULL},
    {"cong over natEqSound",
     "value : (a : ℕ) → (b : ℕ) → (e : Path Bool (natEq a b) true) → Path ℕ (natAdd a 1) (natAdd b 1)\n"
     "value a b e = cong (λ (n : ℕ) → natAdd n 1) (natEqSound a b e)\n", NULL},
    {"natLeSucc at a literal", "value : Path Bool (natLe 1 4) true\nvalue = natLeSucc 3\n", NULL},
    {"natAddAssoc", "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → Path ℕ (natAdd (natAdd a b) c) (natAdd a (natAdd b c))\nvalue a b c = natAddAssoc a b c\n", NULL},
    {"natAddAssoc reversed", "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → Path ℕ (natAdd a (natAdd b c)) (natAdd (natAdd a b) c)\nvalue a b c = natAddAssoc a b c\n", "TYPE_MISMATCH"},
    {"natAddComm", "value : (a : ℕ) → (b : ℕ) → Path ℕ (natAdd a b) (natAdd b a)\nvalue a b = natAddComm a b\n", NULL},
    {"natAddComm at one order", "value : (a : ℕ) → (b : ℕ) → Path ℕ (natAdd a b) (natAdd a b)\nvalue a b = natAddComm a b\n", "TYPE_MISMATCH"},
    {"sumFinBumpLe", "value : (r : ℕ) → (f : Fin r → ℕ) → (j : Fin r) → (k : ℕ) → Path Bool (natLe (sumFin r (λ (l : Fin r) → flagIf (natEq (witness l) (witness j)) (natAdd (f l) k) (f l))) (natAdd (sumFin r f) k)) true\nvalue r f j k = sumFinBumpLe r f j k\n", NULL},
    {"sumFinBumpLe without k", "value : (r : ℕ) → (f : Fin r → ℕ) → (j : Fin r) → (k : ℕ) → Path Bool (natLe (sumFin r (λ (l : Fin r) → flagIf (natEq (witness l) (witness j)) (natAdd (f l) k) (f l))) (sumFin r f)) true\nvalue r f j k = sumFinBumpLe r f j k\n", "TYPE_MISMATCH"},
    {"sumFinAdd", "value : (r : ℕ) → (f : Fin r → ℕ) → (g : Fin r → ℕ) → Path ℕ (sumFin r (λ (j : Fin r) → natAdd (f j) (g j))) (natAdd (sumFin r f) (sumFin r g))\nvalue r f g = sumFinAdd r f g\n", NULL},
    {"sumFinAdd with f twice", "value : (r : ℕ) → (f : Fin r → ℕ) → (g : Fin r → ℕ) → Path ℕ (sumFin r (λ (j : Fin r) → natAdd (f j) (g j))) (natAdd (sumFin r f) (sumFin r f))\nvalue r f g = sumFinAdd r f g\n", "TYPE_MISMATCH"},
    {"sumFinSwap", "value : (r : ℕ) → (f : Fin r → Fin r → ℕ) → Path ℕ (sumFin r (λ (a : Fin r) → sumFin r (λ (b : Fin r) → f a b))) (sumFin r (λ (b : Fin r) → sumFin r (λ (a : Fin r) → f a b)))\nvalue r f = sumFinSwap r f\n", NULL},
    {"sumFinSwap at f b a", "value : (r : ℕ) → (f : Fin r → Fin r → ℕ) → Path ℕ (sumFin r (λ (a : Fin r) → sumFin r (λ (b : Fin r) → f a b))) (sumFin r (λ (b : Fin r) → sumFin r (λ (a : Fin r) → f b a)))\nvalue r f = sumFinSwap r f\n", "TYPE_MISMATCH"},
    {"natLeAddCancel", "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → (p : Path Bool (natLe (natAdd a c) (natAdd b c)) true) → Path Bool (natLe a b) true\nvalue a b c p = natLeAddCancel a b c p\n", NULL},
    {"natLeAddCancel without c", "value : (a : ℕ) → (b : ℕ) → (c : ℕ) → (p : Path Bool (natLe a b) true) → Path Bool (natLe a b) true\nvalue a b c p = natLeAddCancel a b c p\n", "TYPE_MISMATCH"},
    {"sumFinZero", "value : (r : ℕ) → Path ℕ (sumFin r (λ (j : Fin r) → 0)) 0\nvalue r = sumFinZero r\n", NULL},
    {"finEqSound", "value : (r : ℕ) → (a : Fin r) → (b : Fin r) → (e : Path Bool (natEq (witness a) (witness b)) true) → Path (Fin r) a b\nvalue r a b e = finEqSound r a b e\n", NULL},
    {"finEqSound from natLe", "value : (r : ℕ) → (a : Fin r) → (b : Fin r) → (e : Path Bool (natLe (witness a) (witness b)) true) → Path (Fin r) a b\nvalue r a b e = finEqSound r a b e\n", "TYPE_MISMATCH"},
    {"sumFinZero at 1", "value : (r : ℕ) → Path ℕ (sumFin r (λ (j : Fin r) → 0)) 1\nvalue r = sumFinZero r\n", "TYPE_MISMATCH"},
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
    {"dependent fold at open two", "(y : ℕ) → Path ℕ (natAdd y 2) (natAdd y 2)",
     "λ y → fold (λ (k : ℕ) → Path ℕ k k) (λ (k : ℕ) (ih : Path ℕ k k) → refl) refl (natAdd y 2)"},
    {"dependent fold at open three",
     "(P : ℕ → Type 0) → (s : (k : ℕ) → P k → P (natAdd k 1)) → (z : P 0) → (y : ℕ) → P (natAdd y 3)",
     "λ P s z y → fold P s z (natAdd y 3)"},
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
    {"annotated lambda", "ℕ → ℕ", "λ (x : ℕ) → natAdd x 1"},
    {"neutral natEqRefl", "(k : ℕ) → Path Bool (natEq k k) true", "λ k → natEqRefl k"},
    {"neutral natLeTrans",
     "(a : ℕ) → (b : ℕ) → (c : ℕ) → Path Bool (natLe a b) true → Path Bool (natLe b c) true → Path Bool (natLe a c) true",
     "λ a b c p q → natLeTrans a b c p q"},
    {"builtin lemma binder stays free", "(natLeSucc : ℕ) → Path Bool (natLe 0 natLeSucc) true", "λ natLeSucc → natLeZ natLeSucc"}
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
