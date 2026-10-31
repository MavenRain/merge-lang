# merge-lang status

The build implements slices S1, S1b, S2, S2b, S2c, S2d, S3a, S3e, S3x,
S3b and S4 of `docs/PLAN.md` section 9: the port of the evolve-lang 1f87a5a
surface and cubical kernel (S1), induction in the kernel (S1b), the
merge-lang domain with the laws L1 to L4 as guest terms (S2), ℤ with the
`stock` read (S2b), path constructors of dimension 2 with the `fold` over
a higher inductive type (S2c), `isSet` for each part with `mergeAll` on
the program side (S2d), the parse and `fmt` of the merge forms (S3a), the
elaboration of the forms to guest terms (S3e), the check of the four
examples in the gate (S3x), an `op` body as an `Ops` script of part
reads and part operations (S3b), and the checker: the classes, the
bounded search, the coordination codes and the laws L5 to L8 (S4). It
does not complete
M0. There is no merge engine, no replica simulator and no coordination
report yet.

## S4 (2026-10-31)

- Scope (`docs/PLAN.md` section 9, the S4 row; Q45 (a), Q46 (b), Q47
  (a), Q48 (a), Q49 (b), Q50 (a), Q51 (a), Q52 (a), Q53 (b), Q54 (c),
  Q55 (a)): the `Replica` machine, the pair search, four new codes, and
  the laws L5 to L7. The base is the USER S3b commit 263cf6d.
- U1 (Q51 (a), Q45 (a)): `FMSet`, `IsSet`, `mergeAll` and the Hedberg
  set proofs move from `test/eval/mergeall.lang` to `domain/domain.lang`
  (`collBoolT`, `collNat`, `gsetIsSet`, `counterIsSet`, `escrowIsSet`,
  `mvregIsSet`). Each form gets `NAMEL5 : FMSet NAME → NAME`, which is
  `mergeAll NAME NAMEBot NAMEMerge`. `form_machine` adds `NAMEOp` (one
  `NAMEOpWord` per op), `NAMEEvent` (`NAMELocal` with `.i : Replica` and
  `.o : NAMEOp`, `NAMERecv` with `.s : NAME`), the codata `NAMEMachine
  (S : Type 0)` with `NAMEMachineMk` (`NAMEEps`, `NAMEStep`), and
  `NAMEReplicaOf : NAME → NAMEMachine NAME`. Tests:
  `test/eval/machine.lang` (5 evals, the last is `ShopL5` on a two
  element `FMSet`) and `test/check/machine-open.lang` (a written `Event
  S`: `REFUSE_CELL`).
- U2 (Q47 (a), Q48 (a)): `src/front/search.c` and `search.h`. The
  function `search_pair` runs a depth first search in the 5.2 order with
  the bounds 1, 2 and N and `SEARCH_CAP` 100000. A pair that L7, L6a or
  L6b covers skips it. A pair that the search keeps gets the new code
  `W_BOUNDED`: a warning record in `check --json`, one stderr line in
  text mode, rc 0, stdout `ok`. Test: `test/json/W_BOUNDED.lang`
  (`refill` on a plain Counter, K = 108).
- U3 (Q49 (b)): the new code `REFUSE_NEEDS_COORDINATION` comes from
  `form_violation` (J6): the message names the pair and the schedule,
  and `found` is the merged state. An `allocate` line takes the clause
  `for nonneg d`. It is optional and its default is `nonneg c`. The new
  code `REFUSE_RULE_MISMATCH` is raised at `for` when `d` is not `c`.
  The new code `REFUSE_READ_WINNER` comes from `op_reader`: an `MVReg`
  read name used as a write argument. Fixtures:
  `REFUSE_NEEDS_COORDINATION.lang`, `REFUSE_RULE_MISMATCH.lang` and
  `REFUSE_READ_WINNER.lang` in `test/json`, `test/fmt/allocate-for.lang`
  with its `.canon`, and `test/parse/allocate-for-no-body.lang`.
  `inventory.merge` is now refused (`sell ∥ sell`): merge examples 4
  canonical, 3 checked, 1 refused. The `Stock` form left
  `test/eval/escrow.lang`.
- L5r (Q46 (b), Q54 (c)): `NAMERecvN`, `NAMEL5rM` (an ℕ fold: base
  `refl`, step `cong deliver ih`) and `NAMEL5r : Path NAME (NAMEEps NAME
  (NAMERecvN (NAMEReplicaOf s) f n)) (mergeSeq NAME NAMEMerge s f n)`.
  The domain has `fmOf`, `mergeSeq` and `mergeSeqAll`. The multiset step
  is `mergeSeqAll` at an abstract `S`. It is not applied for each form.
  Evidence: `firstA`, `firstB`, `orderAB`, `orderBA` and `deliveredAB`
  in `test/eval/machine.lang`.
- L6a (Q53 (b)): the domain has `orTrueR`, `gsetHasMergeL` and
  `gsetHasMergeR`. The instance is the domain lemma at the `n` of the
  part. A form has no `NAMEL6a` def.
- L6b (Q55 (a)): the domain has `mvregWf`, `mvregBotWf`, `mvregMergeWf`,
  `mvregWriteWf` and `mvregConflict n r i j x y s wf hij hxy : T (natLe
  2 (mvregCount n r (mvregConc n r i j x y s)))`. The lemma holds at
  each `s` with `mvregWf n r s`. The instance is the domain lemma at the
  `n` and `r` of the part.
- L7 (Q52 (a)): the domain has `EscrowOwn`, `escrowRightsMerge`,
  `escrowRightsDec`, `escrowRightsTransfer`, `escrowRightsRestock`,
  `escrowNonnegSum`, `escrowStep`, `escrowReconnect`, the bundles
  `escrowDecOk`, `escrowTransferOk` and `escrowRestockOk`, `escrowL7`
  and the type `EscrowL7Law r split`. Each allocated part gets
  `NAMEPARTL7` (for example `ShopStockL7`). L7 scope: the proof covers
  the 5.2 shape (the base, one prefix op, one op per branch, the merge),
  not all schedules. The other merge order is the same lemma with `i`
  and `j` swapped. Evidence: `examples/inventory-escrow.merge` checks
  `ok`.
