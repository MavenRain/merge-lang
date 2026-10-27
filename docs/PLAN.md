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

S0 status: only the tcc-wasm kit seed of lang-template, `design/DESIGN.md`,
this plan, `README.md`, `AGENTS.md` and `docs/COMPARISON.md` exist. No
merge-lang feature is built. Section 9 gives the slices.

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

Readings (NOT ruled; section 11 has a Q item for each, with (a) as the
recommendation):

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
  (inventory) (Q10 to Q13).

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
- Only `domain/domain.lang` declares a family or a cofamily. A program
  that declares one gets `REFUSE_DATA`.

### 2.2 Every type former merge-lang needs

| Former | Use in merge-lang | Class | Reason |
|---|---|---|---|
| Pi, `A → B` | `apply`, `merge`, `inv`, the read functions; vectors `Fin n → X` | ambient | the internal language |
| Sigma, `(x : A) × B` | the state of a `replicated` declaration (iterated Sigma of its parts); the `MVReg` slot `Fin n × ℕ` | ambient | no `record` form (as evolve-lang) |
| `Fin n` | replica names, atom names (tags, users), amounts | defined | the evolve-lang S2d builtin: `(k : ℕ) × Path Bool (natLe (natAdd k 1) n) true` |
| vectors `Fin n → X` | a set of atoms (`Fin n → Bool`), a version vector (`Fin r → ℕ`), a rights matrix (`Fin r → Fin r → ℕ`) | ambient | no new former |
| `Path`, `PathP`, `I`, `transp`, `hcomp` | the laws (section 6) | ambient | the identity type of Cubical TT |
| `U`, `Glue`, `ua` | none in v1 | ambient, not ported | no merge-lang law needs univalence (O5) |
| replica state | the product of the declared parts | ambient | Sigma of base types; a join-semilattice by the product law (L1 to L3) |
| `GSet n`, `MVReg n r`, `Counter r`, `Escrow r` | the state parts (section 4.2) | base (R2) | defined as Sigma and vectors over base types; no type parameter |
| `Read n` | the read of an `MVReg`: `empty`, `one x`, `conflict s` with a proof that `s` has 2 or more members | base, indexed (R2) | no type parameter (Q11) |
| `Op` of a program | the operations of a declaration, with their arguments | base (R2) | a closed sum that the declaration generates |
| `Option A` | the result of `apply`: `none` when the local precondition fails | CM | one 0-cell `none`, generator `some` |
| op sequences | the changes of one replica in a schedule | ambient | `Fin m → Op`, a vector (Q15) |
| schedules, traces | none in the guest | not admitted | the simulator enumerates them in C (Q16) |
| causal context | the `MVReg` context `Fin r → ℕ` | ambient | a version vector; no new former |
| `FMSet A` | none | not admitted | a set over a finite atom domain is a vector `Fin n → Bool` (Q14) |
| `Pfin⁺ A`, the free join-semilattice | none in v1 | not admitted | the reading of `Read` as the free join-semilattice needs set truncation, a 2-cell (Q11) |
| free CM of operations `Ops A` (mandate-lang 4.6) | none in v1 | not admitted | no description sentence has a script whose later operation reads an earlier response (Q15) |
| `Replica A` cofamily, a Moore machine (mandate-lang 4.7) | none in v1 | not admitted | the simulator replica is a C value (Q16) |
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
| `ℕ`, `natAdd`, `natLe`, `natMax`, `natEq` | base type, ops | D5 (stock, decrements, rights), D4 (version counters) | S1 (kernel), S2 (`natMax`) |
| `Fin n`, `finEq` | base type, op | D3, D4 (atom names), D8 (replica names) | S1 (kernel) |
| `sumFin : (Fin n → ℕ) → ℕ` | op | D5 (total decrements, total rights) | S2 |
| `atoms NAME := a ∣ b ∣ ...` | form | D3 (tags), D4 (owners): a finite domain of opaque names | S3 |
| `replicas A B ...` | form | D8 ("two disconnected replicas"); `Replica = Fin r` | S3 |
| `GSet n`, `add`, `has`, `merge` | base type, ops | D3: two concurrent `add` both survive the join | S2 |
| `MVReg n r`, `write`, `read`, `merge` | base type, ops | D4: concurrent writes keep each value | S2 |
| `Read n`, `empty`, `one`, `conflict` | base type, constructors | D4: the conflict is a value of the read type, so a program cannot take a winner in silence | S2 |
| `Counter r` (an initial stock and a decrement vector), `dec`, `stock`, `nonneg` | base type, ops | D5: an offline decrement; "stock must never become negative" | S2 |
| `Escrow r` (initial split, rights matrix, decrement vector), `dec`, `transfer`, `rights` | base type, ops | D5: "a resource allocation rule" (bounded counter, Balegas et al.) | S2 |
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
| increments (`restock`) | D5 names a decrement only (Q12) |
| `ℤ`, signed values | D5 needs "never negative" as a predicate: `nonneg c` compares the decrements with the stock in ℕ (Q13) |
| sequence and text CRDTs (RGA, Peritext) | no sentence; Automerge covers text |
| JSON documents, nested maps (the Automerge document model) | no sentence; a state is a fixed product of declared parts |
| a merge function that the user writes | D6 "checked merge laws": v1 merges are library merges with proved laws (`REFUSE_NO_MERGE`); a user merge with proof terms is O6 |
| unbounded operation arguments | the simulator enumerates arguments; an argument has a finite type (`REFUSE_OP_INFINITE_ARG`) |
| consensus, locks, leases | D6 asks to distinguish operations that need coordination, not to build a coordination protocol; `transfer` is the one coordination operation |
| membership changes (add or remove a replica) | no sentence; `replicas` fixes the set |
| op-based or delta-state messages | the reading is state based (Q5) |
| `FMSet`, the free CM of operations, a replica cofamily | section 2.2 (Q14, Q15, Q16) |

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
  operations, so it is inflationary (L4).
