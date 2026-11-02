# Validation

Date: 2026-11-02. tcc 0.9.28rc 2026-09-04 mob@0fb54300 builds the host and
the review tools. Apple clang 21.0.0 (clang-2100.0.123.102) runs
`check-clang`. Node v23.10.0 runs the Wasm modules of the differential and
engine legs. Host executable: `build/langc` at ebbc4be, with the S10
working tree.

`make -B check` passes. It builds the host and the review tools with tcc,
compiles the sources again with clang (`-std=c99 -Wall -Wextra
-Wswitch-enum -Werror -fsyntax-only`, `check-clang`), and runs
`test/gate.sh`, `test/review.sh`, `build/fronttool`, `build/diagnostics`,
`build/cubical`, `build/sim`, `build/report`, `build/skill-test`,
`build/verbs-test`, `build/mcp-test`, `build/readme-test`, the token check
shown below and `build/tokcount-io`. The final run
gives these 27 count lines:

```
examples: 13 checked, 0 failed
parse refusals: 23 checked
check refusals: 76 checked
evals: 206 checked
ir: 13 lowered
wasm: 16 modules valid
differential: 1230 cases, 0 mismatches
engine: 6 modules, 0 mismatches
engine: 7 forms, 14581 schedules, 320 refusals, 281813 calls, 0 mismatches
fmt: 163 canonical, 14 non-canonical pairs
merge examples: 4 canonical, 3 checked, 1 refused
sim: 4 golden, 0 mismatches
report: 5 golden, 0 mismatches, 4 same as check --json
json: 35 fixtures; diagnostics: 47 codes, 32 with a fixture, 15 with no fixture
gate: 0 failures
review regressions: passed
domain regressions: passed
diagnostic edit regressions: passed
cubical regressions: 124 checks, 41 round trips passed
sim regressions: 8 checks
report rows: 7 checks
skill: 5 examples, 5 outputs match
explain-code: 47 codes, 0 mismatches
mcp: 33 requests, 0 mismatches; 5 examples
readme: 7 rows, 9 verbs
tokens: 3 files, 2 rules, 4 ratios, 0 mismatches
tokens IO regressions: 5 cases, 0 failures
```

The gate runs 1692 tests with zero failures. N is the sum of the first
number of each count line that counts cases (Q102 (a)). A count line
counts cases when the word after its first number is `checked`, `checks`
or `cases`. These 9 lines count cases: examples 13, parse refusals 23,
check refusals 76, evals 206, differential 1230, cubical regressions 124,
sim regressions 8, report rows 7 and tokens IO regressions 5. The final
gate run: 302 s (2026-11-02).

The other count lines are not in N. Their first number counts modules,
forms, files, fixtures, codes, requests or list items, not cases: `ir`
(the modules that `langc ir` lowers), `wasm` (the valid modules), the two
`engine` lines (the modules, and the forms; the schedules, the refusals
and the calls run in these forms), `fmt` (the canonical files and the
non-canonical pairs), `merge examples` (the canonical `.merge` files),
`sim` and `report` (the golden files), `json` and `diagnostics` (the
fixtures and the codes), `skill` (the examples), `explain-code` (the
codes), `mcp` (the requests), `readme` (the rows and the verbs) and
`tokens` (the files, the rules and the ratios). `gate: 0 failures` counts
failures. The 41 round trips of the cubical line are not in N. Three lines
have no number: `review regressions: passed`, `domain regressions: passed`
and `diagnostic edit regressions: passed`.

## Differential and engine checks

The Wasm differential (the S1 port of evolve-lang): `test/gate.sh` builds a
module with `langc build` for each example and each `test/ir` program, and
`wasm-validate` checks each module. The differential makes a call grid
from `langc ir` with `test/grid.awk`. It runs each call through `langc
eval` and through the module with `node test/run-wasm.mjs`, and compares
the two results. A `pick 2 4 9` call on `entries.wasm` must trap.

The engine (S7): for each golden file `test/engine/NAME.abi`,
`test/gate.sh` builds the module with `langc build`, `wasm-validate`
checks it, and `langc abi` must give the bytes of the golden file
(`engine: 6 modules`). The engine differential (`engine: 7 forms`) runs
each simulator schedule of each form through the Wasm merge engine with
`node test/run-wasm.mjs` and through the guest values, and compares the
two results.