- Builtin ℕ lemmas (Q53 (b); typing rules only, no reduction; 25 in all).
  U4k: `natEqRefl`, `natEqSound`, `natLeRefl`, `natLeTrans`,
  `natLeMaxL`, `natLeMaxR`, `natMaxLeast`, `natLeAddMono`, `natLeSucc`,
  `natEqSuccNe`, `sumFinMono`. L6b: `natLeMulTwo`, `natLeAntisym`,
  `natEqSym`, `natMulTwoSucc`, `sumFinLe`, `sumFinLeTwo`. L7:
  `natAddAssoc`, `natAddComm`, `sumFinBumpLe`, `sumFinAdd`,
  `sumFinSwap`, `natLeAddCancel`, `sumFinZero`, `finEqSound`.
  `finEqSound` is a `Fin` fact, not an ℕ fact.
- U5 (Q50 (a)): `check --json` prints the progress object in the 5.3
  form, with `affine` removed. `laws` counts L1 to L4, L5 and L5r (6 for
  each form), one L6a or L6b for each `GSet` or `MVReg` part, one L7 for
  each allocated part, and one L8 for each pair no rule covers (a
  proved, bounded or refuted mark). `pairs` counts classified pairs. New
  fixture: `test/json/pair-law.lang` (laws 14 of 14 proved).
- U6 mutants: 12/12 KILLED by a test, `check-clang` rc 0 on each. C1 and
  C2 `pair_covered` (`test/json/pair-law.lang`; first run they survived,
  because no fixture had that pair kind). C3 l7 term false
  (`inventory-escrow.merge` `W_BOUNDED` `sell ∥ refill`). C4
  `SEARCH_VIOLATION` test false (`inventory.merge` exit 0, json
  `REFUSE_NEEDS_COORDINATION.lang`). N1 `natLeTrans` arity 4 (review,
  cubical, the row `natLeTrans`, the domain `escrowRightsMerge`
  `TYPE_MISMATCH`). L1 `escrowStep` `hs i` (domain `TYPE_MISMATCH`). L2
  `form_l7` `r + 1u` (`ShopStockL7` `TYPE_MISMATCH`). R1 `d != c` test
  (`test/json/REFUSE_RULE_MISMATCH.lang`). F1 fmt clause (fmt on
  `REFUSE_RULE_MISMATCH.lang`, `allocate-for.canon`). O1 `op_reader`
  (`test/json/REFUSE_READ_WINNER.lang`). V1 `form_violation` (merge loop
  and json). U1 `law_refuted` (json progress line).
- Review fixes: L6b skips the search only when both ops write MVReg
  parts exclusively, so a concurrent Counter decrement is still checked.
  The checker reserves both generated definitions per op and allocate
  line. Builtin natural-number proofs print with their surface names and
  keep those names free of binder capture. C regressions cover the mixed
  write, table capacities and proof source round trips.
- Known edges:
  - L7 has no branch of two ops at one replica, and no empty branch.
  - Each allocated part gets one `NAMEPARTL7`, not one `NAMEL7`.
  - `finEqSound` grows the trust base by a `Fin` fact.
  - Q54 (d), a kernel `conv` shortcut for two stuck folds, is open.
- `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 23;
  check refusals 76; evals 206; ir 13 lowered; wasm 16 modules valid;
  differential 1230 cases, 0 mismatches; fmt 163 canonical, 14
  non-canonical pairs; merge examples 4 canonical, 3 checked, 1 refused;
  json 35 fixtures, diagnostics 46 codes (32 with a fixture, 14 with no
  fixture); gate 0 failures; review, domain and diagnostic edit
  regressions passed; cubical 124 checks, 41 round trips. The S3b counts
  were parse 22, check 75, evals 200, fmt 155 and 13 pairs, merge
  examples 4 checked, json 30, diagnostics 42 codes (28 with a fixture),
  cubical 69 checks.

## S3b (2026-10-29)

- Scope (`docs/PLAN.md` section 9, the S3b row; Q39 (a) to Q44 (a)):
  the kernel CM test admits a field `E → T A` with `E` closed data, the
  library `Ops A`, an `op` body as an `Ops` script of part reads and
  part operations, and L4 for scripts by composition. The base is the
  USER S3x commit a6d7a56.
- U1 (Q39 (a), Q40 (a)): the CM test admits an arrow field `E → T A`
  when `E` is closed data and the field has one use of the family. `E`
  names no parameter and no family, but it can name an earlier field
  (`opRead (readN : ℕ) (readPart : ℕ) (readNext : Read readN → Ops A)`).
  The `fold` at an arrow field is the catamorphism: the method gets `E
  → R`, in eval, check and the IR lowering of `elim`. The `REFUSE_CELL`
  text adds "or a function from closed data to it". Refused fixtures in
  `test/check`: `cell-field.lang` (`E` is the parameter `A`),
  `cell-arrow-self.lang` (`W A → W A`) and `cell-arrow-two.lang` (two
  uses of the family in one field).
- U2 (Q41 (a), Q44 (a)): `domain/domain.lang` has `Ops (A : Type 0)`
  with `opPure`, the reads `opRead` and `opStock`, and the writes
  `opAdd`, `opWrite`, `opDec`, `opRestock` and `opTransfer` (part index,
  arguments, next script). A form with a script op gets the script
  value `NAMEOPScript` and one runner `NAMERun`. The runner answers a
  read from the current state and applies a write; it is one `elim`
  with no fuel. Tests: `test/eval/ops-lib.lang` and `test/eval/ops.lang`.
  `test/eval/owner3.lang` has 3 atoms and 2 replicas, so the swap of `n`
  and `r` in `mvregWrite` is now a TYPE_MISMATCH (the S3x gap is
  closed).
- U3 (Q42 (a)): a script is one line of steps joined by `;`. A read
  binds with `←` (or `<-`): `x ← read PART` (a `GSet` or `MVReg` part,
  `Read n`) or `x ← stock PART` (a `Counter` or `Escrow` part, `ℤ`). A
  write argument can use the parameters and the earlier read names.
  `fmt` prints `; ` between steps (`test/fmt/form-op-steps.lang`). Tests:
  `test/eval/script.lang` (a read, then a write), `script-dec.lang` (a
  partial `dec` step: a `none` step stops the script),
  `test/check/read-counter.lang` and `stock-mvreg.lang` (a read word on
  the wrong part kind: `REFUSE_RAW_WRITE`).
- U4 (Q43 (a)): for each part `P` (gset, counter, escrow, mvreg),
  `domain/domain.lang` has `PLe s t = Path P (merge s t) t` and
  `PLeTrans : PLe s t → PLe t u → PLe s u` (from associativity and the
  part L4). `test/eval/le.lang` proves the L4 of three scripts of two
  steps; `test/check/le-order.lang` gives the two steps in the other
  order and gets TYPE_MISMATCH. The L4 for all scripts is an induction
  on the step count (the S3b design text): each step gives `PLe s(i-1)
  si` (`PIdem` for a read or for a step on a different part, the op L4
  for a write), and `PLeTrans` joins them.
- U5 mutants: 10/10 KILLED by a test. M1 the closed test of `E`
  (`cell-field.lang`); M2 `self_uses == 1u` to `>= 1u`
  (`cell-arrow-two.lang`); M3b `fold_arrow_field` returns `k` (16 FAIL
  lines, first `test/eval/ops stock`); M4 the telescope (388 FAIL); M5
  `family_class` (388 FAIL); M6 the script step order (`script
  claimBot`); M7 the read part index `j - 1u` to `j % u->k`
  (`stock-mvreg.lang`, `shop tagged`); M8b the `run_cast` false branch
  to `yes` (35 FAIL lines, first `raw-write.lang`); M9a the total `kc`
  to `some` (`script bothBen`); M9b the partial `kc` to `some`
  (`script-dec soldTagged`). The first forms of M3 and M8 were KILLED
  by `check-clang` only (an unused name), so M3b and M8b replace them.
- Known edges:
  - Q43 (b) is not done: the elaboration does not give a `NAMEOPL4`
    term for each script op.
  - `langc check domain/domain.lang` gives rc 1 (`REFUSE_NAME` on
    `Verdict`). The gate only does the `fmt` check of the domain. This
    is older than S3b.
- Review fixes (2026-10-30): read binders are fresh against earlier
  generated read binders, preserving distinct names and lexical shadowing.
  Runner and script helpers append primes when needed to preserve public
  operation names (`run`, `run'`, `tagScript`) and scoped parameters.
  The default names remain `NAMERun` and `NAMEOPScript`. A fold maps path
  fields in every cell monad, including one with no path constructors.
  Six C regressions in `test/cubical.c` cover these cases: 69 checks and
  38 round trips pass.
