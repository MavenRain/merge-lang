# merge-lang plan

merge-lang is a language for offline replicated state with checked merge
laws. A program declares replicated state, the permitted operations, the
merge of each state part and the domain invariants. The checker puts each
pair of concurrent operations in one of three classes: independent,
explicit conflict, or needs coordination. For a pair that needs
coordination, it gives a concrete counterexample or it accepts a declared
resource allocation rule. The host is TinyCC. The v1 targets are a Wasm
merge engine, a deterministic replica simulator (`langc sim`) and a JSON
coordination report.

S1b status: the tcc-wasm kit seed of lang-template, `design/DESIGN.md`, this
plan, `README.md`, `AGENTS.md`, `docs/COMPARISON.md` and slices S1 and S1b
exist. S1 is the port of evolve-lang 1f87a5a (section 8): the surface,
`langc fmt`, `W_NONCANON`, `check --json` and the cubical kernel. S1b adds
induction to the kernel (O15): the builtin `natMax` with its rules on open
arguments, a `fold` over ℕ with a motive, the dependent `elim` for Bool and
`Option`, and the builtin `sumFin`. S2 builds the merge-lang domain on the
S1b kernel (O4 DONE): the parts, `Read n`, the product join and L1 to L4.
The O12 replan is DONE (S2 unit 1, 2026-10-27). Section 9 gives the slices.

merge-lang does not prove facts about the network. It assumes that each
replica state that is sent arrives some time, and it does not model loss,
durability, Byzantine replicas or clocks (section 6.2). A law has one of
three marks: proved (a guest term type-checks), bounded (the simulator
found no violation in all schedules up to the bounds; this is NOT a proof)
or refuted (a counterexample). The report never says proved for a bounded
result.

## 1. Rulings and readings

USER rulings (2026-10-27, the request and AskUserQuestion; the words are in
`design/DESIGN.md` sections 1 and 2):

- R1 type formers: for type constructors and higher kinds, only cell monads
  and their duals "as represented computationally in Cubical Type Theory".
  This ask does not have the words "in a locally presentable locally
  cartesian closed (infinity,1)-category" of the earlier asks (intent-lang
  to evolve-lang). Section 2.1 gives the reading.
- R2 base types and operations: only what the pasted merge-lang
  description needs. Section 3 cites the sentence for each admitted name.
- R3 host: TinyCC.
- R4 v1 targets: a Wasm merge engine (`langc build`), a replica simulator
  (`langc sim`) and a JSON coordination report. NOT chosen: an Automerge
  differential harness. Automerge is a docs comparison only
  (`docs/COMPARISON.md`), with no npm dependency.
- R5 first demo: a CLI transcript, `langc sim examples/inventory.merge`.
  The offline gate compares it byte for byte with a golden file.
- R6 agent adoption: the six bullets of the pasted list (section 7).

Readings RULED by the USER on 2026-10-27 (section 11 has the marks):

- Kit and port path: the tcc-wasm seed (done in S0). S1 ports the
  surface and the cubical kernel of evolve-lang 1f87a5a in one patch.
  evolve-lang has the same seed and did this work in its S1 to S2d (Q2).
  S6 ports the JSON writer of `hosts/tcc-json`.
- Wasm target: TinyCC has no Wasm back end, so `langc` itself is not a
  Wasm module. The Wasm writer of `langc` emits one engine module per
  program (Q3, Q4, Q21).
- Simulator: native C in `langc`. Its step function is the guest
  evaluator that `langc eval` uses. The differential leg runs the same
  schedules on the emitted module under node (Q3, section 5).
- Merge model: state based. A state is a join-semilattice and merge is the
  join, the colimit in the posetal category of states (the automerge-cat
  precedent, section 4.1). Each operation is inflationary (Q5).
- Discharge marks: proved, bounded, refuted (Q6).
- Surface: the Agda-like Unicode surface (Q7). Proofs: guest language
  only (Q8). Declaration forms (Q9).
- The three examples of the description map to three library state
  parts: `GSet` (tags), `MVReg` (task owner), `Counter` and `Escrow`
  (inventory) (Q10, Q11). The inventory has `sell` and `restock`
  (increments, Balegas `R[i][i]`) (Q12 (b)). ℤ is admitted (Q13 (b)).
- Formers admitted by the USER beyond the recommendation: `FMSet` in
  generator form (Q14 (b)), the free CM of operations `Ops A` (Q15 (b)),
  and a `Replica` cofamily, a Moore machine (Q16 (b)). Section 2.2 marks
  them. The O12 replan (Q22 to Q25 (a)) puts ℤ in S2b, `FMSet` in S2c,
  `Ops A` in S3b and `Replica` in S4.

## 2. The reading of R1

### 2.1 The precedent reading

merge-lang takes the reading of intent-lang (PLAN 2.2, USER ruling S1 Q1
(a) and Q2 (a), 2026-10-22), leaf-lang (PLAN 2.1) and evolve-lang (PLAN
2.1, ruled (a) 2026-10-25) (Q1):

| Part | Cubical TT | R1 limit |
|---|---|---|
| Ambient | Pi, Sigma, `Path`, `PathP`, `I`, `i0`, `i1`, `transp`, `hcomp`, `U`, the builtin `Unit` | none: the internal language of the model |
| CM, cell monad | a `family` with a type parameter: the parameter is the one field of one generator `eta : A -> T A`; each other constructor takes closed base data and arguments of type `T A`, or is a path in `T A` | else `REFUSE_CELL` |
| CC, cocell comonad | a `cofamily` with one observation `eps : D A -> A`; each other field has type `D A -> D A` or `D A -> E -> D A` with `E` closed data | else `REFUSE_CELL` |

The wording difference. The earlier asks named the ambient model (a
locally presentable, locally cartesian closed (infinity,1)-category). In
the precedent, that clause gave two facts: each cell signature has its
free algebra (local presentability), and Pi and Sigma exist (local
cartesian closure). This ask says "as represented computationally in
Cubical Type Theory". Cubical TT has Pi, Sigma and higher inductive types
as computing terms, so the same two facts hold. The reading: the dropped
clause named the model, not an extra former. The enforcement is the same.

Classification rules of the precedent, kept:

- A family with no type parameter is a base type, not a type constructor.
  R2 limits it, not R1. This includes an indexed family with no type
  parameter, such as `Read n` (2.2).
- A higher inductive type with a type parameter is a CM when its point
  constructors pass the CM test and its other constructors are paths
  (quorum-lang PLAN 2: `FMSet` in generator form, with unit,
  associativity, commutativity and set truncation cells, is a CM).
- The cons form `_∷_ : A → FMSet A → FMSet A` and cons `List` are
  REFUSED: they use `A` outside the generator.
- `Maybe` (one 0-cell `nothing`, generator `just`) and `Trunc` (one 1-cell
  `squash`) are CMs.
- Only `domain/domain.lang` declares base data with no type argument.
  A program can declare a CM or CC signature that passes R1. A program
  base data declaration gets `REFUSE_DATA`.

### 2.2 Every type former merge-lang needs

| Former | Use in merge-lang | Class | Reason |
|---|---|---|---|
| Pi, `A → B` | `apply`, `merge`, `inv`, the read functions; vectors `Fin n → X` | ambient | the internal language |
| Sigma, `(x : A) × B` | the state of a `replicated` declaration (iterated Sigma of its parts); the pairs `Counter r`, `Escrow r` and `MVReg n r` (4.2) | ambient | no `record` form (as evolve-lang) |
| `Fin n` | replica names, atom names (tags, users), amounts | defined | the evolve-lang S2d builtin: `(k : ℕ) × Path Bool (natLe (natAdd k 1) n) true` |
| vectors `Fin n → X` | a set of atoms (`Fin n → Bool`), a version vector (`Fin r → ℕ`), a rights matrix (`Fin r → Fin r → ℕ`) | ambient | no new former |
| `Path`, `PathP`, `I`, `transp`, `hcomp` | the laws (section 6) | ambient | the identity type of Cubical TT |
| `U`, `Glue`, `ua` | none in v1 | ambient, not ported | no merge-lang law needs univalence (O5) |
| replica state | the product of the declared parts | ambient | Sigma of base types; a join-semilattice by the product law (L1 to L3) |
| `GSet n`, `MVReg n r`, `Counter r`, `Escrow r` | the state parts (section 4.2) | base (R2) | defined as Sigma and vectors over base types; no type parameter |
| `Read n` | the read of an `MVReg`: `readEmpty`, `readOne (readValue : Fin n)`, `readConflict (readSets : (s : GSet n) × Path Bool (natLe 2 (gsetCount n s)) true)`; a program reads it with the non-dependent `fold` (one argument for each constructor, no motive) | base, indexed (R2) | no type parameter (Q11) |
| `Op` of a program | the operations of a declaration, with their arguments | base (R2) | a closed sum that the declaration generates |
| `Option A` | the result of `apply`: `none` when the local precondition fails | CM | one 0-cell `none`, generator `some` |
| op sequences | the changes of one replica in a schedule | ambient | `Fin m → Op`, a vector; an `op` body can be an `Ops` script (S3b, Q24 (a)) |
| schedules, traces | none in the guest | not admitted | the simulator enumerates them in C (Q16) |
| causal context | the `MVReg` context `ctx : Fin r → ℕ` (the second component) | ambient | a version vector; no new former |
| `FMSet A` | the multiset of states that a replica merged; `mergeAll : FMSet S → S` states L5 (S2d, Q23 (a), Q29 (a)); not a state part: its union is not idempotent | CM, ADMITTED (Q14 (b)) | generator form with unit, associativity, commutativity and set truncation cells (quorum-lang PLAN 2); the truncation cell has dimension 2 (S2c kernel port); program data, not domain data (Q30 (b)) |
| `Pfin⁺ A`, the free join-semilattice | none in v1 | not admitted | the reading of `Read` as the free join-semilattice needs set truncation, a 2-cell (Q11) |
| free CM of operations `Ops A` (mandate-lang 4.6) | `op` bodies: scripts whose later part operations read earlier part reads (S3b, Q24 (a)) | CM, ADMITTED (Q15 (b)) | the mandate-lang 4.6 `IO` form as built in its S6 s2: one constructor per part read, each with a field `E → Ops A`, `E` closed; the S1 CM test refuses that field, S3b changes it |
| `Replica A` cofamily, a Moore machine (mandate-lang 4.7) | L5r: convergence as a statement about guest replicas (S4, Q25 (a)) | CC, ADMITTED (Q16 (b)) | the mandate-lang 4.7 `Machine` form: `eps : A`, `step : Event → Replica A`, `Event` closed; the S1 codata test admits it |
| `Trunc A` | none in v1 | not admitted | the laws are Pi types into `Path` of sets; no law needs proof irrelevance |
| `Stream A` | none | not admitted | schedules are finite |
| cons `List A`, cons `FMSet A` | none | REFUSED | `REFUSE_CELL`: `A` occurs outside the generator |
| `data Pair (A : U) := mk A A` | none | REFUSED | the precedent `REFUSE_CELL` example; `A × A` is the admitted form |