- An invariant is a predicate `I : State → Bool`. A program can declare
  several. `inv` is their conjunction.
- The initial state is `⊥` with the declared initial values (the initial
  stock and its split).

### 4.2 The library parts

| Part | State | Operations at replica `i` | Merge | Read |
|---|---|---|---|---|
| `GSet n` | `Fin n → Bool` | `add x`: set bit `x` | pointwise `or` | `has x` |
| `MVReg n r` | slots `Fin r → Option (Fin n × ℕ)`, context `Fin r → ℕ` | `write x`: `c := ctx i + 1`; clear each slot; slot `i := some (x, c)`; `ctx i := c` | each slot keeps the entry with the larger counter if the other context does not cover it; context = pointwise `natMax` | `read : MVReg n r → Read n` |
| `Counter r` | initial stock `s0 : ℕ` (declared), decrements `Fin r → ℕ` | `dec k`: precondition `local stock ≥ k`; `d i += k` | decrements by pointwise `natMax` | `stock = s0 - sumFin d` (printed signed); `nonneg = natLe (sumFin d) s0` |
| `Escrow r` | initial split `Fin r → ℕ` (declared), rights matrix `R : Fin r → Fin r → ℕ`, decrements `U : Fin r → ℕ` | `dec k`: precondition `rights i ≥ k`; `U i += k`. `transfer j k`: precondition `rights i ≥ k`; `R i j += k` | `R` and `U` by pointwise `natMax` | `rights i = split i + Σ_j R j i - Σ_j R i j - U i` (compared in ℕ, no subtraction); `stock`, `nonneg` as `Counter` |

Single writer: replica `i` writes only its own slot and counter of
`MVReg`, its own entry of a decrement vector, and its own row of `R`. So
pointwise `natMax` is a join and it loses no write.

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
  conflict clears both slots, so it resolves the conflict. A program that
  uses `read owner` where it needs a user gets a type error with the fix
  "case on `Read`: `one u`, `conflict s`, `empty`".