The simulator (S5): the determinism gate of `test/gate.sh` compares the
transcript of `langc sim` with the golden file `test/sim/NAME.out`. The
reports (S6): `test/gate.sh` compares each report with the golden file
`test/report/NAME.report.json`, and checks that 4 reports are the same as
`langc check --json`. `build/report` checks the report rows.

## Token baseline

`docs/TOKEN-BASELINE.md` (S9) gives the bytes, the code points and the
tokens of the inventory example in 3 files: `examples/inventory-escrow.merge`,
`test/baseline/inventory.py` and `test/baseline/inventory.rs`, under 2
rules, and 4 ratios against the `.merge` file. The gate runs:

```sh
build/tokcount --check docs/TOKEN-BASELINE.md examples/inventory-escrow.merge test/baseline/inventory.py test/baseline/inventory.rs
```

It compares the table with the file region (`tokens: 3 files, 2 rules,
4 ratios, 0 mismatches`).
`build/tokcount-io` then runs its 5 CLI cases.

## Slices

The counts come from the PLAN section 9 DONE text of each slice. A count
gives the value before the slice and the value after it. A mutant count
gives the mutants, the killed mutants and the survivors.

S0: the tcc-wasm kit seed of lang-template, `design/DESIGN.md`,
`docs/PLAN.md`, README, `AGENTS.md` and `docs/COMPARISON.md`; the seed
gate has 10 examples, 9 parse refusals, 19 check refusals, 32 evals and
813 differential cases with 0 mismatches.

S1: the port of evolve-lang 1f87a5a: the surface, `langc fmt`,
`W_NONCANON`, `check --json` and the cubical kernel. The cases are the
evolve-lang cases: 10 examples, 10 parse refusals, 54 check refusals, 62
evals, 13 wasm modules, 773 differential cases with 0 mismatches, 101
canonical fmt files with 6 non-canonical pairs, 26 json fixtures and 38
diagnostic codes (24 with a fixture). The review adds 7 C classification
cases (cubical checks 36 to 43). Mutants: 3 / 3 / 0.

S1b: induction in the kernel (O15): the `natMax` rules on open arguments,
a `fold` over ℕ with a motive, the dependent `elim` for Bool and `Option`,
and the builtin `sumFin`. Examples 10 to 13, check refusals 54 to 59,
evals 62 to 81, wasm 13 to 16, differential 773 to 1233, fmt 101 to 110.
`test/ir/sumfin-open.lang` gets `REFUSE_RUNTIME_PATH`, and
`test/check/expect.txt` pins the message of `nat-motive-sort.lang`.
Mutants: 3 / 3 / 0.

S2: the O12 replan, then the parts in `domain/domain.lang`: GSet, MVReg,
`Read`, Counter, Escrow with `restock`, the product join, and L1 to L4 as
guest terms. Evals 81 to 115, differential 1233 to 1230, fmt 110 to 113.
The review adds 3 regression evals that reject an escrow self transfer.
O4 is DONE; O11 TRIGGERED, and S2 took the fallback (the dotted `MVReg`).
Mutants: 7 / 7 / 0.

S2b: ℤ with no kernel work: the lexer token `ℤ`, the base data `pos` and
`negsuc`, `natDiff`, `intLe` and the `stock` read. Parse refusals 10 to 11
(`test/parse/bad-unicode.lang` gets `LEX_CHAR` for `ℚ`), evals 115 to 133
(18 ℤ evals in `test/eval/int.lang`), fmt 113 to 114. Mutants: 3 / 3 / 0.

S2c: path constructors of dimension 2 in the kernel (the quorum-lang
09f7f59 port), the `FMSet` generator form, `fold` over a HIT and
`fmsetMap`. Evals 133 to 146 (`test/eval/cell-path-dim.lang`,
`test/eval/hit.lang`, `test/eval/fmset.lang`), fmt 114 to 117, cubical
round trips 22 to 36. A fixture of dimension 3 gets `REFUSE_CELL`.
Mutants: 4 / 3 / 1 (M3a survives).

S2d: `isSet` for each part (the Hedberg port) and `mergeAll : FMSet S → S`
on the program side. Evals 146 to 152 (`test/eval/mergeall.lang`,
`test/eval/fold-open.lang`), fmt 117 to 119. The mutants fail at load with
`TYPE_MISMATCH`, `TYPE_ENDPOINT` or `TYPE_REFL`. Mutants: 4 / 4 / 0.