- `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 22;
  check refusals 75; evals 200; ir 13 lowered; wasm 16 modules valid;
  differential 1230 cases, 0 mismatches; fmt 155 canonical, 13
  non-canonical pairs; merge examples 4 canonical, 4 checked; json 30
  fixtures, diagnostics 42 codes (28 with a fixture, 14 with no
  fixture); review, domain and diagnostic edit regressions passed;
  cubical 63 checks, 38 round trips. The S3x counts were check 70,
  evals 173, fmt 143 and 12 pairs.

## S3x (2026-10-29)

- Scope (`docs/PLAN.md` section 9, the S3x row): the four examples
  `examples/*.merge` parse and type in the gate, the docs and the S3
  close. The base is the USER S3e commit 2b4edf4.
- Gate: the `examples/*.merge` loop of `test/gate.sh` runs `langc
  check` on each file after `fmt`. It fails on an exit that is not 0,
  an output that is not `ok`, or a stderr line (for example a `W_*`
  warning). The count line is now `merge examples: 4 canonical, 4
  checked`.
- Mutants: 4/4 example mutants KILLED by the loop only. Each mutant
  file stays canonical, so the `fmt` part passes: `write` on the GSet
  of `tags.merge`, `add` on the MVReg of `owner.merge`, an `Escrow`
  part with an init in `inventory.merge`, and the shares `(A ↦ 1, B ↦
  1)` in `inventory-escrow.merge`. No `check.c` path is reached by the
  examples only: the S3e tests also kill the `counterRestock` and the
  allocate `over` mutants.
- Gap: the swap of `n` (the atoms count) and `r` (the replica count) in
  the `mvregWrite` term survives the gate. Each MVReg test and example
  has 2 atoms and 2 replicas. S3b U2 (Q44 (a)) closes the gap:
  `test/eval/owner3.lang` has 3 atoms and 2 replicas, so the swap is a
  TYPE_MISMATCH there.
- `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 22;
  check refusals 70; evals 173; ir 13 lowered; wasm 16 modules valid;
  differential 1230 cases, 0 mismatches; fmt 143 canonical, 12
  non-canonical pairs; merge examples 4 canonical, 4 checked; json 30
  fixtures, diagnostics 42 codes (28 with a fixture, 14 with no
  fixture); cubical 63 checks, 38 round trips.

## S3e (2026-10-29)

- Scope (`docs/PLAN.md` section 9, the S3e row; Q33 (a) to Q38 (a)):
  `langc check` elaborates the 7 forms to guest terms on the S2 domain
  parts. Each form gives synthetic `DECL_DEF` nodes that `check_def`
  checks (Q33 (a)). A type error points at the form word.
- `replicas` and `atoms`: the type `NAME = Fin n` and one definition for
  each member. A numeric `atoms` domain also gives `NAMEValue : NAME →
  ℕ` (Q34 (a)). A name clash gives `REFUSE_NAME` at the form word.
- `replicated NAME`: the state type `NAME`, `NAMEBot` and `NAMEMerge`.
  A `state` line of a `GSet` or `MVReg` part names an atoms domain
  (`state tags : GSet Tag`, Q36 (a)). An `op` body is one part
  operation (Q35 (a)) and gives one definition (`ShopSell` for `op
  sell`, Q38 (a)); a partial part operation gives an `Option`.
  `allocate` on a `Counter` part gives an `Escrow` part and
  `NAMEPARTSplit : Replica → ℕ` (`ShopStockSplit`). `invariant nonneg
  PART` is the only invariant body (Q37 (a)); it gives `NAMEInv : NAME
  → Bool`. `domain/domain.lang` gets `counterNonneg` and
  `escrowNonneg`.
- New codes (`docs/DIAGNOSTICS.md`, each with a fixture
  `test/json/CODE.lang`): `REFUSE_NO_MERGE` (a `state` part with no
  merge, a bare `GSet` or `MVReg`, an init on a part that is not a
  `Counter`, an `allocate` on a part that is not a `Counter`, a second
  `allocate`, a form with no `state`), `REFUSE_OP_INFINITE_ARG` (an
  `op` parameter type that is not finite; the span is the parameter
  name), `REFUSE_RAW_WRITE` (an `op` body that is not a part operation
  of its part, or an `invariant` body that is not `nonneg PART`) and
  `REFUSE_ALLOC_SUM` (two shares for one replica, a replica with no
  share, or a sum of shares that is not the init). `REFUSE_FORM` and
  `test/json/REFUSE_FORM.lang` are removed. An unknown atoms name, part
  or share replica, or no `replicas` form, gives `NAME_UNKNOWN`.
- Parser: the span of a Field is its name token, not the `(` of the
  binder group.
- Examples: `tags.merge` and `owner.merge` name an atoms domain (Q36
  (a)). The four examples pass `langc check`. The `examples/*.merge`
  gate loop is still fmt only (S3x added `check`).
- Tests: 10 new `test/check` fixtures (`alloc-missing`, `alloc-sum`,
  `alloc-twice`, `alloc-under`, `atoms-clash`, `form-invariant`,
  `op-arity`, `op-fin-open`, `raw-write`, `replicas-twice`) with the
  text pinned; `test/check/form.lang` now gives `REFUSE_NO_MERGE`;
  `test/eval/replicas.lang`, `shop.lang` and `escrow.lang` with 7 rows
  each; 4 new json fixtures. The gate pair branch: `form-replicated`
  stops with `REFUSE_OP_INFINITE_ARG`, `form-op` and `form-state` stop
  with `REFUSE_NO_MERGE`.
- Gate: `make check` EXIT 0. Examples 13; parse refusals 22; check
  refusals 70; evals 173; ir 13 lowered; wasm 16; differential 1230
  cases, 0 mismatches; fmt 143 canonical, 12 non-canonical pairs; merge
  examples 4 canonical; json 30 fixtures, diagnostics 42 codes (28 with
  a fixture, 14 with no fixture); gate 0 failures; review, domain and
  diagnostic edit regressions passed; cubical 46 checks, 38 round trips.
- Count changes from S3a: check refusals +10; evals +21; fmt +16
  canonical; json +3 (4 new, `REFUSE_FORM` out); diagnostics +3 codes,
  each with a fixture. The other counts are the same.
- Mutants (14, each KILLED by a gate test; 4 after new fixtures): the
  J1 `REFUSE_NO_MERGE` head test; the `REFUSE_OP_INFINITE_ARG` call, its
  `Fin` numeral test (`op-fin-open`) and its `Replica` test (eval
  escrow); the parser Field span (the json column); the
  `REFUSE_ALLOC_SUM` `over` test, the sum under the init
  (`alloc-under`) and the share count (`alloc-missing`); the J2
  `REFUSE_RAW_WRITE` word test, part kind test and arity test
  (`op-arity`); the `cg_value` names and `natEq` (eval replicas);
  `op_amount` (eval shop).

## S3a (2026-10-28)

- Scope (`docs/PLAN.md` section 9, the S3a row; Q32 (a), Q34 (a)): the
  parse and `fmt` of the forms `replicas`, `atoms`, `replicated`,
  `state`, `op`, `invariant` and `allocate`. There is no elaboration
  yet (S3e).
- Lexer: the token `∣` (between the members of `atoms`) and 10 form
  words: `replicas`, `atoms`, `replicated`, `state`, `op`, `invariant`,
  `allocate`, `by`, `escrow` and `init`.
- AST and parser: 3 Decl kinds (`DECL_REPLICAS`, `DECL_ATOMS`,
  `DECL_REPLICATED`) with `FormName` and `FormItem`. A `replicated NAME`
  body has one item a line at one column (the evolve-lang `protocol`
  layout). The items are `state`, `op`, `invariant` and `allocate`. An
  `atoms` domain with numerals and names is a parse refusal (Q34 (a)).
- `fmt`: the printers for the 7 forms. `fmt` adds a blank line after
  `replicas`.
- `check`: a `replicas`, `atoms` or `replicated` form gets the new code
  `REFUSE_FORM` until S3e (`docs/DIAGNOSTICS.md`, fixture
  `test/json/REFUSE_FORM.lang`).
- Tests: 11 parse refusals, one for each bad form shape, with the text
  pinned in `test/parse/expect.txt`; `test/check/form.lang`; 6 pairs
  `test/fmt/form-*.lang` and `.canon`. For each pair file, the gate also
  checks that `langc check` stops with `REFUSE_FORM`.
- Examples: `examples/tags.merge`, `owner.merge`, `inventory.merge` and
  `inventory-escrow.merge`. A new gate loop checks that each
  `examples/*.merge` is `fmt` canonical.
- Gate: `make check` EXIT 0. Examples 13; parse refusals 22; check
  refusals 60; evals 152; ir 13 lowered; wasm 16; differential 1230
  cases, 0 mismatches; fmt 127 canonical, 12 non-canonical pairs; merge
  examples 4 canonical; json 27 fixtures, diagnostics 39 codes (25 with
  a fixture, 14 with no fixture); gate 0 failures; review, domain and
  diagnostic edit regressions passed; cubical 46 checks, 38 round trips.
- Count changes from S2d: parse refusals +11; check refusals +1; fmt +8
  canonical, +6 pairs; json +1; diagnostics +1 code with a fixture; the
  merge examples line is new. The other counts are the same.
- Mutants (11, each KILLED by a gate test): the numeric member branch
  of `atoms` takes a name (`atoms-mix-nat`); the first-name branch
  messages swapped (`atoms-mix-name`); the `replicated` item loop takes
  a column >= the item column (`replicated-column`); the `at_init` test
  of `state` always true (`state-init-name`, 2 form pairs); the `↦` of
  `allocate` optional (`share-no-mapsto`); `fmt` drops the `init` print
  (the form pairs); the share separator `, ` as a space
  (`form-replicated`, `inventory-escrow.merge`); the `∣` separator as a
  space (`form-atoms-*`); the `REFUSE_FORM` test drops `atoms`,
  `replicated` or `replicas` (3 mutants: the form pair branch,
  `form.lang`).
- Note for S3e: without the `REFUSE_FORM` test, `langc check` on a form
  stops with SIGSEGV (exit 139), because the checker reads the form
  Decl as a def. S3e must take the 3 form kinds before the generic Decl
  path.

## S2d (2026-10-28)

- Scope (`docs/PLAN.md` section 9, the S2d row; Q29 (a), Q30 (b), Q31
  (a)): `isSet` for each part and `mergeAll : FMSet S → S` on the
  program side.
- Kernel (Q31 (a)): one hunk in `src/front/eval.c` before
  `reduce_fold_nat`. The ℕ `fold` at an open `natAdd y c` (a numeral
  `c >= 1`) is the `fold` at `y`, then `c` steps with the counters `y`,
  `natAdd y 1`, ..., `natAdd y (c - 1)`. There is no `check.c` change:
  conversion compares evaluated values.
- `test/eval/mergeall.lang`: `isSet` for each part (`GSet`, `Counter`,
  `Escrow`, `MVReg`, the product) by a Hedberg port with no empty type.
  `collSet` uses one `hcomp` with 4 faces. The `absurd` terms use
  `transp` along a large `fold` or an `elim` into `Type 0`. `mergeAll`
  takes the bottom, the `merge`, its unit, associativity and
  commutativity laws, and `isSet` of the part. 5 evals: `allGs` `2`,
  `allCt` `12`, `allEs` `15`, `allMv` `2`, `allGc` `pack 2 12`.
- `test/eval/fold-open.lang`: the laws `predOneLaw` (c = 1),
  `predTwoLaw` (c = 2) and `addTwoLaw` (no motive, c = 2) are `refl`.
  One eval: `predAtFive` `6`.
- Gate: `make check` EXIT 0. Examples 13; parse refusals 11; check
  refusals 59; evals 152; ir 13 lowered; wasm 16; differential 1230
  cases, 0 mismatches; fmt 119 canonical, 6 non-canonical pairs; json 26
  fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture);
  gate 0 failures; review, domain and diagnostic edit regressions passed;
  cubical 43 checks, 36 round trips.
- Count changes from S2c: evals +6; fmt +2 canonical. The other counts
  are the same.
- Mutants (all KILLED): M1 the `mcomm` case of `mergeAll` as `cm h g`
  (`TYPE_MISMATCH` at `mergeAll`, 5 failures); M2a the first counter
  `natAdd y 0` (`TYPE_ENDPOINT` at `fN`, 6 failures); M2b each counter
  `y` (`TYPE_REFL` at `predTwoLaw`, 1 failure); M2c one step short
  (`TYPE_MISMATCH` at `absurdZS`, 6 failures).
- Drift: `src/front/eval.c` is shared with the tcc kits. The open
  successor rule is a merge-lang change.

### Review corrections

The dependent fold at `natAdd y 2` normalized a proof to a path with
endpoint `natAdd (natAdd y 1) 1`, which did not match the declared
endpoint `natAdd y 2`. The kernel now combines adjacent literal offsets
in `natAdd` when their sum fits the numeral range. `natMax` peels common literal
offsets so its existing successor equations remain convertible.
Two dependent fold round trips and three `natMax` successor checks in
`test/cubical.c` cover the fix. The review gate passes with 46 cubical
checks and 38 round trips; the other S2d counts above are unchanged.

## S2c (2026-10-28)

- Scope (`docs/PLAN.md` section 9, the S2c row; O13, Q29 (a), Q30 (b)):
  the kernel port of path constructors of dimension 2, a bottom and a
  left unit law for each part, and `FMSet` as program data. `isSet` and
  `mergeAll` are slice S2d.
- Kernel port (quorum-lang 09f7f59 and 351cbf4, one hunk at a time):
  `check_pctor` accepts a path constructor of dimension 1 or 2 and
  refuses dimension 3 (`REFUSE_CELL`, "has dimension 3; only 1 and 2 are
  available"). The `fold` over a higher inductive type is in `check.c`,
  `eval.c` and `lower.c`. At a 1-cell, the `fold` applies the case to the
  fields (a recursive field is folded) at the interval argument. At a
  2-cell, or at a cell with a path field, the `fold` stays stuck (O52).
- Domain (`domain/domain.lang`): a bottom and a left unit law for each
  part (`gsetBot`/`gsetUnit`, `counterBot`/`counterUnit`,
  `escrowBot`/`escrowUnit`, `mvregBot`/`mvregUnit`, `pairBot`/`pairUnit`).
  Each unit law is `refl`: `or false y` is `y` and `natMax 0 b` is `b`.
- `FMSet` (Q30 (b)): each domain family is `FAMILY_BASE`, and R1 refuses
  a path constructor in it. `FMSet` (`msing`, `mnil`, `munion`, `munit`,
  `massoc`, `mcomm`, the 2-cell `trunc`) and `fmsetMap` are program data
  in `test/eval/fmset.lang`.
- Tests: `test/check/cell-path-dim.lang` moves to `test/eval` (accepted
  at dimension 2); `test/check/cell-path-dim3.lang` gets `REFUSE_CELL`;
  `test/check/hit-fold.lang` gets `TYPE_MISMATCH` (a 1-cell case must be
  a `PathP`); `test/eval/hit.lang` (5 evals: `Circle` and the square `Sq`,
  with the stuck `fold` at the 2-cell); `test/eval/fmset.lang` (7 evals:
  `fmsetMap` at `mnil`, at two points and at `mcomm`; a read of the
  bottom of `GSet`, `Counter`, `Escrow` and `MVReg`).
- Gate: `make check` EXIT 0. Examples 13; parse refusals 11; check
  refusals 59; evals 146; ir 13 lowered; wasm 16; differential 1230
  cases, 0 mismatches; fmt 117 canonical, 6 non-canonical pairs; json 26
  fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture);
  gate 0 failures; review, domain and diagnostic edit regressions passed;
  cubical 43 checks, 36 round trips.
- Count changes from S2b: evals +13; fmt +3 canonical. The other counts
  are the same.
- Mutants: M1 the dimension bound `> 1u` KILLED (13 failures); M2 the
  bound `> 3u` KILLED (1 failure); M3b the `fold` at a 1-cell with an
  unfolded recursive field KILLED (6 failures). M3a, the wrong end in the
  `fold` at a 1-cell, SURVIVED: an end at an interval variable does not
  show in a printed value.
- Drift: `src/front/check.c`, `eval.c` and `lower.c` are shared with the
  tcc kits. The dimension 2 port is a merge-lang change.

## S2b (2026-10-27)

- Scope (`docs/PLAN.md` section 9, the S2b row; Q13 (b), Q22 (a)): ℤ and
  the `stock` read. No kernel edit.
- Token: the lexer reads `ℤ` (U+2124) as a name token, the same as `ℕ`
  (one line in the `SYMBOLS` table of `src/front/lexer.c`). Parser, fmt,
  check and eval do not change for the token.
- `LEX_CHAR`: `test/parse/bad-unicode.lang` uses `ℚ` (U+211A), a
  neighbour of `ℕ` and `ℤ`, and gets `LEX_CHAR` (`unexpected byte 0xe2`).
  Before S2b, no `.lang` fixture had a non-ASCII `LEX_CHAR`.
- Domain (`domain/domain.lang`): base data `ℤ` with `pos : ℕ → ℤ` and
  `negsuc : ℕ → ℤ` (`negsuc k` is `-(k+1)`); `natDiff a b` is `pos (a -
  b)` if `b ≤ a`, else `negsuc (b - (a+1))`; `intLe` by the
  non-dependent `fold` on each argument; `counterStock r s0 s` =
  `natDiff (s0 + sumFin p) (sumFin d)`; `escrowStock r split s` =
  `natDiff (Σ split + Σ_i R i i) (Σ_i U i)`. The `Escrow` state has no
  `s0`, so the initial split is an argument.
- Print rule (`src/front/eval.c`, `int_ctor` and `print_int`): a value
  `pos k` of the domain data `ℤ` prints as `k`; `negsuc k` prints as
  `-(k+1)` (`-18446744073709551616` at `k` = 2^64 - 1). A negative value
  as an argument gets parentheses (`some (-3)`). Only `domain/domain.lang`
  can declare base data (R2), so a program cannot get this rule for its
  own data.
- Tests: `test/eval/int.lang` (18 evals): `natDiff` at 5 2, 2 2 and 2 5;
  `negsuc 0`, `pos 4`, `negsuc` at 2^64 - 1, `some (-3)`; `intLe` on the
  four constructor pairs (7 evals); `counterStock` after two concurrent
  `counterDec` of 1 at `s0` = 1 (merged: -1, the negative stock) and
  after a restock (2); `escrowStock` after a restock and a dec (8) and
  after a transfer (6).
- Gate: `make check` EXIT 0. Examples 13; parse refusals 11; check
  refusals 59; evals 133; ir 13 lowered; wasm 16; differential 1230
  cases, 0 mismatches; fmt 114 canonical, 6 non-canonical pairs; json 26
  fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture);
  gate 0 failures; review, domain and diagnostic edit regressions passed;
  cubical 43 checks, 22 round trips.
- Count changes from S2: parse refusals +1 (`bad-unicode.lang`); evals
  +18 (`int.lang`); fmt +1 canonical. The other counts are the same.
- Mutants KILLED (3): `natDiff` with `pos` and `negsuc` swapped (14
  eval failures); `intLe` at `negsuc`/`negsuc` with `natLe m n` (2
  failures); the print rule without `+ 1` (4 failures).
- Drift: `src/front/eval.c` is shared with the tcc kits. `int_ctor` and
  `print_int` are a merge-lang change. The kit copies do not have them.

- Review (2026-10-28): signed numerals are display text, not source syntax.
  `value_print` keeps the `pos` and `negsuc` constructors in source and
  diagnostics. `value_display` preserves the signed CLI display for closed
  constructor data, and keeps source syntax for functions, types and neutral
  terms, including those nested in data. Eight source round trips and six
  display round trips in `test/cubical.c` cover both signs, zero, the numeric
  boundaries, an `Option`, functions, a path type and a neutral comparison.
  The review gate passes with 43 checks and 36 round trips; other counts
  above are unchanged.

## S2 (2026-10-27)

- Scope (`docs/PLAN.md` section 9, the S2 row): the O12 replan (unit 1),
  then the domain on the S1b kernel in `domain/domain.lang`. No kernel
  edit.
- Head data (O4 DONE): `Verdict`, `Log`, `Hist`, `Step : ℕ → ℕ → Type`
  and `Read n` replace the kit sample domain. 14 fixtures moved to these
  names (`domain/README.md` gives the list).
- Library: `or`, `and`, `not`, `finEq`; `orIdem`, `orComm`, `orAssoc`,
  `natMaxIdem`, `natMaxComm`, `natMaxAssoc`; `vecMax` and `matMax`
  (pointwise `natMax`) with their laws by function extensionality.
- Parts: `GSet n = Fin n → Bool`; `Counter r` = (p, d) vectors;
  `Escrow r` = (R, U); `MVReg n r` = (m, ctx) in the dotted lex form
  (O11 TRIGGERED: the slot form has no commutative merge at a tie). Each
  part has `<part>Merge`, `<part>Idem`, `<part>Comm`, `<part>Assoc` and
  `<part>L4`. Each op is `merge s (delta s)`, so L4 follows from L2 and
  L3. An op with a precondition (`counterDec`, `escrowDec`,
  `escrowTransfer`) gives an `Option`.
- Reads: `gsetCount`, `gsetRead`, `mvregRead : MVReg n r → Read n`.
  `Read n` has `readEmpty`, `readOne` and `readConflict` (a set with a
  proof of 2 or more members). A program reads domain data with the
  non-dependent `fold`; `elim` takes only Bool or `Option`.
- Product join: `pairMerge` with `pairIdem`, `pairComm`, `pairAssoc`,
  one generic lemma for each law.
- Tests: `test/eval/parts.lang`, `test/eval/mvread.lang` and
  `test/eval/pair.lang` (33 evals: each part op and merge, the `MVReg`
  read, the product join, and three self-transfer regressions).
- Gate: `make check` EXIT 0. Examples 13; parse refusals 10; check
  refusals 59; evals 115; ir 13 lowered; wasm 16; differential 1230
  cases, 0 mismatches; fmt 113 canonical, 6 non-canonical pairs; json 26
  fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture);
  gate 0 failures; review, domain and diagnostic edit regressions passed;
  cubical 43 checks, 22 round trips.
- Count changes from S1b: evals +34 (the 33 new evals and `inventory
  added`); differential -3 (`examples/elim.lang` and
  `examples/natmax.lang` use the domain laws, `inventory` has one more
  closed entry); fmt +3 canonical. The other counts are the same.
- Mutants KILLED (7): `natAdd` in `vecMax` and in `matMax`, `and` in
  `gsetMerge` (TYPE_ENDPOINT at load); the `MVReg` clear set to 0 and
  `mvregLive` without `natLe 1 (ctx j)` (evals); `gsetRead` threshold 3
  and `pairMerge` with `ma` twice (TYPE_MISMATCH at load).
- Review fix: `escrowTransfer` refuses equal sender and recipient replicas,
  including zero transfers, so a transfer cannot mint diagonal restock.

## S1 (2026-10-27)

- Port source: evolve-lang 1f87a5a (`docs/PLAN.md` section 8). The kit
  files of merge-lang and of the evolve-lang seed 394dfe9 are byte-identical,
  so the port is one patch of 160 files. The patch changes the language name
  to merge-lang.
- Rulings (`docs/PLAN.md` section 11): (a) for Q1 to Q11 and Q17 to Q21;
  (b) for Q12 to Q16.
- O12: the (b) rulings admit `restock`, ℤ, `FMSet`, `Ops A` and the
  `Replica` cofamily. The first unit of S2 replans `docs/PLAN.md` sections 3
  to 6 and 9 for them. S1 builds none of them. The replan is DONE (S2
  unit 1, 2026-10-27).
- Gate: `make check` EXIT 0. Examples 10; parse refusals 10; check refusals
  54; evals 62; ir 10; wasm 13; differential 773 cases, 0 mismatches; fmt 101
  canonical, 6 non-canonical pairs; json 26 fixtures, diagnostics 38 codes
  (24 with a fixture, 14 with no fixture); gate 0 failures; review, domain
  and diagnostic edit regressions passed; cubical 43 checks, 22 round trips
  after review (the upstream port has 36 checks).
- Review: cell parameter and field types normalize before the R1/R2
  classification, so reducible universe and generator aliases work.
  Seven C regression cases cover aliases and preserve the refusals for
  higher kind parameters, base data and cons signatures.
- Mutants KILLED: M1 the `REFUSE_CELL` test of a CM signature (the cons
  form), M2 the `Fin` bound, M3 the `W_NONCANON` test.

## S1b (2026-10-27)

- Scope (`docs/PLAN.md` section 9, the S1b row; O15): induction in the
  kernel, so that S2 can prove the laws L1 to L3 as guest terms.
- Rulings (`docs/PLAN.md` section 11): (a) for Q22 to Q25 (the O12
  replan, S2 unit 1) and (a) for Q26 to Q28.
- `natMax`: a builtin. At open arguments, `natMax a 0 = a`,
  `natMax 0 b = b` and `natMax (a+1) (b+1) = natMax a b + 1`. The
  lowering computes `a + (b - a)`, so there is no new IR op.
- `fold` over ℕ with a motive (port of zkml-lang af57c42): when the first
  argument is a motive over ℕ, the result type is the motive at `n`.
  `natMaxIdem`, `natMaxComm` and `natMaxAssoc` in `examples/natmax.lang`
  are proofs by this fold.
- `elim P yes no b` for Bool and `elim P z s o` for `Option`: a new
  builtin with a dependent motive (port of quorum-lang 351cbf4). Its core
  is `flagIf` or `option` with a motive tag, so there is no new core op.
  Neutral normal forms retain the motive and print as `elim`. `orIdem`,
  `orComm`, `orAssoc` and `optionEta` in `examples/elim.lang` are proofs
  by `elim`.
- `sumFin n f`: a builtin. At a numeral `n` it unrolls to the sum of
  `f` at each member of `Fin n`. At an open `n` it stays stuck, and the
  lowering refuses it with REFUSE_RUNTIME_PATH.
- Refused fixtures: a motive with the wrong domain
  (`nat-motive-domain.lang`, `elim-motive-domain.lang`) or the wrong sort
  (`nat-motive-sort.lang`) gets TYPE_MISMATCH. A refl proof of
  `natMax n n = n` (`natmax-neutral.lang`) or of a `sumFin` at an open `n`
  (`sumfin-open.lang`) gets TYPE_REFL.
- Gate: `make check` EXIT 0. Examples 13; parse refusals 10; check
  refusals 59; evals 81; ir 13 lowered; wasm 16; differential 1233 cases,
  0 mismatches; fmt 110 canonical, 6 non-canonical pairs; json 26
  fixtures, diagnostics 38 codes (24 with a fixture, 14 with no fixture);
  gate 0 failures; review, domain and diagnostic edit regressions passed;
  cubical 43 checks, 22 round trips.
- Count changes from S1: examples +3 (natmax, elim, sumfin); check
  refusals +5; evals +19; ir +3 lowered and 1 refusal (`sumfin-open`);
  wasm +3; differential +460; fmt +9 canonical. The other counts are the
  same.
- Mutants KILLED: M1 drop the `natMax (a+1) (b+1)` rule; M2 drop the
  motive sort check (KILLED after `test/check/expect.txt` pins the message
  text of `nat-motive-sort.lang`); M3 `sumFin` stops one short.

## Implemented

- The Agda-like surface with the ASCII aliases (`SPEC.md` section 2).
- `langc fmt` and the warning `W_NONCANON` of `langc check`.
- `langc check --json`, with the `parsed` and `typed` counts on the last
  line. `docs/DIAGNOSTICS.md` gives the codes.
- The cubical kernel: `I`, `PathP`, `Path`, `transp`, `hcomp`, systems,
  indexed families, path constructors of dimension 1 and 2, `REFUSE_CELL` for CM
  and CC signatures, codata, the corecursor, `head` and `tail`.
- `Fin n` as the Sigma of `docs/PLAN.md` section 2.2.
- Induction (S1b): `natMax` with its rules on open arguments, a `fold`
  over ℕ with a motive, `elim` for Bool and `Option`, and `sumFin`.
- The domain (S2): `GSet`, `Counter`, `Escrow`, `MVReg`, `Read n`, the
  product join and the laws L1 to L4 as guest terms.
- ℤ (S2b): the token `ℤ`, base data `ℤ` (`pos`, `negsuc`), `natDiff`,
  `intLe`, `counterStock`, `escrowStock`, and the print rule for `ℤ`.
- S2c: the `fold` over a higher inductive type (a 1-cell case reduces, a
  2-cell case stays stuck, O52), a bottom and a left unit law for each
  part, and `FMSet` with `fmsetMap` as program data (Q30 (b)).
- S2d: the ℕ `fold` also steps at an open successor `natAdd y c` (a
  numeral `c >= 1`, Q31 (a)); `isSet` for each part by a Hedberg port and
  `mergeAll : FMSet S → S` as program data in `test/eval/mergeall.lang`
  (Q29 (a), Q30 (b)).
- S3a: the parse and `fmt` of the forms `replicas`, `atoms`,
  `replicated`, `state`, `op`, `invariant` and `allocate` (Q32 (a), Q34
  (a)).
- S3e: `langc check` elaborates the 7 forms to guest terms (Q33 (a) to
  Q38 (a)) with the codes `REFUSE_NO_MERGE`, `REFUSE_OP_INFINITE_ARG`,
  `REFUSE_RAW_WRITE` and `REFUSE_ALLOC_SUM`; `counterNonneg` and
  `escrowNonneg` in the domain.
- S3x: the gate runs `langc check` on the four examples
  `examples/*.merge`.
- S3b: the CM test admits a field `E → T A` with `E` closed data, with
  the catamorphism `fold` at the field (Q39 (a), Q40 (a)); the library
  `Ops A` and the form runner `NAMERun` (Q41 (a)); an `op` body as a
  script of reads and part operations joined by `;` (Q42 (a)); `PLe`
  and `PLeTrans` for each part (Q43 (a)).
- S4: the classes J4 to J6 and the bounded search core in C
  (`src/front/search.c`, Q48 (a)); the codes
  `REFUSE_NEEDS_COORDINATION`, `REFUSE_RULE_MISMATCH`,
  `REFUSE_READ_WINNER` and `W_BOUNDED`; the `Replica S` machine per
  form (Q45 (a)); L5 and L5r per form, with `mergeSeqAll` in the domain
  (Q46 (b), Q51 (a), Q54 (c)); L6a and L6b as domain lemmas (Q55 (a));
  L7 per allocated part (Q52 (a)); 25 builtin ℕ lemmas (Q53 (b)); the
  `laws` and `pairs` counts of `check --json`.

## Remaining M0 work

1. S5: `langc sim`: schedules, order, reduction, transcript; golden
   `test/sim/*.out` (the demo); the determinism gate (`docs/PLAN.md`
   section 9, the S5 row). It needs the USER S4 commit.
2. The later slices of `docs/PLAN.md` section 9. Each needs the slice
   before it.

## Known limits

- `langc` stops at the first error (`docs/DIAGNOSTICS.md`).
- `langc check` gives one `W_NONCANON` at most: for the first program region
  that is not canonical.
- A path constructor has dimension 1 or 2. A 3-cell gets `REFUSE_CELL`.
- Domain data has no path constructor (R1, Q30): each domain family is
  `FAMILY_BASE`. A higher inductive type is program data with a type
  argument.
- A `fold` at a 2-cell, or at a cell with a path field, stays stuck
  (O52).
- The ℕ `fold` steps at a numeral and at an open `natAdd y c` with a
  numeral `c >= 1` (Q31 (a)). At a different open ℕ, it stays stuck.
- `sumFin` at an open `n` is stuck: `langc ir` refuses it with
  `REFUSE_RUNTIME_PATH`.
- The kit names `List`, `nil` and `cons` are dropped. Use a vector
  `Fin n → A`.
- A constructor field is not dependent: `readConflict` takes one Sigma
  field (the set and the proof).
- `nonneg` compares in ℕ. The signed `stock` read gives ℤ (S2b).
- ℤ has only `natDiff` and `intLe`: there is no addition, subtraction or
  negation on ℤ. The print rule applies only to the domain data `ℤ`.
- An `op` body is one part operation or a script of reads and part
  operations on one line (Q42 (a)), with no `requires` (Q35 (a)). An
  `invariant` body is `nonneg PART` only (Q37 (a)).
- The elaboration gives no L4 term for a script op (Q43 (b)).
- The 10 form words (`replicas`, `atoms`, `replicated`, `state`, `op`,
  `invariant`, `allocate`, `by`, `escrow`, `init`) are keywords: a
  program cannot use them as names.
- The L7 proof covers the 5.2 shape (the base, one prefix op, one op per
  branch, the merge), not all schedules.
- `NAMEL5r` is stated at `NAMEReplicaOf s`, not at each machine `m`. The
  multiset step is the domain lemma `mergeSeqAll`, not a term per form
  (Q54 (c)).
- J4 reads the classes statically: an L6b pair skips the search, even
  when its ops also change a `nonneg` Counter.
- A pair that the search keeps is bounded (`W_BOUNDED`), not proved.

## Internal boundaries

`src/front/` holds the lexer, the parser, the checker (`src/front/check.c`:
the builtin table and the refusal rules) and the lowering to the IR.
`src/main.c` holds the commands of `langc`, the JSON output and the
`W_NONCANON` test. The C tests are in `test/` (`test/cubical.c`,
`test/diagnostics.c`, `test/fronttool.c`). The guest fixtures are in
`test/check`, `test/eval`, `test/fmt`, `test/ir` and `test/json`. The gate
compares the refusal, evaluation and JSON output with `expect.txt` files,
formatted text with `.canon` files, and evaluator results with Wasm.
A program can declare
CM and CC signatures that pass R1, but cannot declare base data, an axiom
or a general recursive definition (`docs/PLAN.md` section 2).