The kernel port (S1) keeps the CM and CC test and its fixtures. A former
that is "not admitted" above is a former that `domain/domain.lang` does
not declare. A USER ask can add it later, and the test then runs on it.

## 3. The minimal base signature (R2)

### 3.1 The description sentences

The sentences of the pasted description (`design/DESIGN.md` section 1),
numbered for the admission table:

- D1 "Replicas can converge to the same answer while still producing an
  answer the application considers wrong."
- D2 "Describe replicated state, permitted operations, merge behavior, and
  domain invariants."
- D3 "Concurrent tag additions should both survive."
- D4 "Concurrent assignment of a task owner should remain an explicit
  conflict."
- D5 "An offline inventory decrement needs a resource allocation rule if
  stock must never become negative."
- D6 "You could express merge laws and prove selected invariants, while
  distinguishing operations that work independently from those requiring
  coordination."
- D7 "First target: A Wasm merge engine and deterministic replica
  simulator. Existing Automerge conflict semantics offer a useful
  comparison."
- D8 "Two disconnected replicas make changes, reconnect, and either
  preserve the declared invariant or produce a concrete counterexample
  explaining why coordination is necessary."
- D9 "This would occupy a distinct space from dil-lang’s reactive
  interfaces and tether’s Redis scripts." (no name; README only)

### 3.2 Admission table

| Name | Kind | Serves | Slice |
|---|---|---|---|
| `Bool`, `true`, `false`, `and`, `or`, `not` | base type, ops | D2 (an invariant is a predicate), D6 | S1 (kernel) |
| `ℕ`, `natAdd`, `natLe`, `natMax`, `natEq` | base type, ops | D5 (stock, decrements, rights), D4 (version counters) | S1 (kernel), S1b (`natMax` and its rules on open arguments, Q26 (a), Q27 (a)) |
| `Fin n`, `finEq` | base type, op | D3, D4 (atom names), D8 (replica names) | S1 (kernel) |
| `sumFin : (n : ℕ) → (Fin n → ℕ) → ℕ` | op (builtin: it unrolls at a numeral `n`, it is stuck at an open `n`; Q28 (a)) | D5 (total decrements, total rights) | S1b |
| `atoms NAME := a ∣ b ∣ ...` | form | D3 (tags), D4 (owners): a finite domain of opaque names | S3 |
| `replicas A B ...` | form | D8 ("two disconnected replicas"); `Replica = Fin r` | S3 |
| `GSet n`, `add`, `has`, `merge` | base type, ops | D3: two concurrent `add` both survive the join | S2 |
| `MVReg n r`, `write`, `read`, `merge` (built as `mvregWrite`, `mvregRead`, `mvregMerge`) | base type, ops | D4: concurrent writes keep each value | S2 |
| `Read n`, `readEmpty`, `readOne`, `readConflict` | base type, constructors | D4: the conflict is a value of the read type, so a program cannot take a winner in silence | S2 |
| `Counter r` (an initial stock, an increment vector and a decrement vector), `dec`, `restock`, `nonneg` | base type, ops | D5: an offline decrement; "stock must never become negative"; `restock` (Q12 (b)) | S2 |
| `Escrow r` (initial split, rights matrix, decrement vector), `dec`, `transfer`, `restock`, `rights` | base type, ops | D5: "a resource allocation rule" (bounded counter, Balegas et al.); `restock` is `R i i` (Q12 (b)) | S2 |
| `ℤ`, `pos`, `negsuc`, `natDiff`, `intLe`; the `stock` read of `Counter` and `Escrow` | base type, ops | D5: the stock that "must never become negative" is a signed value (Q13 (b), Q22 (a)) | S2b |
| `FMSet A` (`msing`, `mnil`, `munion`, `munit`, `massoc`, `mcomm`, `trunc`), `mergeAll` | CM, op | D8: replicas that reconnect converge (L5, Q14 (b), Q23 (a)) | S2c `FMSet` as program data (Q30 (b)); S2d `mergeAll` (Q29 (a)) |
| `Ops A` (`opPure` and one constructor per part read) | CM | D2 "permitted operations": an `op` body as a script (Q15 (b), Q24 (a)) | S3b |
| `Replica A` (`eps`, `step`), `Event` (`local`, `recv`) | CC, base type | D8 "two disconnected replicas" as guest values (L5r, Q16 (b), Q25 (a)) | S4 |
| `Option`, `none`, `some` | CM | D2 ("permitted operations"): `apply` gives `none` when the local precondition fails | S1 (kernel) |
| `replicated`, `state`, `op`, `invariant`, `allocate` | forms | D2 (state, operations, merge, invariants), D5 (`allocate`) | S3 |
| merge laws L1 to L4 | guest terms | D6 "express merge laws" | S2 |
| classes, I-confluence check, `REFUSE_NEEDS_COORDINATION` | judgements | D6 "distinguishing operations", D1 | S4 |
| invariant laws L6, L7 | guest terms | D6 "prove selected invariants" | S4 |
| schedules, counterexample | simulator | D7, D8 | S5 |
| engine module | target | D7 | S7 |

### 3.3 Refused candidates

| Candidate | Reason |
|---|---|
| strings, text values | no sentence needs text; an atom is an opaque name in a declared finite domain |
| floats | D7 asks for a deterministic engine; no sentence needs them |
| wall-clock time, timestamps, last-writer-wins register | D4: a concurrent owner write "should remain an explicit conflict"; last-writer-wins takes a winner in silence; no clocks (determinism) |
| `remove` on tags, the add-wins OR-set with dots | D3 names additions only (Q10) |
| sequence and text CRDTs (RGA, Peritext) | no sentence; Automerge covers text |
| JSON documents, nested maps (the Automerge document model) | no sentence; a state is a fixed product of declared parts |
| a merge function that the user writes | D6 "checked merge laws": v1 merges are library merges with proved laws (`REFUSE_NO_MERGE`); a user merge with proof terms is O6 |
| unbounded operation arguments | the simulator enumerates arguments; an argument has a finite type (`REFUSE_OP_INFINITE_ARG`) |
| consensus, locks, leases | D6 asks to distinguish operations that need coordination, not to build a coordination protocol; `transfer` is the one coordination operation |
| membership changes (add or remove a replica) | no sentence; `replicas` fixes the set |
| op-based or delta-state messages | the reading is state based (Q5) |

The S0 plan refused increments (`restock`), `ℤ`, `FMSet`, the free CM of
operations and a replica cofamily. The USER ruled Q12 to Q16 (b) on
2026-10-27: all five are admitted. The O12 replan puts them in 3.2 and
section 9.

## 4. The merge model

### 4.1 Objects

- A replica is a name in `replicas A B ...`; `Replica = Fin r`.
- A state is the Sigma of the declared parts. Each part type `P` has a
  bottom `⊥ : P`, a join `merge : P → P → P` and an order
  `le a b := Path P (merge a b) b`. The state join is the join of each
  part (the product of join-semilattices). This is the automerge-cat
  reading: merge is the colimit (join) in the posetal category of states.
- An operation `o` with arguments `x` runs at a replica `i`:
  `apply : Replica → Op → State → Option State`. It gives `none` when the
  local precondition fails at `i`. An operation is a composition of part
  operations, or an `Ops` script of part reads and part operations that
  runs at one replica with no merge between its steps (S3b). So it is
  inflationary (L4).
- An invariant is a predicate `I : State → Bool`. A program can declare
  several. `inv` is their conjunction.
- The initial state is `⊥` with the declared initial values (the initial
  stock and its split).

### 4.2 The library parts