S3a: parse and fmt of the 7 forms (replicas, atoms, replicated, state, op,
invariant, allocate), the lexer token `∣` and 4 `.merge` examples. Parse
refusals 11 to 22 (11 form refusals), check refusals 59 to 60
(`test/check/form.lang`, `REFUSE_FORM`), fmt 119 to 127 with pairs 6 to 12
(`test/fmt/form-*`), a new loop with 4 canonical `.merge` examples, json
26 to 27, diagnostics 38 to 39, cubical 43 to 46. Mutants: 11 / 11 / 0.

S3e: the 7 forms become guest terms on the S2 parts and `mergeAll`, with
`NAMEValue`; an op body is one part op. Check refusals 60 to 70 (for
example `op-fin-open`, `alloc-under`, `alloc-missing`, `op-arity`), evals
152 to 173 (`test/eval/replicas.lang`, `shop.lang`, `escrow.lang`), fmt
127 to 143, json 27 to 30, diagnostics 39 to 42. The new codes
`REFUSE_NO_MERGE`, `REFUSE_OP_INFINITE_ARG`, `REFUSE_RAW_WRITE` and
`REFUSE_ALLOC_SUM` each have a `test/json` fixture; `REFUSE_FORM` and its
fixture go. Mutants: 14 / 14 / 0 (4 of them after new fixtures).

S3x: the 4 examples parse and type; the `.merge` loop also runs `check`
(`4 canonical, 4 checked`); cubical 46 to 63. No new test file. Mutants: 4
/ 4 / 0, the example mutants that the loop kills (`REFUSE_RAW_WRITE`,
`REFUSE_NO_MERGE`, `REFUSE_ALLOC_SUM`). The text names 2 more mutants that
other tests also kill, and a swap of `n` and `r` in `mvregWrite` that
survived at S3x (its MVReg tests had 2 atoms and 2 replicas). S3b added
`test/eval/owner3.lang` with 3 atoms and 2 replicas to catch that swap.

S3b: the CM test admits a field `E → T A` with a closed `E`, the library
`Ops A`, an op body as an Ops script, and L4 by composition. Check
refusals 70 to 75 (`cell-arrow-self`, `cell-arrow-two`, `read-counter`,
`stock-mvreg`, `le-order`), evals 173 to 200 (`test/eval/ops-lib.lang`,
`ops.lang`, `owner3.lang`, `script.lang`, `script-dec.lang`, `le.lang`),
fmt 143 to 155 with pairs 12 to 13. Mutants: 10 / 10 / 0.

S4: the checker judgements J4 to J6, the core of the bounded search, the
laws L5 to L8 and the cofamily `Replica S`. Parse refusals 22 to 23, check
refusals 75 to 76, evals 200 to 206 (`test/eval/machine.lang`), fmt 155 to
163 with pairs 13 to 14, `.merge` examples `4 canonical, 3 checked, 1
refused`, json 30 to 35, diagnostics 42 to 46, cubical 63 to 122. The new
codes `REFUSE_NEEDS_COORDINATION`, `REFUSE_RULE_MISMATCH`,
`REFUSE_READ_WINNER` and `W_BOUNDED` each have a json fixture; the pair
laws have `test/json/pair-law.lang`. Mutants: 12 / 12 / 0.

S5: `langc sim` (schedules, order, reduction, transcript). A new line
`sim 2 golden, 0 mismatches` (`test/sim/inventory.out`, 33 schedules, rc
1; `test/sim/inventory-escrow.out`, 620 schedules, rc 0); cubical 122 to
124, round trips 38 to 41. Mutants: 6 / 4 / 2 (O2 and N3 survive).

S6: the JSON report (the `json.c` port, `NAME.report.json`, the PLAN 5.3
schema that `check --json` also uses). Sim 2 to 4 golden (`owner.out`,
`tags.out`), and new lines for 5 golden reports in `test/report/` (4 the
same as `check --json`), 7 sim regression checks and 7 report row checks.
No diagnostic code changes. Mutants: 8 / 8 / 0.

S7: the Wasm engine (the writer changes of PLAN 5.1, the part templates,
the lowered invariants, the engine ABI). New lines `engine: 4 modules, 0
mismatches` (`test/engine/*.abi`) and `engine: 5 forms, 14419 schedules,
206 refusals, 279146 calls, 0 mismatches` (`test/engine.c`,
`test/run-wasm.mjs`). Mutants: 5 / 5 / 0, all killed by the engine leg.