- Inventory (D5): `sell k := dec stock k` with `invariant nonneg stock`.
  The pair `sell ∥ sell` needs coordination. Counterexample: initial
  stock 1; A sells 1 (local stock 1, precondition holds); B sells 1 (local
  stock 1, precondition holds); merge: stock 1 - 2 = -1; `nonneg stock`
  is false. With `allocate stock by escrow (A ↦ 1, B ↦ 0)`, B's `sell 1`
  fails locally (rights 0 < 1). The coordination it needs is `transfer B
  1` at A. Every schedule keeps `nonneg stock` (bounded), and L7 proves
  it (proved).

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
| `REFUSE_READ_WINNER` | an `MVReg` read is used where a value is needed | "case on `Read`" |
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
- no imports, no floats, no clock, no randomness.

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
  I-confluence shape of J5.
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
| L4 | `apply i o s ≡ some t → le s t` (inflation) | proved (library, per part operation) | S2 |
| L5 | convergence: two replicas that merged the same set of states are equal (a corollary of L1 to L3) | proved (library); the simulator also checks each delivery order (bounded) | S4 |
| L6a | `GSet`: `has (merge (add x a) (add y b)) x` and the same for `y` (D3) | proved (library) | S4 |
| L6b | `MVReg`: concurrent `write x` and `write y` with `x ≠ y` read as `conflict {x, y}` (D4) | proved (library) | S4 |
| L7 | `Escrow`: in each reachable state, `rights i ≥ 0` for each `i` and the sum of the rights is `stock`, so `nonneg` holds (D5) | proved (library) | S4 |
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
| quorum-lang | 09f7f59 | PLAN 2: `FMSet` in generator form, the refused cons form, `Maybe`, `Trunc` (read only) |
| intent-lang | 67445ae | `REFUSE_CELL`, cofamily (in evolve-lang 1f87a5a already) |
| resolve-lang | 632d819 | `REFUSE_CELL` (second implementation, read only) |
| mandate-lang | 94de313 | PLAN 4.6 free cell monad of operations (not admitted, Q15) |
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
| S1 | one port of evolve-lang 1f87a5a (section 8): surface (Q7), `langc fmt`, `W_NONCANON`, `check --json` with `parsed` and `typed` counts, the S2a to S2d kernel; the kit sample domain stays (O4) | `make check` EXIT 0 with the evolve-lang 1f87a5a counts (its PLAN 9 S2d row: examples 10; parse refusals 10; check refusals 54; evals 62; ir 10; wasm 13; differential 773 cases, 0 mismatches; fmt 101 canonical, 6 non-canonical pairs; json 26 fixtures, diagnostics 38 codes; cubical 36 checks, 22 round trips), re-measured at S1 STEP 0 on evolve-lang 1f87a5a; mutants: the `REFUSE_CELL` CM test, the `Fin` bound, the `W_NONCANON` test |
| S2 | domain: `natMax`, `sumFin`, `GSet`, `MVReg`, `Read`, `Counter`, `Escrow`, the product join; L1 to L4 as library guest terms; replace the kit sample domain | one eval test per part operation and merge; L1 to L4 type-check; mutants on each merge (swap `natMax` for `natAdd`, drop the context test of `MVReg`) |
| S3 | forms: `replicas`, `atoms`, `replicated`, `state`, `op`, `invariant`, `allocate`, elaborated to guest terms (Q9); `REFUSE_NO_MERGE`, `REFUSE_OP_INFINITE_ARG`, `REFUSE_RAW_WRITE`, `REFUSE_ALLOC_SUM`; `examples/tags.merge`, `owner.merge`, `inventory.merge`, `inventory-escrow.merge` parse and type | one fixture per code; `fmt` round trip on the forms; mutants on each J1 and J2 test |
| S4 | the checker: J4 to J6, the bounded search core in C (shared with S5), `REFUSE_NEEDS_COORDINATION`, `REFUSE_RULE_MISMATCH`, `REFUSE_READ_WINNER`, `W_BOUNDED`; L5, L6a, L6b, L7, L8; `laws` and `pairs` counts in `check --json` | one test per code with its JSON shape; `inventory.merge` refused, `inventory-escrow.merge` GREEN with L7 proved; mutants on each class test and on L7 |
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
- O4 the kit sample domain (`domain/domain.lang`) and its examples stay
  until S2 replaces them. The kit examples use `.lang`; the merge-lang
  examples use `.merge` (R5 names `examples/inventory.merge`).
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
- O11 the MVReg merge with one slot per replica (4.2) is correct only
  under the single-writer rule. S2 proves L1 to L3 for it; if the proof
  fails, S2 falls back to the dotted set form and records it.

## 11. Questions for the USER

Each item has options. (a) is the recommendation. All are OPEN at S0.

- Q1 R1 reading. (a) the precedent reading of 2.1: ambient Pi, Sigma,
  Path, U are not limited; declared type constructors are only CM and
  CC; the cell test is syntactic; cons is refused; the dropped clause
  named the model, not a former. (b) the same, and also refuse higher
  inductive types (no path constructors in a CM).