| Part | State | Operations at replica `i` | Merge | Read |
|---|---|---|---|---|
| `GSet n` | `Fin n → Bool` | `add x`: set bit `x` | pointwise `or` | `has x` |
| `MVReg n r` | dotted lex form (O11): entries `m : Fin r → Fin n → ℕ`, context `ctx : Fin r → ℕ`; `m j y = 2 (ctx j)` when `y` is present at the dot `(j, ctx j)`, `2 (ctx j) + 1` when it is absent | `write x` (`mvregWrite` = merge of `s` with `mvregDelta`): row `i` entry `y` := `2 (ctx i + 1)` + (0 if `y = x`, else 1); row `j ≠ i` entry := `2 (ctx j) + 1` (this clears each value that `i` has seen); `ctx i := ctx i + 1` | `mvregMerge`: `m` and `ctx` by pointwise `natMax` (L1 to L3 from `matMax` and `vecMax`) | `x` is live at `j` when `m j x = 2 (ctx j)` and `ctx j ≥ 1` (`mvregLive`); `mvregRead : MVReg n r → Read n` is `gsetRead` of the live set |
| `Counter r` | initial stock `s0 : ℕ` (declared), increments `p : Fin r → ℕ`, decrements `d : Fin r → ℕ` | `dec k`: precondition `local stock ≥ k`; `d i += k`. `restock k`: no precondition; `p i += k` | `p` and `d` by pointwise `natMax` | `nonneg = natLe (sumFin d) (s0 + sumFin p)` (S2); `stock = natDiff (s0 + sumFin p) (sumFin d) : ℤ` (S2b) |
| `Escrow r` | initial split `Fin r → ℕ` (declared), rights matrix `R : Fin r → Fin r → ℕ`, decrements `U : Fin r → ℕ` | `dec k`: precondition `rights i ≥ k`; `U i += k`. `transfer j k` (`j ≠ i`): precondition `rights i ≥ k`; `R i j += k`. `restock k`: no precondition; `R i i += k` (Balegas `R[i][i]`) | `R` and `U` by pointwise `natMax` | `rights i = split i + R i i + Σ_{j ≠ i} R j i - Σ_{j ≠ i} R i j - U i` (compared in ℕ, no subtraction; the diagonal counts once, O14); `stock = s0 + Σ_i R i i - Σ_i U i`; `nonneg`, `stock` as `Counter` |

Single writer: replica `i` writes only its own row of `m` and its own
entry of `ctx` in `MVReg`, its own entry of a decrement vector, and its own row of `R`. So
pointwise `natMax` is a join and it loses no write. `restock` writes only
the entry of `i` too. An `Ops` script (S3b) and a guest `Replica` (S4) use
these parts and add no part.

### 4.3 Judgements

- J1 well-formed declaration: each state part is a library part; each
  operation argument has a finite type; the declared initial split of an
  `Escrow` sums to its initial stock.
- J2 operation: the effect is a composition of part operations at the
  running replica; the precondition is the conjunction of the part
  preconditions and an optional declared `requires` predicate.
- J3 merge laws: L1 to L4 hold for each part (library proofs) and for the
  product (one library lemma). A program does not prove them again.
- J4 class of an operation pair `(o1, o2)`, where `o1` runs at replica A
  and `o2` runs at replica B on a common ancestor:
  - explicit conflict: both write the same `MVReg` part. The row names the
    part and the read `conflict {x, y}` for `x ≠ y`. This is a static
    test.
  - needs coordination: the bounded search (J5) finds a schedule whose
    branches contain `o1` and `o2` and whose merged state breaks an
    invariant `I`, and no `allocate` rule covers `I`. The check refuses
    with `REFUSE_NEEDS_COORDINATION` and the minimal counterexample.
  - independent: neither of the two cases above. The mark is proved when
    a library law or a user proof term covers the pair, else bounded.
- J5 bounded I-confluence search (Bailis et al.: an invariant is
  I-confluent under a set of operations when any two I-valid states that
  are reachable from a common ancestor merge to an I-valid state). The
  search starts at the initial state, applies a synced prefix of at most
  `P` operations, then gives each replica at most `M` operations, then
  merges. Each argument ranges over its finite type. The first violation
  in the order of 5.2 is the minimal counterexample.
- J6 allocation rule: `allocate c by escrow (A ↦ a, B ↦ b)` changes the
  part `c` from `Counter` to `Escrow`. It covers only the invariant
  `nonneg c`. The library law L7 proves `nonneg c` in each reachable
  state, so the mark is proved. Any other invariant named by the rule
  gets `REFUSE_RULE_MISMATCH`.

### 4.4 The three examples

- Tags (D3): `tag t := add tags t`. The pair `tag ∥ tag` is independent:
  the join of two sets keeps both new members (L1 to L3 of `GSet`). No
  invariant.
- Task owner (D4): `assign u := write owner u`. The pair `assign ∥
  assign` is an explicit conflict. After the merge, `read owner` gives
  `conflict {ana, ben}`. A later `assign` at a replica that has the
  conflict clears both values, so it resolves the conflict. A program that
  uses `read owner` where it needs a user gets a type error with the fix
  "fold over `Read`: `readEmpty`, `readOne u`, `readConflict s`".
- Inventory (D5): `sell k := dec stock k` with `invariant nonneg stock`.
  The pair `sell ∥ sell` needs coordination. Counterexample: initial
  stock 1; A sells 1 (local stock 1, precondition holds); B sells 1 (local
  stock 1, precondition holds); merge: stock 1 - 2 = -1; `nonneg stock`
  is false. With `allocate stock by escrow (A ↦ 1, B ↦ 0)`, B's `sell 1`
  fails locally (rights 0 < 1). The coordination it needs is `transfer B
  1` at A. Every schedule keeps `nonneg stock` (bounded), and L7 proves
  it (proved). `restock ∥ sell` and `restock ∥ restock` are independent
  for `nonneg stock`: a restock only adds stock (Q12 (b)).

Surface sketch (Q7 fixes the surface; S3 fixes the grammar):

```text
replicas A B
atoms Amount := 1 ∣ 2

replicated Shop
  state stock : Counter (init 1)
  op sell (k : Amount) := dec stock k
  invariant nonneg stock
  -- examples/inventory-escrow.merge adds:
  -- allocate stock by escrow (A ↦ 1, B ↦ 0)
```

### 4.5 Refusal codes (`langc check --json`)

Each code has a fix hint. S1 ports the kernel codes (`REFUSE_CELL`,
`REFUSE_DATA`, the parse and type codes, `W_NONCANON`).

| Code | When | Fix hint |
|---|---|---|
| `REFUSE_NO_MERGE` | a state part is not a library part | "use GSet, MVReg, Counter or Escrow" |
| `REFUSE_OP_INFINITE_ARG` | an operation argument has an infinite type | "declare the argument as atoms or Fin n" |
| `REFUSE_RAW_WRITE` | an effect is not a composition of part operations | "write the part through its operations" |
| `REFUSE_NEEDS_COORDINATION` | J4: a pair breaks an invariant after merge | "declare an allocation rule, for example `allocate stock by escrow (...)`, or remove the invariant" |
| `REFUSE_RULE_MISMATCH` | J6: the rule names an invariant that L7 does not prove | "an escrow rule covers `nonneg c` only" |
| `REFUSE_ALLOC_SUM` | J1: the initial split does not sum to the initial stock | "make the split sum to the stock" |
| `REFUSE_READ_WINNER` | an `MVReg` read is used where a value is needed | "fold over `Read`" |
| `W_BOUNDED` | a law or a pair has the mark bounded | "add a proof term to get proved" |

### 4.6 Counterexample format

A counterexample has these fields, in this order: the bounds (`P`, `M`,
replicas), the ancestor (the prefix operations and the state), the
operations of each replica with the local state after each one, the
merged state, the broken invariant and the values in it, and the class
row with its code and fix. It is minimal in the number of operations,
then first in the order of 5.2. The JSON form (5.3) and the transcript
(5.2) carry the same fields.

## 5. Targets

### 5.1 Wasm merge engine (`langc build`)

The writer facts (`docs/host/README.md`, `src/wasm.c` head, kit 5265a2a):
Wasm 1.0 (MVP); one memory that grows to at most 16384 pages (1 GiB) and
traps past it; an internal bump allocator over global 0; each IR entry is
exported by its name; each parameter and the result is `i64`; each entry
resets the heap pointer to 0; no imports.

Engine ABI (Q4 (a)):

- exports `memory`, `alloc(bytes) -> ptr` and `reset()`;
- `apply(state, replica, op, arg) -> ptr`: a new state, or 0 when the
  local precondition fails;
- `merge(a, b) -> ptr`;
- `check(state) -> i64`: bit `k` is set when invariant `k` is false;
- `encode(state) -> ptr` and `decode(ptr) -> ptr`: the canonical layout is
  the words of the parts in declaration order (vectors in index order);
  `decode` gives 0 for a layout that is not canonical;
- no imports, no floats, no clock, no randomness;
- the `stock` read (ℤ, S2b) is a signed `i64`; an `Ops` op body (S3b) is
  one `apply` call.

The writer changes: an engine entry does not reset the heap pointer, and
the module exports `memory`, `alloc` and `reset`. The kit entries keep
their old behavior. The engine code comes from one IR template per part
and from the invariants lowered through the kit IR (Q21 (a)).

Differential gate: for each example, the simulator schedules (5.2) run on
`langc eval` and on the module under `node`. The encoded states and the
`check` bits must be equal at each step. `wasm-validate` checks each
module. A wasmtime leg is O3.

### 5.2 Replica simulator (`langc sim`)

`langc sim FILE [--replicas N] [--prefix P] [--ops M] [--json]`. Defaults
(Q17): the declared replicas, `P = 1`, `M = 2`.

- Step function: the guest evaluator that `langc eval` uses, in process.
  There is no third implementation of the parts.
- Schedule shape: a synced prefix, then a partition (each replica runs its
  own sequence), then a reconnect (merge in both directions). This is the
  I-confluence shape of J5. An `Ops` op body (S3b) is one step. The
  transcript prints `stock` as ℤ (S2b).
- Order: by the total number of operations, then by replica, operation
  index and argument index, in declaration order. No seed, no clock, no
  host randomness, no floats.
- Reduction: operations at different replicas commute in a schedule until
  a sync, so the simulator enumerates one sequence per replica and one
  sync pattern. It does not enumerate interleavings of local operations.
- Output: "invariant kept on all K schedules (bounded: P, M, N)" or the
  minimal counterexample (4.6) and the coordination it needs.