S8: the agent kit (`skills/merge-lang/SKILL.md` with 5 graded examples,
`langc mcp`, `explain-code`, `AGENTS.md`, the README table). New lines for
5 skill outputs, 46 `explain-code` codes, 33 mcp requests and 7 README
rows with 9 verbs (`test/skill.c`, `test/verbs.c`, `test/mcp.c`,
`test/readme.c`). Mutants: 8 / 8 / 0.

S9: `docs/TOKEN-BASELINE.md` and its gate leg (`test/tokcount.c`, the
evolve-lang dd98209 port, and `test/tokcount-io.c`). 27 count lines;
diagnostics 46 to 47 (`CHECK_DEPTH`, no fixture), `explain-code` 46 to 47,
and new lines for 3 token files with 4 ratios and 5 token IO cases.
Mutants: 6 / 6 / 0.

## What the gate does not check

- PLAN 6.2 (outside the denotation): network loss, duplication beyond
  idempotence and order beyond the schedules of PLAN 5.2; durability (a
  replica that restarts from an old state can spend rights twice, and L7
  does not cover it); Byzantine replicas; clocks; size (counters grow
  without bound in ℕ, and the engine traps at `i64` overflow, O7).
- The bounded mark: "kept on all K schedules" is a fact about K
  schedules, not a proof for all schedules. The bounded search is a
  refuter, not a prover (O10).
- The L7 guest proof covers the bottom state, one prefix operation and
  one operation on each of two distinct replica branches, then a merge.
  It is not an induction over arbitrary histories. L5r starts at
  `NAMEReplicaOf s`; comparing receive orders requires the same seed
  state `s`. The checker has no input for user-supplied L8 proof terms.
- The open items that S10 carries (PLAN section 10): O2 (the HN thread is
  not found; the docs cite "an HN discussion (2024)"), O3 (no wasmtime
  leg), O5 (`Glue` and `ua` are not ported; no law needs them), O6 (a
  user merge function with proofs), O7, O8 (no S1 list of the evolve-lang
  O10 to O18 was found), O9 (the Automerge facts of 2026-10-27; check
  them before a public README) and O10.
- The 3 historical survivors in the counted runs (docs/STATUS.md
  `## Mutant summary`): S2c M3a (an
  end at an interval variable does not show in a printed value), S5 O2
  (no golden file has a 2-argument op, Q58 (b)) and S5 N3 (no golden file
  has 3 replicas).
- No Python or Rust run: the gate counts the tokens of `inventory.py` and
  `inventory.rs`, but it does not run them.
- The law map below: no gate leg checks its ids, its paths or its
  statements (Q101 (a)). S10 checked each path by hand with `ls`.

## Laws and judgements

This map gives each law of PLAN 6.1 and each judgement of PLAN 4.3, and the
files and the gate lines that prove or test it.