- Q2 kit and port path. (a) tcc-wasm seed; S1 = one port of evolve-lang
  1f87a5a (surface and kernel S2a to S2d); `json.c` from tcc-json in S6.
  (b) port from leaf-lang, quorum-lang and intent-lang one by one, as
  evolve-lang did (four slices more).
- Q3 Wasm target. (a) an engine module per program, emitted by the Wasm
  writer of `langc`; `langc` itself is native, not Wasm. (b) build
  `langc` with clang for wasm32-wasi (this breaks R3).
- Q4 engine ABI. (a) the memory ABI of 5.1 (export `memory`, `alloc`,
  `reset`; state pointers; a writer change so that engine entries keep
  the heap). (b) pack each state in one `i64` and refuse larger states.
- Q5 merge model. (a) state based: a state is a join-semilattice, merge
  is the join, each operation is inflationary. (b) operation based, with
  causal delivery assumed.
- Q6 discharge marks. (a) proved, bounded, refuted; the report never says
  proved for bounded. (b) proofs only: refuse a law with no proof term.
- Q7 surface. (a) the Agda-like Unicode surface of quorum-lang (Q13),
  leaf-lang (Q14) and evolve-lang (Q7). (b) ASCII keywords. Unicode
  symbols can cost more tokens; S9 measures it.
- Q8 proofs. (a) guest language only. (b) also a Lean export.
- Q9 declaration forms. (a) dedicated forms `replicas`, `atoms`,
  `replicated`, `state`, `op`, `invariant`, `allocate` that elaborate to
  guest terms. (b) plain guest definitions over the domain types only.
- Q10 tags. (a) `GSet` with `add` only (D3 names additions). (b) the
  add-wins OR-set with dots, and `remove`.
- Q11 owner read type. (a) `Read n`, a base indexed family: `empty`,
  `one x`, `conflict s` with a proof that `s` has 2 or more members. (b)
  `Pfin⁺ A`, the free join-semilattice as a CM higher inductive type; it
  needs set truncation, a 2-cell, and the evolve-lang kernel has 1-cells
  only (its Q-S2b-5).
- Q12 inventory operations. (a) `sell` (a decrement) only, with a
  declared initial stock and initial split; the escrow rule is the
  rights and `transfer`. (b) also `restock` (increments, Balegas
  `R[i][i]`).
- Q13 integers. (a) ℕ only; `nonneg` compares in ℕ; the transcript prints
  the signed stock. (b) admit ℤ.
- Q14 `FMSet`. (a) not admitted; a set over an atom domain is a vector
  `Fin n → Bool`. (b) admit `FMSet` in generator form (quorum-lang).
- Q15 free CM of operations. (a) not admitted; the changes of a replica
  are a vector of operations. (b) admit `Ops A` (mandate-lang 4.6) for
  scripts whose later operations read earlier responses.
- Q16 replica as a cofamily. (a) not admitted; the simulator replica is
  a C value. (b) a `Replica` cofamily, a Moore machine (mandate-lang 4.7
  `Machine`), so that convergence is a statement about guest replicas.
- Q17 simulator bounds and order. (a) the 5.2 defaults (`P = 1`,
  `M = 2`, the declared replicas), order by total operations then
  declaration order, the reduction of 5.2. (b) seeded random schedules
  (this breaks byte determinism).
- Q18 demo files. (a) `examples/inventory.merge` (no rule: refused with
  the counterexample) and `examples/inventory-escrow.merge` (kept), both
  golden; the demo transcript is the first. (b) one file and a flag that
  adds the rule.
- Q19 tokenizer for the baseline. (a) a fixed offline proxy, as leaf-lang
  Q13 and evolve-lang Q12. (b) a named BPE tokenizer through a script.
- Q20 report entry. (a) `langc build` writes `NAME.report.json`; `langc
  check --json` prints the same schema with diagnostics. (b) a separate
  `langc report` command.
- Q21 engine code. (a) one IR template per part, plus the invariants
  lowered through the kit IR; the differential gate checks the module
  against `langc eval` of the guest terms. (b) lower the guest terms of
  the parts directly (the IR then needs finite vectors as arrays).