- Determinism gate: two runs give byte-identical output; the golden files
  `test/sim/*.out` hold the expected bytes.

Transcript sketch for the demo (S5 fixes the bytes):

```text
merge-lang sim examples/inventory.merge
replicas A B; bounds prefix 1, ops 2; schedules K
ancestor: stock 1 (init 1)
A: sell 1   stock 1 -> 0   requires stock ≥ 1: held
B: sell 1   stock 1 -> 0   requires stock ≥ 1: held
reconnect: merge A B
merged: stock -1 (init 1, decrements A 1, B 1)
REFUTED invariant nonneg stock: stock = -1
sell ∥ sell: needs coordination (REFUSE_NEEDS_COORDINATION)
fix: allocate stock by escrow (A ↦ 1, B ↦ 0)
```

### 5.3 JSON coordination report

`langc build FILE` writes `NAME.wasm` and `NAME.report.json`. `langc check
--json` prints the same schema with the diagnostics (Q20 (a)). Keys, in
this fixed order:

- `version` (1), `file`, `replicas`, `bounds` (`prefix`, `ops`);
- `progress`: `parsed`, `typed`, `laws` (`total`, `proved`, `bounded`,
  `refuted`), `pairs` (`total`, `classified`);
- `pairs`: one row per unordered operation pair: `a`, `b`, `class`
  (`independent`, `explicit-conflict`, `needs-coordination`), `part` (for
  a conflict), `invariant`, `rule` (or null), `mark` (`proved`,
  `bounded`, `refuted`), `code` (or null), `fix` (or null),
  `counterexample` (4.6, or null);
- `diagnostics`: `code`, `span`, `expected`, `found`, `fix`, `retry`.

The writer is a port of `hosts/tcc-json/src/json.{c,h}` (S6). Golden
reports are compared byte for byte.

## 6. Laws (guest terms)

### 6.1 Laws

| Law | Statement | Mark | Slice |
|---|---|---|---|
| L1 | `merge a b ≡ merge b a` for each part and the product | proved (library) | S2 |
| L2 | `merge (merge a b) c ≡ merge a (merge b c)` | proved (library) | S2 |
| L3 | `merge a a ≡ a` | proved (library) | S2 |
| L4 | `apply i o s ≡ some t → le s t` (inflation) | proved (library, per part operation; per `Ops` script by composition, S3b) | S2 |
| L5 | convergence: the state of a replica is `mergeAll` of the `FMSet` of states it merged (S2d, Q29 (a)), so two replicas that merged the same multiset are equal (from L1 to L3) | proved (library); the simulator also checks each delivery order (bounded) | S4 |
| L5r | guest replicas: two `Replica S` values (S4) that got the same multiset of `recv` events after their last `local` event observe equal `eps` | proved when its guest term type-checks; else bounded (the simulator, each delivery order); a bounded run never gives proved | S4 |
| L6a | `GSet`: `has (merge (add x a) (add y b)) x` and the same for `y` (D3) | proved (library) | S4 |
| L6b | `MVReg`: concurrent `write x` and `write y` with `x ≠ y` read as `conflict {x, y}` (D4) | proved (library) | S4 |
| L7 | `Escrow`: in each reachable state, `rights i ≥ 0` for each `i` and the sum of the rights is `stock`, so `nonneg` holds (D5); with `restock` as `R i i` (O14) | proved (library) | S4 |
| L8 | a program invariant under an independent pair | proved with a user term, else bounded | S4 |

Why L7 holds: replica `i` writes only its row of `R` and `U i`, and it
spends only rights it holds. A merge can only add incoming rights. So the
rights of `i` in a merged state are at least its rights in its own last
state, and they are never below zero. The transfers cancel in the sum.

### 6.2 Outside the denotation

- Network delivery: the model assumes each sent state arrives some time.
  It does not model loss, duplication beyond idempotence, or order beyond
  the schedules of 5.2.
- Durability: a replica that loses its state and restarts from an old one
  can spend rights twice. L7 does not cover it.
- Byzantine replicas: a replica that forges a counter, a slot or a row of
  `R` breaks L1 to L7.
- Clocks: there are none; clock skew is not a fact of the model.
- Size: counters grow without bound in ℕ; the engine traps at `i64`
  overflow (O7).
- The bounded mark: "kept on all K schedules" is a fact about K schedules,
  not a proof for all schedules.

## 7. Agent adoption

Each bullet of the pasted list (`design/DESIGN.md` section 1), with its
deliverable, slice and gate. Anything beyond the six bullets is marked
EXTRA.

| Bullet | Deliverable | Slice | Gate |
|---|---|---|---|
| 1. One canonical form per operation and a `langc fmt` canonical printer | the Agda-like surface (Q7) with one spelling per form; `langc fmt`; `W_NONCANON` with a fix for non-canonical input; the `replicated` forms in S3 | S1, S3 | `fmt` is idempotent and round-trips on each example and on `test/fmt/`; each example is canonical |
| 2. `langc check --json` with fix hints and progress counts (parsed, typed, laws discharged) | the 5.3 schema; `progress` with `parsed`, `typed`, `laws` (n of m, split proved, bounded, refuted) and `pairs`; `docs/DIAGNOSTICS.md` | S1, S4, S6 | each refusal code has a fixture that checks its JSON shape; counts in each result |
| 3. `SKILL.md` with 3 to 5 graded examples | `skills/merge-lang/SKILL.md` with 5 examples from easy to hard: tags (independent); task owner (explicit conflict); inventory (refused, counterexample); inventory with escrow (kept, proved); seats (3 replicas, escrow with `transfer`, a tag set; the expected output names the transfer that a refused sale needs). Each has its expected `check --json` output | S8 | each example gives the output in the file |
| 4. An MCP server written in TinyCC (`langc mcp`) | a stdio JSON-RPC MCP server in C that tcc builds; tools `check`, `fmt`, `sim`, `report`, `explain-code` | S8 | an MCP stdio test drives `check` and `sim` on each graded example |
| 5. `AGENTS.md` and a README table saying when to use the language | `AGENTS.md` routing rule; README "When to use merge-lang" table: use for offline-first replicated state with domain invariants; not for text co-editing, server-only databases or Redis scripts | S0, S8 | the table matches the admitted parts |
| 6. A token-count comparison against Python and Rust | `docs/TOKEN-BASELINE.md`: the inventory demo (state, `sell`, invariant, escrow rule, the merge laws) in merge-lang, Python and Rust; the tokenizer is named (Q19); a script gives the counts; it also measures the cost of the Unicode surface | S9 | the script output equals the counts in the file |
| EXTRA | the Wasm engine lets an agent with no C toolchain run `apply`, `merge` and `check` | S7 | differential gate |

## 8. Port sources (pinned 2026-10-27)

| Repo | SHA | What |
|---|---|---|
| lang-template | 5265a2a | `hosts/tcc-wasm` (the seed); `hosts/tcc-json/src/json.{c,h}` (the report writer, S6) |
| evolve-lang | 1f87a5a | S1: the Agda-like surface, `langc fmt`, `W_NONCANON`, `check --json`, `docs/DIAGNOSTICS.md`, and the kernel of its S2a to S2d (`I`, `PathP`, `Path`, `transp`, `hcomp`, systems, indexed families, higher constructors, `REFUSE_CELL` for CM and CC, `Option`, `Fin`, the value printer); the same tcc-wasm 5265a2a seed; PLAN shape |
| leaf-lang | f5e587d | the source of the evolve-lang surface; PLAN 2.1 (read only) |
| quorum-lang | 09f7f59 | PLAN 2: `FMSet` in generator form (admitted, Q14 (b)), the refused cons form, `Maybe`, `Trunc` (read only) |
| intent-lang | 67445ae | `REFUSE_CELL`, cofamily (in evolve-lang 1f87a5a already) |
| resolve-lang | 632d819 | `REFUSE_CELL` (second implementation, read only) |
| mandate-lang | 94de313 | PLAN 4.6 free cell monad of operations (admitted, Q15 (b)); PLAN 4.7 `Machine` (the `Replica` cofamily, Q16 (b)) |
| zkpolicy-lang | 1d263fb | `Glue`, `ua` (not ported, O5) |
| agent-lang | 96e5473 | diagnostic fix text |
| dil-lang | 9e57528 | diagnostic fix text; README head (distinct space) |
| tether | 1f84803 | README head (distinct space) |
| automerge-cat | 61aa688 | README head: merge as the colimit in the posetal category of states |

No *-lang repository has an MCP server in C yet (O1).

## 9. Slices

One session each. Each slice: scope, gate (`make check`, EXIT 0), mutants
on the new checks, docs, close (stage, commit message, next prompt).