| id | statement | files | gate |
|---|---|---|---|
| L1 | `merge a b ≡ merge b a` for each part and the product. | `domain/domain.lang` (`orComm`, `natMaxComm`, `gsetComm`, `counterComm`, `escrowComm`, `mvregComm`, `pairComm`), `test/eval/pair.lang`, `test/eval/mergeall.lang` | `evals` |
| L2 | `merge (merge a b) c ≡ merge a (merge b c)`. | `domain/domain.lang` (`orAssoc`, `natMaxAssoc`, `gsetAssoc`, `counterAssoc`, `escrowAssoc`, `mvregAssoc`, `pairAssoc`), `test/eval/pair.lang`, `test/eval/mergeall.lang` | `evals` |
| L3 | `merge a a ≡ a`. | `domain/domain.lang` (`orIdem`, `natMaxIdem`, `gsetIdem`, `counterIdem`, `escrowIdem`, `mvregIdem`, `pairIdem`), `test/eval/pair.lang`, `test/eval/mergeall.lang` | `evals` |
| L4 | Inflation: `apply i o s ≡ some t → le s t`. | `domain/domain.lang` (`gsetL4`, `gsetAddL4`, `counterL4`, `counterDecL4`, `counterRestockL4`, `escrowL4`, `escrowDecL4`, `escrowTransferL4`, `escrowRestockL4`, `mvregL4`, `mvregWriteL4`), `test/eval/le.lang`, `test/check/le-order.lang` | `evals`, `check refusals` |
| L5 | From a common seed `s`, merging the same multiset of states in either order gives equal states. `mergeSeqAll` relates the sequence to `merge s (mergeAll bag)`. | `domain/domain.lang` (`mergeSeqAll`), `test/eval/mergeall.lang`, `test/eval/fmset.lang`, `test/sim/inventory.out` | `evals`, `sim` |
| L5r | `NAMEL5r` relates receives at `NAMEReplicaOf s` to `mergeSeq` from `s`. Equal receive multisets give equal `eps` from the same seed, via the abstract domain lemma `mergeSeqAll`. | `src/front/check.c` (`NAMEL5r` for each form), `domain/domain.lang` (`mergeSeqAll`), `test/eval/machine.lang`, `test/eval/replicas.lang` | `merge examples`, `evals` |
| L6a | `GSet`: after a merge of `add x a` and `add y b`, `has` gives true for `x` and for `y`. | `domain/domain.lang` (`gsetHasMergeL`, `gsetHasMergeR`), `test/eval/parts.lang` | `evals` |
| L6b | From a common base with `mvregWf`, writes by distinct replicas of `x ≠ y` retain both values and have at least two live values, so the read is a conflict. | `domain/domain.lang` (`mvregConflict`, `mvregConflictHasL`, `mvregConflictHasR`), `test/eval/mvread.lang`, `test/eval/owner3.lang`, `examples/owner.merge` | `evals`, `merge examples` |
| L7 | `Escrow`: `escrowL7` proves `nonneg` after one prefix operation from bottom, one operation per distinct replica branch and a merge, under `EscrowStepOk` ownership and rights assumptions. Longer histories have no guest induction proof. | `domain/domain.lang` (`escrowNonneg`, `escrowNonnegSum`, `escrowL7`, `EscrowL7Law`), `src/front/check.c` (`NAMEPARTL7`), `test/eval/escrow.lang`, `examples/inventory-escrow.merge` | `evals`, `merge examples` |
| L8 | A pair outside the library covers is searched for an invariant violation; a kept pair is bounded. User-supplied proof terms are planned, with no implemented input. | `src/front/search.c`, `src/front/check.c`, `examples/tags.merge`, `test/report/classes.merge` | `merge examples`, `report` |
| J1 | Well-formed declaration: library parts, finite argument types, an `Escrow` split that sums to its stock. | `test/check/alloc-sum.lang`, `test/check/alloc-missing.lang`, `test/check/alloc-twice.lang`, `test/check/alloc-under.lang`, `test/json/REFUSE_NO_MERGE.lang`, `test/json/REFUSE_OP_INFINITE_ARG.lang`, `test/json/REFUSE_ALLOC_SUM.lang` | `check refusals`, `json` |
| J2 | Supported operation scripts compose part operations with their part preconditions. Declared `requires` predicates are not implemented. | `test/eval/ops.lang`, `test/eval/ops-lib.lang`, `test/eval/script.lang`, `test/json/REFUSE_RAW_WRITE.lang`, `test/json/REFUSE_READ_WINNER.lang` | `evals`, `check refusals`, `json` |
| J3 | Merge laws: L1 to L4 hold for each part and for the product (library proofs). | `domain/domain.lang`, `test/eval/pair.lang` | `evals` |
| J4 | Class of an operation pair: explicit conflict, needs coordination or independent. | `src/front/check.c`, `examples/owner.merge`, `examples/inventory.merge`, `examples/tags.merge`, `test/json/REFUSE_NEEDS_COORDINATION.lang`, `test/report/classes.merge` | `merge examples`, `json`, `report` |
| J5 | Bounded I-confluence search: the first violation in the order of PLAN 5.2 is the minimal counterexample. | `src/front/search.c`, `test/sim.c`, `test/sim/inventory.out`, `test/report.c` | `sim`, `sim regressions`, `report rows` |
| J6 | Allocation rule: `allocate c by escrow (...)` covers only `nonneg c`; its generated proof has the scope of L7 above. | `examples/inventory-escrow.merge`, `test/eval/escrow.lang`, `test/check/alloc-sum.lang`, `test/json/REFUSE_RULE_MISMATCH.lang` | `merge examples`, `evals`, `json` |