| Slice | Scope | Gate and mutants |
|---|---|---|
| S0 | kit seed, `design/DESIGN.md`, this plan, README, `AGENTS.md`, `docs/COMPARISON.md` | `make check` EXIT 0: examples 10 checked, 0 failed; parse refusals 9; check refusals 19; evals 32; ir 10 lowered; wasm 13 modules valid; differential 813 cases, 0 mismatches; gate 0 failures; review and domain regressions passed |
| S1 | one port of evolve-lang 1f87a5a (section 8): surface (Q7), `langc fmt`, `W_NONCANON`, `check --json` with `parsed` and `typed` counts, the S2a to S2d kernel; the kit sample domain stays (O4) | `make check` EXIT 0 with the evolve-lang 1f87a5a counts (its PLAN 9 S2d row: examples 10; parse refusals 10; check refusals 54; evals 62; ir 10; wasm 13; differential 773 cases, 0 mismatches; fmt 101 canonical, 6 non-canonical pairs; json 26 fixtures, diagnostics 38 codes; cubical 36 checks, 22 round trips), re-measured at S1 STEP 0 on evolve-lang 1f87a5a; mutants: the `REFUSE_CELL` CM test, the `Fin` bound, the `W_NONCANON` test. DONE 2026-10-27: `make check` EXIT 0 with these counts (also: diagnostics 24 codes with a fixture, 14 with no fixture; gate 0 failures; review, domain and diagnostic edit regressions passed); the three mutants KILLED. Review adds seven C classification cases (43 checks total); the upstream counts above remain the port baseline |
| S1b | kernel (Q26 to Q28 (a), O15): the builtin `natMax` with its rules on open arguments (`natMax a 0 = a`, `natMax 0 b = b`, `natMax (a+1) (b+1) = natMax a b + 1`) and a `fold` over ℕ with a motive (port of zkml-lang af57c42); a dependent `elim` for Bool (port of quorum-lang 351cbf4 rule_elim and motive_check) and for `Option`; the builtin `sumFin` (as auction-lang sumRat: it unrolls at a numeral `n`, it is stuck at an open `n`) | one eval test per new op and per new rule; one accepted fixture per eliminator with a `Path` motive (`natMax` comm by induction, `or` comm by cases); a refused fixture for a bad motive; `natMax n n` with an open `n` stays stuck; mutants: drop the `natMax (a+1) (b+1)` rule, the motive check, the `sumFin` bound. DONE 2026-10-27: `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 10; check refusals 59; evals 81; ir 13 lowered, and `test/ir/sumfin-open.lang` gets REFUSE_RUNTIME_PATH; wasm 16 modules valid; differential 1233 cases, 0 mismatches; fmt 110 canonical, 6 non-canonical pairs; json 26 fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture); gate 0 failures; review, domain and diagnostic edit regressions passed; cubical 43 checks, 22 round trips. The three mutants KILLED; the gate kills the motive check mutant by the message text that `test/check/expect.txt` pins for `nat-motive-sort.lang` |
| S2 | first the O12 replan (Q12 to Q16 (b)), DONE 2026-10-27 (Q22 to Q25 (a)); then domain on the S1b kernel: `GSet`, `MVReg`, `Read`, `Counter` and `Escrow` with `restock` (Q12 (b)), the product join; `nonneg` compares in ℕ; L1 to L4 as library guest terms; replace the kit sample domain (O4) | one eval test per part operation (`restock` too) and merge; L1 to L4 type-check; mutants on each merge (swap `natMax` for `natAdd`, drop the context test of `MVReg`) DONE 2026-10-27: `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 10; check refusals 59; evals 115; ir 13 lowered; wasm 16 modules valid; differential 1230 cases, 0 mismatches; fmt 113 canonical, 6 non-canonical pairs; json 26 fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture); gate 0 failures; review, domain and diagnostic edit regressions passed; cubical 43 checks, 22 round trips. O4 DONE, O11 TRIGGERED (the dotted lex form of `MVReg`). The 7 mutants KILLED: `natAdd` in `vecMax`, in `matMax`; `and` in `gsetMerge`; the `MVReg` clear set to 0; `mvregLive` without `natLe 1 (ctx j)`; `gsetRead` threshold 3; `pairMerge` with `ma` twice. Review rejects escrow self transfers and adds three regression evals |
| S2b | ℤ (Q13 (b), Q22 (a)), no kernel work: the lexer token `ℤ`; base data `ℤ` (`pos`, `negsuc`) in `domain/domain.lang`; `natDiff`, `intLe`; the `stock` read of `Counter` and `Escrow` | one eval test per ℤ operation; `stock` below zero prints a negative value; `LEX_CHAR` still refuses another non-ASCII character; mutant: swap the sign in `natDiff` DONE 2026-10-27: `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 11 (`test/parse/bad-unicode.lang` gets `LEX_CHAR` for `ℚ`); check refusals 59; evals 133 (18 ℤ evals in `test/eval/int.lang`); ir 13 lowered; wasm 16 modules valid; differential 1230 cases, 0 mismatches; fmt 114 canonical, 6 non-canonical pairs; json 26 fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture); gate 0 failures; review, domain and diagnostic edit regressions passed; cubical 43 checks, 22 round trips. `negsuc k` prints as `-(k+1)`. The 3 mutants KILLED: `natDiff` with `pos` and `negsuc` swapped; `intLe` at `negsuc`/`negsuc` with `natLe m n`; the print rule without `+ 1` |
| S2c | kernel: path constructors of dimension 2 (port of quorum-lang 09f7f59 S3, its O37 to O56; O13); `FMSet` in generator form (Q14 (b)) as program data in `test/eval/fmset.lang` (Q30 (b)); the Q29 (a) scope: the `fold` over a higher inductive type, a bottom and a left unit law for each part in `domain/domain.lang`, and `fmsetMap`, a `fold` from `FMSet` into `FMSet`; `mergeAll` moves to S2d | `test/check/cell-path-dim.lang` moves to accepted at dimension 2 (`test/eval/cell-path-dim.lang`), a new dimension 3 fixture gets `REFUSE_CELL`; the cons form still gets `REFUSE_CELL`; mutants: the dimension bound, the `fold` at a 1-cell DONE 2026-10-28: `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 11; check refusals 59; evals 146 (+13: 1 in `test/eval/cell-path-dim.lang`, 5 in `test/eval/hit.lang`, 7 in `test/eval/fmset.lang`); ir 13 lowered; wasm 16 modules valid; differential 1230 cases, 0 mismatches; fmt 117 canonical, 6 non-canonical pairs; json 26 fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture); gate 0 failures; review, domain and diagnostic edit regressions passed; cubical 43 checks, 36 round trips. Mutants: M1 the bound `> 1u` KILLED (13 failures); M2 the bound `> 3u` KILLED (1 failure); M3a the wrong end in the `fold` at a 1-cell SURVIVED (an end at an interval variable does not show in a printed value); M3b the unfolded recursive field in the `fold` at a 1-cell KILLED (6 failures) |
| S2d | `isSet` for each part by a Hedberg port (it can need a multi-face `hcomp` and an empty type); `mergeAll : FMSet S → S` by `FMSet` recursion into the bottom and `merge` of each part, on the program side (Q29 (a); Q30 (b): `FMSet` is a cell signature in a program); a `fold` into a part over a type with a 2-cell needs `isSet` of the part | `isSet` type-checks for each part; one eval test of `mergeAll` per part; mutants: the `mcomm` case of `mergeAll` DONE 2026-10-28: `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 11; check refusals 59; evals 152 (5 `mergeAll` evals in `test/eval/mergeall.lang`, 1 in `test/eval/fold-open.lang`); ir 13 lowered; wasm 16 modules valid; differential 1230 cases, 0 mismatches; fmt 119 canonical, 6 non-canonical pairs; json 26 fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture); gate 0 failures; review, domain and diagnostic edit regressions passed; cubical 43 checks, 36 round trips. Q31 (a): one `eval.c` hunk, the ℕ `fold` steps at an open successor `natAdd y c`. The 4 mutants KILLED: the `mcomm` case of `mergeAll` as `cm h g` (`TYPE_MISMATCH`, 5 failures); the first counter `natAdd y 0` (`TYPE_ENDPOINT` at `fN`, 6 failures); each counter `y` (`TYPE_REFL` at `predTwoLaw`, 1 failure); one step short (`TYPE_MISMATCH` at `absurdZS`, 6 failures) |
| S3a | parse and `fmt` of the forms `replicas`, `atoms`, `replicated`, `state`, `op`, `invariant`, `allocate` (Q32 (a)): the lexer token `∣` and the form keywords; the form AST; the parser blocks (the evolve-lang `protocol` layout: a `replicated NAME` body has one item a line at one column); `fmt` of the 7 forms; a domain with numerals and names is a parse refusal (Q34 (a)); `check` of a form refuses with a kit code until S3e; `examples/tags.merge`, `owner.merge`, `inventory.merge`, `inventory-escrow.merge` parse and fmt (a gate loop for `examples/*.merge`, fmt only) | `fmt` round trip on the forms; one parse refusal per bad form shape; the four examples fmt canonical; mutants on the parse refusals and the fmt printers DONE 2026-10-28: `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 22 (11 form refusals, the text pinned); check refusals 60 (`test/check/form.lang`, `REFUSE_FORM`); evals 152; ir 13 lowered; wasm 16 modules valid; differential 1230 cases, 0 mismatches; fmt 127 canonical, 12 non-canonical pairs (6 `test/fmt/form-*` pairs); merge examples 4 canonical (the new gate loop); json 27 fixtures, diagnostics 39 codes (25 with a fixture, 14 with no fixture; the new code `REFUSE_FORM`); gate 0 failures; review, domain and diagnostic edit regressions passed; cubical 46 checks, 38 round trips. The 11 mutants KILLED by a gate test: the numeric member branch of `atoms` takes a name (`atoms-mix-nat`); the first-name branch messages swapped (`atoms-mix-name`); the `replicated` item loop takes a column >= the item column (`replicated-column`); the `at_init` test of `state` always true (`state-init-name`, 2 form pairs); the `↦` of `allocate` optional (`share-no-mapsto`); `fmt` drops the `init` print (the form pairs); the share separator `, ` as a space (`form-replicated`, `inventory-escrow.merge`); the `∣` separator as a space (`form-atoms-*`); the `REFUSE_FORM` test drops `atoms`, `replicated` or `replicas` (3 mutants: the form pair branch, `form.lang`) |
| S3e | the 7 forms elaborated to guest terms (Q9) on the S2 domain parts and the S2d `mergeAll`, by the evolve-lang route (Q33 (a): synthetic `DECL_DEF` nodes, a type error points at the form word); a numeric atom domain also gives `NAMEValue : NAME → ℕ` (Q34 (a)); an `op` body is one part operation, no `requires` (Q35 (a)); `REFUSE_NO_MERGE`, `REFUSE_OP_INFINITE_ARG`, `REFUSE_RAW_WRITE`, `REFUSE_ALLOC_SUM` with their docs/DIAGNOSTICS.md rows | one fixture per code; mutants on each J1 and J2 test DONE 2026-10-29: `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 22; check refusals 70 (10 new `test/check` fixtures, the text pinned); evals 173 (`test/eval/replicas.lang`, `shop.lang`, `escrow.lang`, 7 rows each); ir 13 lowered; wasm 16 modules valid; differential 1230 cases, 0 mismatches; fmt 143 canonical, 12 non-canonical pairs; merge examples 4 canonical; json 30 fixtures, diagnostics 42 codes (28 with a fixture, 14 with no fixture; `REFUSE_FORM` and its fixture removed, the 4 new codes each with a fixture `test/json/CODE.lang`); gate 0 failures; review, domain and diagnostic edit regressions passed; cubical 46 checks, 38 round trips. The four examples pass `langc check` (`tags.merge` and `owner.merge` name an atoms domain, Q36 (a)); the gate loop is still fmt only (S3x). 14/14 mutants KILLED (4 after new fixtures): the J1 `REFUSE_NO_MERGE` head test (`REFUSE_NO_MERGE.lang`); the `REFUSE_OP_INFINITE_ARG` call (json, the `form-replicated` pair), its `Fin` numeral test (new `op-fin-open`) and its `Replica` test (eval escrow); the parser Field span (the json column); the `REFUSE_ALLOC_SUM` `over` test (`alloc-sum`), the sum under the init (new `alloc-under`) and the share count (new `alloc-missing`); the J2 `REFUSE_RAW_WRITE` word test and part kind test (`raw-write`, eval shop) and arity test (new `op-arity`); `cg_value` names and `natEq` (eval replicas); `op_amount` (eval shop) |
| S3x | the four examples parse and type (`check` GREEN; the `examples/*.merge` gate loop runs `check` too); the docs; the S3 close | the four examples GREEN DONE 2026-10-29 (on the USER S3e commit 2b4edf4): `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 22; check refusals 70; evals 173; ir 13 lowered; wasm 16 modules valid; differential 1230 cases, 0 mismatches; fmt 143 canonical, 12 non-canonical pairs; merge examples 4 canonical, 4 checked (the line text is new: the loop runs `langc check` on each file and fails on an exit that is not 0, an output that is not `ok`, or a stderr line); json 30 fixtures, diagnostics 42 codes (28 with a fixture, 14 with no fixture); cubical 63 checks, 38 round trips. 4/4 example mutants KILLED by the loop only (each file stays canonical, so the `fmt` part passes): `write` on the GSet of `tags.merge` and `add` on the MVReg of `owner.merge` (`REFUSE_RAW_WRITE`), an `Escrow` part with an init in `inventory.merge` (`REFUSE_NO_MERGE`), the shares `(A ↦ 1, B ↦ 1)` in `inventory-escrow.merge` (`REFUSE_ALLOC_SUM`). No `check.c` path is reached by the examples only: the loop also kills `counterRestock` to `escrowRestock` and the `over` test `>` to `>=`, but `test/eval/shop.lang`, `escrow.lang` and `alloc-twice` kill them too. The swap of `n` and `r` in `mvregWrite` survives the gate: each MVReg test has 2 atoms and 2 replicas |
| S3b | kernel: the CM test admits a field `E → T A` with `E` closed data (the mandate-lang 4.6 R1 rule, its domain.lang `IO`); library `Ops A`; an `op` body can be an `Ops` script of part reads and part operations, one step at one replica (Q15 (b), Q24 (a)); L4 for scripts by composition | a fixture for the new field rule and a refused fixture where `E` mentions `A`; an eval test of a script that reads, then writes; mutants on the closed test of `E` and on the script step order. DONE 2026-10-29 (on the USER S3x commit a6d7a56; Q39 (a) to Q44 (a)): `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 22; check refusals 75 (5 new `test/check` fixtures: `cell-arrow-self`, `cell-arrow-two`, `read-counter`, `stock-mvreg`, `le-order`; the `REFUSE_CELL` text adds "or a function from closed data to it"); evals 200 (`test/eval/ops-lib.lang`, `ops.lang`, `owner3.lang`, `script.lang`, `script-dec.lang`, `le.lang`); ir 13 lowered; wasm 16 modules valid; differential 1230 cases, 0 mismatches; fmt 155 canonical, 13 non-canonical pairs (`form-op-steps`); merge examples 4 canonical, 4 checked; json 30 fixtures, diagnostics 42 codes (28 with a fixture, 14 with no fixture); gate 0 failures; review, domain and diagnostic edit regressions passed; cubical 63 checks, 38 round trips. U1 the arrow field rule and the catamorphism `fold` at the field (`cell-field.lang` refuses `E = A`); U2 `Ops A` and the runner `NAMERun` (Q44 (a): `owner3.lang` closes the S3x `n` and `r` swap gap); U3 the script syntax `x ← read PART; ...`; U4 `PLe` and `PLeTrans` for each part, the L4 of three scripts of two steps in `le.lang` (Q43 (b), a `NAMEOPL4` term for each script op, is not done). 10/10 mutants KILLED by a test: M1 the closed test of `E` (`cell-field`); M2 `self_uses == 1u` to `>= 1u` (`cell-arrow-two`); M3b `fold_arrow_field` returns `k` (eval `ops`); M4 the telescope; M5 `family_class`; M6 the script step order (eval `script claimBot`); M7 the read part index (`stock-mvreg`, eval `shop tagged`); M8b the `run_cast` false branch (`raw-write`); M9a the total `kc` (eval `script bothBen`); M9b the partial `kc` (eval `script-dec soldTagged`) |
| S4 | the checker: J4 to J6, the bounded search core in C (shared with S5), `REFUSE_NEEDS_COORDINATION`, `REFUSE_RULE_MISMATCH`, `REFUSE_READ_WINNER`, `W_BOUNDED`; L5, L5r, L6a, L6b, L7, L8; the `Replica S` cofamily with `Event = local Op ∣ recv S` (Q16 (b), Q25 (a)); `laws` and `pairs` counts in `check --json` | one test per code with its JSON shape; `inventory.merge` refused, `inventory-escrow.merge` GREEN with L7 proved; L5r proved or bounded, never proved by a bounded run; mutants on each class test and on L7 |
| S5 | `langc sim`: schedules, order, reduction, transcript; golden `test/sim/*.out` (the demo); determinism gate | the demo transcript equals its golden file; two runs byte-identical; mutants on the order and on the minimality test |
| S6 | JSON report: port of `json.c`, `NAME.report.json`, the 5.3 schema shared with `check --json` | golden reports; mutants on the class map |
| S7 | Wasm engine: the writer changes of 5.1, part templates, lowered invariants, the engine ABI | `wasm-validate`; differential gate on every simulator schedule; mutants on one template per part |
| S8 | agent kit: `SKILL.md` with 5 graded examples, `langc mcp`, `explain-code`, final `AGENTS.md` and README table | MCP stdio test; each example passes `check` with the output in the file |
| S9 | `docs/TOKEN-BASELINE.md` and its script | the script output equals the file |
| S10 | close: `docs/STATUS.md`, `docs/VALIDATION.md`, full gate, mutant summary | full gate EXIT 0 |

## 10. Open items

- O1 the MCP server has no C precedent in the *-lang repositories. S8
  pins the MCP protocol revision it implements.
- O2 the 2024 HN thread of the description was not found by two web
  searches (2026-10-27). Docs cite it only as "an HN discussion (2024)".
- O3 a wasmtime leg for the differential gate (wasmtime is installed,
  2026-10-27). The gate runner stays `node` (the kit choice).
- O4 DONE 2026-10-27 (S2 unit 4 step 6). The kit sample domain in
  `domain/domain.lang` is replaced: `Verdict`, `Log`, `Hist`, `Read n` and
  `Step`, then the parts, their laws and the product join. 14 fixtures
  moved to these names. The examples still use `.lang`; R5 names
  `examples/inventory.merge` for the merge-lang surface.
- O5 `Glue` and `ua` are ambient by the R1 reading, but no merge-lang law
  needs univalence. They are not ported. zkpolicy-lang 1d263fb is the
  source if a later law needs them.
- O6 a user merge function with proof terms for L1 to L4 (3.3). Not in
  v1; on a USER ask only.
- O7 counter growth: counters and version counters grow without bound.
  The engine traps on `i64` overflow; the evaluator uses big naturals.
  The differential gate stays inside `i64`.
- O8 the S1 port carries the open items of evolve-lang at 1f87a5a (its
  PLAN O10 to O18: the ASCII alias policy, `let`, dead core paths and
  others). S1 lists the ones that still apply.
- O9 the Automerge facts (`design/DESIGN.md` section 3) are from the
  v3.1.1 docs on 2026-10-27. Check them again before a public README.
- O10 the bounded search is a refuter, not a decision procedure. A pair
  row with no counterexample in the bounds has the mark bounded. A
  complete I-confluence test (Whittaker and Hellerstein) is out of v1.
- O11 TRIGGERED 2026-10-27 (S2 unit 3). The slot form
  `Fin r → Option (Fin n × ℕ)` has no commutative merge at a tie: two
  entries with the same counter and different values. Single writer
  does not reach that state, but L1 is stated for all states. A pick by
  `Fin` order needs equal witnesses to give equal `Fin` terms (UIP on
  `Bool` paths), so the L1 proof fails as stated. S2 takes the pre-ruled
  fallback: the dotted lex form of 4.2, `MVReg n r =
  (Fin r → Fin n → ℕ) × (Fin r → ℕ)`, merged by pointwise `natMax`. The
  full dotted set merge is also a semilattice, but L4 of `write` fails
  at a state with a dot that its context does not cover; S2 does not
  use it.
- O12 DONE 2026-10-27 (S2 unit 1). The USER ruled Q12 to Q16 (b):
  `restock`, ℤ, `FMSet`, `Ops A` and a `Replica` cofamily are admitted.
  The replan changed sections 1 to 6, 9 and 10, and the USER ruled Q22
  to Q25 (a): `restock` in S2, ℤ in S2b, `FMSet` in S2c, `Ops A` in S3b,
  `Replica` in S4.
- O13 the S2c port of dimension 2 path constructors (quorum-lang S3)
  keeps its O52: a fold at the 2-cell `trunc` stays stuck. `mergeAll` on
  a closed multiset never meets `trunc`. DONE 2026-10-28 (S2c): the port
  is in, and the `sqFc` eval of `test/eval/hit.lang` prints the stuck
  fold at a 2-cell. `mergeAll` moves to S2d (Q29 (a)).
- O14 the S0 rights formula of `Escrow` counted `R i i` in both sums.
  With `restock` as `R i i` (Q12 (b)), the diagonal counts once (4.2).
  The L7 proof (S4) must cover it.

- O15 DONE 2026-10-27 (S1b). The S1 kernel had only eliminators with a
  fixed result type (`fold`, `flagIf`, `option`), and the builtins were
  stuck at open arguments. So no guest term proved L1 to L3 at open
  states, and no guest `sumFin` built `pack k refl` at an open `k`. S1b
  (Q26 to Q28 (a)) added induction, the `natMax` rules and the builtin
  `sumFin`. At an open `n`, `sumFin` stays stuck in the checker, and the
  lowering refuses it with REFUSE_RUNTIME_PATH.

## 11. Questions for the USER

Each item has options; (a) was the recommendation. Marks (2026-10-27):
RULED (letter) is a USER ruling.

- Q1 R1 reading. RULED (a). (a) the precedent reading of 2.1: ambient Pi, Sigma,
  Path, U are not limited; declared type constructors are only CM and
  CC; the cell test is syntactic; cons is refused; the dropped clause
  named the model, not a former. (b) the same, and also refuse higher
  inductive types (no path constructors in a CM).
- Q2 kit and port path. RULED (a). (a) tcc-wasm seed; S1 = one port of evolve-lang
  1f87a5a (surface and kernel S2a to S2d); `json.c` from tcc-json in S6.
  (b) port from leaf-lang, quorum-lang and intent-lang one by one, as
  evolve-lang did (four slices more).
- Q3 Wasm target. RULED (a). (a) an engine module per program, emitted by the Wasm
  writer of `langc`; `langc` itself is native, not Wasm. (b) build
  `langc` with clang for wasm32-wasi (this breaks R3).
- Q4 engine ABI. RULED (a). (a) the memory ABI of 5.1 (export `memory`, `alloc`,
  `reset`; state pointers; a writer change so that engine entries keep
  the heap). (b) pack each state in one `i64` and refuse larger states.
- Q5 merge model. RULED (a). (a) state based: a state is a join-semilattice, merge
  is the join, each operation is inflationary. (b) operation based, with
  causal delivery assumed.
- Q6 discharge marks. RULED (a). (a) proved, bounded, refuted; the report never says
  proved for bounded. (b) proofs only: refuse a law with no proof term.
- Q7 surface. RULED (a). (a) the Agda-like Unicode surface of quorum-lang (Q13),
  leaf-lang (Q14) and evolve-lang (Q7). (b) ASCII keywords. Unicode
  symbols can cost more tokens; S9 measures it.
- Q8 proofs. RULED (a). (a) guest language only. (b) also a Lean export.
- Q9 declaration forms. RULED (a). (a) dedicated forms `replicas`, `atoms`,
  `replicated`, `state`, `op`, `invariant`, `allocate` that elaborate to
  guest terms. (b) plain guest definitions over the domain types only.
- Q10 tags. RULED (a). (a) `GSet` with `add` only (D3 names additions). (b) the
  add-wins OR-set with dots, and `remove`.
- Q11 owner read type. RULED (a). (a) `Read n`, a base indexed family: `empty`,
  `one x`, `conflict s` with a proof that `s` has 2 or more members. (b)
  `Pfin⁺ A`, the free join-semilattice as a CM higher inductive type; it
  needs set truncation, a 2-cell, and the evolve-lang kernel has 1-cells
  only (its Q-S2b-5).
- Q12 inventory operations. RULED (b). (a) `sell` (a decrement) only, with a
  declared initial stock and initial split; the escrow rule is the
  rights and `transfer`. (b) also `restock` (increments, Balegas
  `R[i][i]`).
- Q13 integers. RULED (b). (a) ℕ only; `nonneg` compares in ℕ; the transcript prints
  the signed stock. (b) admit ℤ.
- Q14 `FMSet`. RULED (b). (a) not admitted; a set over an atom domain is a vector
  `Fin n → Bool`. (b) admit `FMSet` in generator form (quorum-lang).
- Q15 free CM of operations. RULED (b). (a) not admitted; the changes of a replica
  are a vector of operations. (b) admit `Ops A` (mandate-lang 4.6) for
  scripts whose later operations read earlier responses.
- Q16 replica as a cofamily. RULED (b). (a) not admitted; the simulator replica is
  a C value. (b) a `Replica` cofamily, a Moore machine (mandate-lang 4.7
  `Machine`), so that convergence is a statement about guest replicas.
- Q17 simulator bounds and order. RULED (a). (a) the 5.2 defaults (`P = 1`,
  `M = 2`, the declared replicas), order by total operations then
  declaration order, the reduction of 5.2. (b) seeded random schedules
  (this breaks byte determinism).
- Q18 demo files. RULED (a). (a) `examples/inventory.merge` (no rule: refused with
  the counterexample) and `examples/inventory-escrow.merge` (kept), both
  golden; the demo transcript is the first. (b) one file and a flag that
  adds the rule.
- Q19 tokenizer for the baseline. RULED (a). (a) a fixed offline proxy, as leaf-lang
  Q13 and evolve-lang Q12. (b) a named BPE tokenizer through a script.
- Q20 report entry. RULED (a). (a) `langc build` writes `NAME.report.json`; `langc
  check --json` prints the same schema with diagnostics. (b) a separate
  `langc report` command.
- Q21 engine code. RULED (a). (a) one IR template per part, plus the invariants
  lowered through the kit IR; the differential gate checks the module
  against `langc eval` of the guest terms. (b) lower the guest terms of
  the parts directly (the IR then needs finite vectors as arrays).
- Q22 ℤ (S2 O12 replan). RULED (a). (a) a new slice S2b after S2, no
  kernel work: the lexer token `ℤ`, base data `ℤ` (`pos`, `negsuc`),
  `natDiff`, `intLe`, the `stock` read in ℤ; S2 `nonneg` compares in ℕ.
  (b) ℤ inside S2.
- Q23 `FMSet` use. RULED (a). (a) slice S2c: port the dimension 2 path
  constructors of quorum-lang S3; `mergeAll : FMSet S → S`; L5 says the
  state of a replica is `mergeAll` of its merged multiset; `GSet` stays a
  vector (`FMSet` union is not idempotent). (b) S2c, and `FMSet` holds the
  operations of each replica in J5; L5 keeps the S0 statement.
- Q24 `Ops A` use. RULED (a). (a) slice S3b after S3: the CM test admits
  `E → T A` with `E` closed; an `op` body can be an `Ops` script, one step
  at one replica. (b) slice S4b: the replica sequence of J5 and the
  simulator is an `Ops` term; `op` bodies stay compositions.
- Q25 `Replica` use. RULED (a). (a) S4, no kernel work: `Event = local Op
  ∣ recv S`; L5r about guest replicas, proved only by a guest term, else
  bounded. (b) S5: the simulator step runs a guest `Replica`; L5 keeps
  the S0 statement.
- Q26 L1 to L3 proof route (S2 session 2). RULED (a). (a) port the ℕ
  motive fold and the `natMax` rules on open arguments (zkml-lang
  af57c42) and a dependent `elim` for Bool and `Option` (quorum-lang
  351cbf4); L1 to L3 are guest proofs, the mark stays proved. (b)
  trusted kernel rules (auction-lang `rule`). (c) `postulate` in
  domain.lang. (d) closed instances by refl only.
- Q27 kernel slice. RULED (a). (a) a new slice S1b before S2, with its
  own commit. (b) inside S2.
- Q28 `sumFin`. RULED (a). (a) a builtin that unrolls at a numeral `n`
  (as auction-lang sumRat); `r` is always a numeral in a program. (b) a
  guest term on the S1b fold, else (a). (c) parts over `ℕ → ℕ`.
- Q29 `isSet` for `mergeAll` (S2c Unit 0, 2026-10-28). RULED (a). The
  `trunc` case of a `fold` into a part needs `isSet` of the part; the
  domain has no `isSet`, no empty type and no `⊥`. (a) split: S2c does
  the kernel port (dimension 2, the `fold` over a higher inductive type),
  `FMSet` with `trunc`, a `⊥` and a left unit law for each part, and a
  `fold` from `FMSet` into `FMSet`; a new slice S2d does `isSet` for each
  part (a Hedberg port) and `mergeAll`. (b) all in S2c. (c) `FMSet`
  without `trunc`.
- Q30 the place of `FMSet` (S2c Unit 2, 2026-10-28). RULED (b). Each
  domain family is `FAMILY_BASE` (`family_class` in `check.c`), and R1
  refuses a path constructor in it (`DOMAIN_FAMILY`). (a) a domain
  exception for a higher inductive type in `family_class`. (b) `FMSet`
  and `fmsetMap` are program data (a cell signature) in
  `test/eval/fmset.lang`, with no kernel change; S2d `isSet` and
  `mergeAll` are also on the program side. (c) defer `FMSet`.
- Q31 `isSet ℕ` and the ℕ `fold` at an open successor (S2d Unit 0,
  2026-10-28). RULED (a). The ℕ `fold` reduces only at a numeral; a
  guest proof of `isSet ℕ` needs `fold M s z (natAdd y 1)` to step to
  `s y (fold M s z y)`. `Counter`, `Escrow` and `MVReg` need `isSet ℕ`.
  (a) kernel: `reduce_fold_nat` steps at a stuck `natAdd y c` with a
  numeral `c >= 1` (`c` steps, the step argument `natAdd y k`), as the
  S1b `natMax` open successor rule; then `isSet ℕ` is a guest proof. (b)
  no kernel change: `IsSet ℕ` is a hypothesis argument of the fixture.
  (c) defer `isSet` of the ℕ parts to a new slice.
- Q32 the S3 split (S3 Unit 0, 2026-10-28). RULED (a). S3 needs more
  than one session. The name S3b is taken (the `Ops A` slice, Q24 (a)),
  so the new letters skip b. (a) three slices, each with a USER commit:
  S3a = the lexer (`∣`, the form keywords), the form AST, the parser
  blocks (the evolve-lang `protocol` layout), `fmt` of the 7 forms with
  a round trip, the parse refusals for a bad form shape, and the four
  `examples/*.merge` parse and fmt (a gate loop for `.merge`; `check` of
  a form refuses with a kit code until S3e); S3e = the elaboration of
  the 7 forms to guest terms, the 4 codes with one fixture each, the
  DIAGNOSTICS rows, the J1 and J2 mutants; S3x = the four examples parse
  and type (`check` GREEN), the docs and the S3 close. (b) two slices:
  S3a as in (a); S3e = (a) S3e and S3x (two sessions or more, one
  commit). (c) one slice S3 over resume sessions with one USER commit at
  the end (as S2). The reason for (a): each part fits one session with
  the close, and each commit is a small review.
- Q33 the elaboration route (S3 Unit 0, 2026-10-28). RULED (a). (a) the
  evolve-lang route: a form stays a Decl node; `check.c` builds
  synthetic `DECL_DEF` nodes (`ORIGIN_PROGRAM`) and runs `check_def` on
  them; a type error points at the form word. (b) the restrict-lang and
  resolve-lang route: `check.c` prints the guest source text of the
  form, then lexes, parses and checks it; less C, but a type error in
  the generated text has a span in that text, not in the program. The
  reason for (a): a diagnostic must point at the program line (4.5 fix
  hints, `check --json` spans).
- Q34 numeric atoms (S3 Unit 0, 2026-10-28). RULED (a). The Surface
  sketch (4.4) has `atoms Amount := 1 ∣ 2` and `sell (k : Amount) :=
  dec stock k`, but `counterDec` takes `k : ℕ`. (a) an atom domain is
  `Fin n`; when each member is a numeral, the elaboration also writes
  `NAMEValue : NAME → ℕ`, and a part operation that takes an amount
  applies it; a domain with numerals and names is a parse refusal. (b)
  refuse numeral members; an amount argument is `Fin n` and its value is
  the index (`Fin 3` gives 0, 1, 2). The reason for (a): it keeps the
  sketch and the D5 counterexample (`sell 1`).
- Q35 the op body (S3 Unit 0, 2026-10-28). RULED (a). J2 (4.3) says "a
  composition of part operations" and an optional `requires` predicate;
  no slice row names the syntax. (a) S3 admits one part operation per
  `op` body and no `requires`; a sequence and `requires` come with the
  S3b `Ops` scripts (Q24 (a)); `REFUSE_RAW_WRITE` refuses a body that is
  not one part operation at the running replica. (b) S3 also admits `o1
  ; o2` (Option composition, the precondition is the conjunction) and a
  `requires P` line. The reason for (a): the four examples use one part
  operation each, and the S3b scripts give the sequence.
- Q36 the element domain of a `GSet` or `MVReg` part (S3e Unit 0,
  2026-10-28). RULED (a). The examples write `state tags : GSet` and `state
  owner : MVReg`, but the parts are `GSet n` and `MVReg n r` (4.2). (a)
  the `state` line names an atoms domain: `state tags : GSet Tag`,
  `state owner : MVReg User`; S3e changes tags.merge and owner.merge.
  (b) the elaboration takes the domain from the argument of the `op`
  bodies that write the part; a part with no such `op` is refused. The
  reason for (a): each line elaborates alone, a type error points at
  its own line, and a part with no `op` still has a type.
- Q37 the invariant body (S3e Unit 0, 2026-10-28). RULED (a). 4.4 has
  `invariant nonneg stock`; the domain has no `nonneg`. (a) S3e admits
  `nonneg PART` only, for a `Counter` or an allocated part; the domain
  gets `counterNonneg` and `escrowNonneg` (the 4.2 Read column), and a
  different body is refused (`REFUSE_RAW_WRITE` names the invariant).
  (b) an invariant is a guest `Bool` term with each part name bound to
  its part; `nonneg` is rewritten to the part function. The reason for
  (a): J6 covers `nonneg c` only, and the J5 search (S4) needs a known
  predicate.
- Q38 the names of the synthetic definitions (S3e Unit 0, 2026-10-28).
  RULED (a). (a) NAME + Word as Q34: `Replica`, the replica and atom names,
  `Shop` (the state type), `ShopBot`, `ShopMerge`, `ShopSell` (op `sell`),
  `ShopInv`, `ShopStockSplit`; a clash gives `REFUSE_NAME` at the form
  word. (b) names with a `.` (`Shop.sell`) that a program cannot write,
  as the cofamily ctor names. The reason for (a): S4 and a program (a
  test, an eval) use them by name.
- Q39 "closed" for the domain `E` of a cell monad field `E → T A` (S3b
  Unit 0, 2026-10-29). RULED (a). (a) the merge-lang
  `cell_closed(E, earlier)`: `E` names no parameter and no family, but
  can name an earlier field (`opRead (n : ℕ) (k : Read n → Ops A)`), as
  the closed data field rule. (b) the mandate-lang test
  `cell_closed(E, 0)`: no free variable at all; then `Read n` needs a
  numeral `n`, so a library `Ops` cannot read an `MVReg` of any size.
  The reason for (a): it is the merge-lang closed data rule, and the
  `Read n` response needs it.
- Q40 the fold at an arrow field (S3b Unit 0, 2026-10-29). RULED (a).
  (a) the catamorphism: the method gets `E → R` (`fold_family` gives
  `λ e → fold (k e)`, the method type is `E → res`); kernel work in
  eval, check and the IR lowering of `elim`. (b) the mandate-lang rule:
  the field as it is, with no induction hypothesis (no new code); a
  runner takes fuel, the step count. The reason for (a): a script
  runner is one `elim` and has no fuel argument.
- Q41 `Ops A` and the `op` route (S3b Unit 0, 2026-10-29). RULED (a).
  (a) library `Ops (A : Type 0)`: `opPure (opOf : A)`, the reads
  `opRead (readN : ℕ) (readPart : ℕ) (readNext : Read readN → Ops A)`
  and `opStock (stockPart : ℕ) (stockNext : ℤ → Ops A)`, and the writes
  `opAdd`, `opWrite`, `opDec`, `opRestock`, `opTransfer` (part index,
  arguments, `next : Ops A`). The elaboration gives the script value
  `NAMEOPScript` and one runner per form `NAMERun` that answers a read
  from the current state and applies a write (a `natEq` chain on the
  part index); with Q40 (a) the runner is one `elim` with no fuel. The
  op definition applies `NAMERun` to the replica, the script and the
  state. (b) a direct composition: a script elaborates to state passing,
  with no `Ops` value. (c) one family per form `NAMEOps` and `NAMERun`;
  no library `Ops`. The reason for (a): the script is a first class
  value, as the Q15 (b) text says.
- Q42 the script syntax (S3b Unit 0, 2026-10-29). RULED (a). (a) one
  line, steps joined by `;`, a read binds with `←` (and the ASCII
  `<-`): `op sell (t : Tag) := x ← read owner; add tags (readOr t x)`.
  The read words are `read PART` (a `GSet` or `MVReg` part, `Read n`)
  and `stock PART` (a `Counter` or an `Escrow` part, `ℤ`). A write
  argument is a guest term in the scope of the parameters and the
  earlier read variables (check_def types it). fmt prints `; ` between
  steps. (b) a step per line under the `op` line (layout); a bigger
  parser change. The reason for (a): a small lexer and parser change.
- Q43 L4 by composition (S3b Unit 0, 2026-10-29). RULED (a). (a)
  library: `le` per part as the Path `merge s t ≡ t`, a transitivity
  lemma per part (from associativity and the part L4), and a two step
  guest proof (a fixture: the L4 of one sample script of two writes)
  that type-checks; the claim for all scripts is induction on the step
  count, in the design text. (b) the elaboration also gives `NAMEOPL4`
  for each script op, a proof term checked by check_def. The reason for
  (a): (b) is a proof producing elaboration, more than one slice unit.
- Q44 the S3x known gap, the `n` and `r` swap in `mvregWrite` (S3b Unit
  0, 2026-10-29). RULED (a). (a) a side fixture in U2 with 3 atoms or 3
  replicas, and a STATUS line. (b) not in S3b. The reason for (a): one
  fixture kills a live mutant.
