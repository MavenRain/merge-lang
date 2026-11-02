# merge-lang status

The build implements slices S1, S1b, S2, S2b, S2c, S2d, S3a, S3e, S3x,
S3b, S4, S5, S6, S7, S8 and S9 and the close S10 of `docs/PLAN.md`
section 9. The plan is CLOSED (Q105 (a)). The slices: the port of the evolve-lang 1f87a5a
surface and cubical kernel (S1), induction in the kernel (S1b), the
merge-lang domain with the laws L1 to L4 as guest terms (S2), ℤ with the
`stock` read (S2b), path constructors of dimension 2 with the `fold` over
a higher inductive type (S2c), `isSet` for each part with `mergeAll` on
the program side (S2d), the parse and `fmt` of the merge forms (S3a), the
elaboration of the forms to guest terms (S3e), the check of the four
examples in the gate (S3x), an `op` body as an `Ops` script of part
reads and part operations (S3b), the checker: the classes, the bounded
search, the coordination codes and the laws L5 to L8 (S4), the
replica simulator `langc sim` with golden transcripts and a determinism
gate (S5), the JSON coordination report of `check --json` and `build`
(S6), the Wasm merge engine that `build` writes, with a differential
gate against the guest on each simulator schedule (S7), and the agent
kit: `SKILL.md` with 5 graded examples, `langc explain-code` and the
stdio MCP server `langc mcp` (S8), and the token baseline
`docs/TOKEN-BASELINE.md` with its gate leg (S9). The close (S10) writes
`docs/VALIDATION.md` and the mutant summary.

## S10 (2026-11-02)

- Scope (`docs/PLAN.md` section 9, the S10 row; Q98 (a) to Q106 (a),
  Q-S10-s2-1 (a)): the close of the plan. No `langc` code changes and no
  gate leg is added. The base is the USER S9 commit ebbc4be.
- Units: U0 the extracts and the rulings; U1 `docs/VALIDATION.md` (the
  versions, the gate counts, N, the differential and engine checks, the
  laws and judgements map, the slices); U2 this file (the head, this
  section, `## Mutant summary`, `## Remaining work`, two Known limits
  lines); U3 `docs/PLAN.md` (the S10 row, sections 10 and 11, the head)
  and the README status; U4 the final gate and the stage.
- The gate counts are the same as the S9 counts: the 27 count lines of
  the final gate are equal to the S9 count file (`cmp` rc 0). N = 1692
  (`docs/VALIDATION.md` gives the sum and its lines; Q102 (a)).
- No mutant runs in S10. `## Mutant summary` gives the mutants of S1 to
  S9 from the PLAN section 9 DONE text (Q99 (a)).
- O item checks (PLAN section 10, Q103 (a)): O11 DONE (S2 took the
  fallback, the dotted `MVReg`). O14 DONE (Q-S10-s2-1 (a)): the PLAN 6.1
  L7 row gives `restock` as `R i i` and the mark proved (library), with
  the domain lemmas `escrowRestockL4` and `escrowL7` and the S2b text of
  `escrowStock`. The S4 text in this file does not name `R i i`. O8
  stays open: no S1 list of the evolve-lang O10 to O18 was found.
- D-s5-1 is closed (Q104 (a)): the USER S9 commit ebbc4be accepts it.
- The final gate: `make -B check` EXIT 0 with no FAIL line, no
  `build/merge-tokens-*` file left. `docs/VALIDATION.md` gives the
  seconds and the date.

## Mutant summary

The counted mutation runs of S1 to S9, from the PLAN section 9 DONE
text, and for S1 to S2d from the slice sections of this file. These are
historical results at each slice, not reruns against S10. No mutant
runs in S10.

| Slice | Files | Mutants | Killed | Survivors | Kill tests |
|---|---|---|---|---|---|
| S1 | not given | 3 | 3 | 0 | the gate (test not named) |
| S1b | not given | 3 | 3 | 0 | M1, M3 the gate (test not named); M2 `nat-motive-sort.lang` (`test/check/expect.txt`) |
| S2 | not given (`domain/domain.lang` terms) | 7 | 7 | 0 | the gate (test not named): 3 at load, 2 evals, 2 at load with `TYPE_MISMATCH` |
| S2b | `domain/domain.lang`, `src/front/eval.c` | 3 | 3 | 0 | the gate (test not named): 14, 2 and 4 eval failures |
| S2c | not given | 4 | 3 | 1 (M3a) | M1, M2, M3b the gate (test not named): 13, 1 and 6 failures |
| S2d | not given | 4 | 4 | 0 | M1 `TYPE_MISMATCH` at `mergeAll`; M2a `TYPE_ENDPOINT` at `fN`; M2b `TYPE_REFL` at `predTwoLaw`; M2c `TYPE_MISMATCH` at `absurdZS` |
| S3a | not given | 11 | 11 | 0 | `atoms-mix-nat`; `atoms-mix-name`; `replicated-column`; `state-init-name`; `share-no-mapsto`; the form pairs; `form-replicated`, `inventory-escrow.merge`; `form-atoms-*`; `form.lang` (3) |
| S3e | not given | 14 | 14 | 0 | `REFUSE_NO_MERGE.lang`; json, `form-replicated`; `op-fin-open`; eval `escrow`; json column; `alloc-sum`; `alloc-under`; `alloc-missing`; `raw-write`, eval `shop`; `op-arity`; eval `replicas`; eval `shop` |
| S3x | `examples/tags.merge`, `owner.merge`, `inventory.merge`, `inventory-escrow.merge` | 4 | 4 | 0 | the `.merge` loop (`REFUSE_RAW_WRITE` 2, `REFUSE_NO_MERGE`, `REFUSE_ALLOC_SUM`) |
| S3b | not given | 10 | 10 | 0 | M1 `cell-field`; M2 `cell-arrow-two`; M3b eval `ops`; M4, M5 the gate (test not named); M6 eval `script`; M7 `stock-mvreg`, eval `shop`; M8b `raw-write`; M9a eval `script`; M9b eval `script-dec` |
| S4 | not given | 12 | 12 | 0 | C1, C2 `pair-law.lang`; C3 `inventory-escrow.merge` (`W_BOUNDED`); C4 `REFUSE_NEEDS_COORDINATION.lang`; N1 row `natLeTrans`; L1 the gate (test not named); L2 `ShopStockL7`; R1 `REFUSE_RULE_MISMATCH.lang`; F1 `allocate-for.canon`; O1 `REFUSE_READ_WINNER.lang`; V1 the merge loop, json; U1 the json progress line |
| S5 | not given | 6 | 4 | 2 (O2, N3) | O1, O3, N1, N2 `test/sim/inventory.out` |
| S6 | not given | 8 | 8 | 0 | K1 to K8 the golden reports (`classes`, `owner`, `tags`, `inventory`, `inventory-escrow`) |
| S7 | not given | 5 | 5 | 0 | the engine leg: E1 tags, classes; E2 inventory; E3 inventory-escrow; E4 owner, classes; E5 inventory |
| S8 | not given | 8 | 8 | 0 | M1 to M3, M8 mcp; M4 verbs; M5, M6 skill; M7 readme |
| S9 | `test/tokcount.c` (M1, M3 to M6), `docs/TOKEN-BASELINE.md` (M2) | 6 | 6 | 0 | M1 to M5 tokens; M6 tokens IO |
| total | | 108 | 105 | 3 | |

The 3 survivors in the counted runs:

- S2c M3a (the wrong end in the `fold` at a 1-cell): an end at an
  interval variable does not show in a printed value.
- S5 O2 (`next_args`): no golden file has a 2-argument op (Q58 (b)).
- S5 N3 (the class row pair): no golden file has 3 replicas.

The S3x DONE text also names two killed code mutants and a swap of `n`
and `r` in `mvregWrite` that survived at S3x (its MVReg tests had 2 atoms
and 2 replicas). Those three runs are outside the four counted S3x
example mutants, so the table excludes those two kills and one historical
survivor.
S3b later added `test/eval/owner3.lang`, with 3 atoms and 2 replicas,
to catch the `n`/`r` swap. The historical totals do not measure the
current tree's mutation coverage.

## S9 (2026-11-02)

- Scope (`docs/PLAN.md` section 9, the S9 row; Q88 (a), Q90 (a) to Q97
  (a)): the token baseline `docs/TOKEN-BASELINE.md` and its script. The
  base is the USER S8 commit 7bbcd58. The port source is evolve-lang
  dd98209 (PLAN section 8, Q96 (a)).
- New files: `test/tokcount.c` (the token count tool, Q90 (a)),
  `test/tokcount-io.c` (its C CLI regressions) and
  `docs/TOKEN-BASELINE.md`. The two samples `test/baseline/inventory.py`
  and `test/baseline/inventory.rs` (Q94 (a)) hold the full model of the
  demo task `examples/inventory-escrow.merge` (Q91 (a)) and its tests:
  the merge laws and the invariant over the bounded domain of the file
  (Q92 (a)).
- The table of `docs/TOKEN-BASELINE.md` (rules A and B, Q90 (a); the
  `non-ASCII tokens` column and two share lines, Q93 (a), so no lexer
  change). The ratios over the merge file: Python 10.20 (A) and 9.71
  (B), Rust 18.90 (A) and 16.87 (B). The non-ASCII share of the merge
  file: 0.11 (A) and 0.08 (B).

| file | bytes | code points | tokens A | tokens B | non-ASCII tokens |
|---|---:|---:|---:|---:|---:|
| examples/inventory-escrow.merge | 312 | 306 | 83 | 110 | 9 |
| test/baseline/inventory.py | 3071 | 3071 | 847 | 1068 | 0 |
| test/baseline/inventory.rs | 5345 | 5345 | 1569 | 1856 | 0 |

- Gate leg (Q95 (a)): the Makefile `check` recipe runs `build/tokcount
  --check docs/TOKEN-BASELINE.md` on the 3 files and
  `build/tokcount-io`; `check-clang` compiles both C files. Count lines:
  `tokens: 3 files, 2 rules, 4 ratios, 0 mismatches` and `tokens IO
  regressions: 5 cases, 0 failures`.
- U4 mutants: 6/6 KILLED, `make -B` for each build. After each mutant,
  the tree is restored and `cmp` gives rc 0. Each `check` leg mutant (M1
  to M5) also makes `build/tokcount-io` fail (`check exits 1, expected
  0`).

| # | Mutant | Kill |
|---|---|---|
| M1 | `end_run` with `(run + 2) / 4` | tokens: doc line 90 (tokens B 87, not 110) |
| M2 | the merge row of the doc with 84 tokens A, not 83 | tokens: doc line 90 |
| M3 | the ratio with no half-up term | tokens: `201 over 200 gives 1.00, not 1.01` and doc line 95 (9.70) |
| M4 | `build/langc chek` for `build/langc check` | tokens: the check of `examples/inventory-escrow.merge` exits 2, not 0 |
| M5 | no non-ASCII count | tokens: doc line 90 (the last column 0, not 9) |
| M6 | `output_status` gives `status` only | tokens IO: `table` and `check` with an unwritable stdout exit 0, not 2 |

- Q88 (a): the CHECK_DEPTH row (error, check, `The term nests deeper
  than the checker limit of 2000 levels.`) is after TYPE_LINE in
  `docs/DIAGNOSTICS.md`, with a `no fixture:` reason (the parser refuses
  a term deeper than 1000 levels, `PARSE_DEPTH`). `test/gate.sh` adds
  CHECK to its prefix list. `src/codes.c` holds the rows of
  `docs/DIAGNOSTICS.md` in the same order, and `test/verbs.c` compares
  the two tables cell for cell. Thus the row is also in `src/codes.c`,
  and the `src/codes.h` comment gives 47 codes. These are the only agent
  kit changes. `src/front/check.c` already emits CHECK_DEPTH; S9 adds
  only its row.
- Q97 (a): SPEC.md 2.2 lists `→`, `λ`, `×`, `ℕ`, `ℤ`, `∧`, `∨`, `↦` and
  `∣` (`src/front/lexer.c`: `∣` is TOK_BAR, `ℤ` is TOK_IDENT). The
  LEX_CHAR row of `docs/DIAGNOSTICS.md` and of `src/codes.c` adds `ℤ`.
- Port notes:
  - The evolve `test/tokcount-io.c` writes its NUL document to
    `/tmp/evolve-tokens-XXXXXX`. The sandbox refuses writes to `/tmp`,
    so the port uses `build/merge-tokens-XXXXXX` (`build/` is in
    `.gitignore`).
  - The count filter `^[a-z][a-z -]*: ` does not match `tokens IO
    regressions: ...`. The count filter of the gate runs is
    `^[a-zA-Z][a-zA-Z -]*: `.
- No change to the checker, the simulator, the report or the engine.
- `make check` EXIT 0 (final gate): the S8 counts are the same (25
  lines), but `json: 35 fixtures; diagnostics: 47 codes, 32 with a
  fixture, 15 with no fixture` and `explain-code: 47 codes, 0
  mismatches`. There are two new lines: `tokens: 3 files, 2 rules, 4
  ratios, 0 mismatches` and `tokens IO regressions: 5 cases, 0
  failures`.

## S8 (2026-11-01)

- Scope (`docs/PLAN.md` section 9, the S8 row; Q77 (a) to Q87 (a), Q89
  (a)): the agent kit of PLAN section 7, rows 3, 4 and 5. The base is
  the USER S7 commit ff5720b. The port source is evolve-lang 8c5d8e1
  (PLAN section 8, Q87 (a)).
- U1 (Q81 (a) to Q83 (a)): `skills/merge-lang/SKILL.md` has 5 graded
  examples from easy to hard: tags (independent), task owner (explicit
  conflict), inventory (refused, counterexample), inventory with escrow
  (kept, proved) and seats. Examples 1 to 4 cite
  `examples/{tags,owner,inventory,inventory-escrow}.merge`. Example 5 is
  the new `examples/agent/seats.merge`, outside the gate globs: 3
  replicas, the form `Hall` (`sell` and `give`, a `transfer` to C, an
  escrow rule) and the form `Door` (`sell` with no rule, and `tag`).
  `check --json` refuses the `Door` sale and gives its counterexample
  row. The example text names the transfer that the sale needs from
  that row (Q83 (a); the checker does not change). Each example has the
  full stdout of `check --json PATH`, byte for byte, and `The exit code
  is N.` (Q82 (a)). `test/skill.c` runs each example and compares the
  stdout and the exit code with the file. Count line: `skill: 5
  examples, 5 outputs match`.
- U2 (Q80 (a)): `langc explain-code CODE` prints one JSON object with
  the row of CODE in `docs/DIAGNOSTICS.md` (`code`, `severity`, `stage`,
  and the Meaning cell verbatim as `meaning`). A code with no row gives
  rc 1. The table is compiled into `src/codes.{c,h}` (46 codes).
  `test/verbs.c` compares the table with `docs/DIAGNOSTICS.md` in both
  directions. Count line: `explain-code: 46 codes, 0 mismatches`.
- U3 (Q77 (a), Q78 (a), Q84 (a), Q85 (a), Q89 (a)): `langc mcp` is a
  stdio JSON-RPC MCP server in C that tcc builds: `src/mcp.{c,h}` and
  the JSON reader `src/front/jsonin.{c,h}`. `initialize` gives the
  protocol revision 2025-11-25 and serverInfo `merge-lang` `S8` (Q85
  (a)). It needs `protocolVersion` in params, as the port; else the
  error is -32602. The tools are `check` (the S6 report document),
  `fmt`, `sim` and `explain-code` (Q77 (a)). `sim` takes `path` and the
  optional JSON integers `replicas`, `prefix` and `ops` with the CLI
  bounds; a value that is not an integer gives -32602 (Q78 (a)). Each
  tool call goes through `parse_options` with a CLI argv, so a bad tool
  argument gives the same USAGE diagnostic and usage text as the CLI.
  Q89 (a): `sim_state`, `sim_step` and `sim_print` in
  `src/front/check.c` write to a `FILE *dst`, and `run_sim` gives its
  OUT, so the transcript does not go to the JSON-RPC stdout of `langc
  mcp`. `test/mcp.c` makes its own temp directory, sends
  `test/mcp/requests.jsonl` and compares the replies with
  `test/mcp/responses.jsonl`, sends `check` and `sim` for each graded
  example and compares the result with the CLI, and checks that an
  output write error gives exit 2 (Q84 (a)). Count line: `mcp: 33
  requests, 0 mismatches; 5 examples`.
- U4 (Q86 (a)): `AGENTS.md` has `## Status (S8)`, names the kit verbs
  `explain-code` and `mcp`, also `eval`, `ir` and `abi`, and points to
  SKILL.md. README has `## Status (S8)`, SKILL.md and `examples/agent/`
  in `## Files`, and the "When to use merge-lang" table with a Parts
  column (`GSet`, `MVReg`, `Counter`, `Escrow`). `test/readme.c` (the
  port of evolve-lang `test/readme.c`) checks the table: a `yes` row
  names an admitted part of PLAN 3.2, a `no` row names none, and each
  part is in some row. It also checks AGENTS.md in two directions: each
  `langc VERB` of AGENTS.md is in the usage text, and each usage verb
  is in AGENTS.md. Count line: `readme: 7 rows, 9 verbs`.
- No diagnostic code changed.
- U4 mutants: 8/8 KILLED, `make -B` for each build. After each mutant,
  the tree is restored and `cmp` gives rc 0.

| # | Mutant | Kill |
|---|---|---|
| M1 | the `sim` tool removed from `src/mcp.c` | mcp: reply 3 (`tools/list`) |
| M2 | `isError` on `status == 0` | mcp: reply 4 and the next replies |
| M3 | jsonin also takes "1.0" | mcp: reply 15 (-32600 expected, a result found) |
| M4 | a word changed in the REFUSE_NO_MERGE row of `src/codes.c` | verbs: the `explain-code` cell |
| M5 | a `:` changed to `;` in the json block of SKILL.md example 1 | skill: example 1 json block |
| M6 | the exit code of SKILL.md example 3 changed from 1 to 0 | skill: example 3 exit code sentence |
| M7 | the text co-editing row of the README table changed to `yes:` | readme: the row names no part |
| M8 | (Q89) `sim_step` writes to `stdout` | mcp: reply 4 and the `sim` drive of inventory |

- Known edges (not S8 changes):
  - `examples/agent/seats.merge` has `tag` in the form `Door`. In the U1
    probe, a `Hall` form with 3 replicas and the ops `sell`, `give` and
    `tag` stopped with EVAL_FUEL.
  - The fix text of REFUSE_NEEDS_COORDINATION names the part `stock` as
    a constant (`src/front/check.c:4358`), also for a form whose part
    has a different name (`seats` in `Door`).
  - Q88 (OPEN, the USER rules): `src/front/check.c:2185` emits
    `CHECK_DEPTH`, but `docs/DIAGNOSTICS.md` has no row for it, so
    `explain-code CHECK_DEPTH` gives rc 1. The code-list check of
    `test/gate.sh` takes only the prefixes LEX, PARSE, REFUSE, TYPE,
    NAME, EVAL, DOMAIN and W. The port has the same gap.
  - Port deltas: `src/front/jsonin.c` has its own `json_utf8_size`;
    `put_string` of `src/mcp.c` uses the rule of `put_string` in
    `src/front/json.c`; the `McpRunTool` callback has a BOUNDS array for
    `sim` in place of the PROTOCOL argument of the port; `run_fmt`,
    `run_sim` and `run_explain_code` take OUT only.
  - `test/readme.c` checks the 4 parts of PLAN 3.2 in place of the
    `list-steps` of the port (Q79 (a)), and a `yes` row has no slice
    tag. AGENTS.md names `eval`, `ir` and `abi` because of the check in
    two directions.
- `make check` EXIT 0 (serial gate run 10): the S7 counts are the same
  (21 lines), and there are four new lines: `skill: 5 examples, 5
  outputs match`, `explain-code: 46 codes, 0 mismatches`, `mcp: 33
  requests, 0 mismatches; 5 examples` and `readme: 7 rows, 9 verbs`.

### S8 review corrections (2026-11-02)

- MCP simulator bounds now follow the integer schema and Q78 (a).
  Values such as `1.0` and `1e0` are normalized to decimal before CLI
  parsing; fractional values give -32602. Integer values outside the
  supported range still receive the CLI USAGE diagnostic.
- `test/mcp.c` adds 25 cases for decimal and exponent spellings,
  negative zero, range boundaries, and fractions that a floating
  conversion would round or underflow. The new regression had 20
  failures before the fix and passes after it. The fractional-bound
  golden response now pins -32602.

## S7 (2026-11-01)

- Scope (`docs/PLAN.md` section 9, the S7 row; Q70 (a) to Q76 (a)): the
  writer changes of 5.1, one template for each part, the lowered
  invariants and the engine ABI. The base is the USER S6 commit 1f94f15.
- U1: `src/ir.h` has `keeps_heap` (the last field of `IrFunc`).
  `src/wasm.c` exports `memory`, `alloc` and `reset` only when an entry
  keeps the heap. The kit entries keep their old behavior.
- U2: `engine_form` in `src/front/check.{c,h}` gives the engine view of
  a form: the parts, the words, the ops and their steps.
  `src/engine.{c,h}` add six entries for each replicated form NAME
  (Q70 (a)): `NAME.apply`, `NAME.merge`, `NAME.check`, `NAME.encode`,
  `NAME.decode` and `NAME.bot`. `memory`, `alloc` and `reset` are
  shared. There is one template for each part (GSet, Counter, Escrow,
  MVReg). `check` gives one bit for each invariant (the bit is 1 when
  the invariant is false). A counter word or a sum above 2^63 - 1 traps
  (Q71 (a)). The `arg` of an op with more than one parameter is the
  mixed radix pack a0 + s0 (a1 + s1 (...)) (Q72 (a)). `decode` gives 0
  for a layout that is not canonical (Q75 (a)): length L, each word at
  most 2^63 - 1, a GSet word 0 or 1, an MVReg word 2 ctx or 2 ctx + 1.
  `src/main.c` calls `engine_program` after `lower_program`. Golden ABI
  files `test/engine/{tags,owner,inventory-escrow,classes}.abi`; the
  gate builds each module, runs `wasm-validate` and `wasm-objdump -x`,
  and compares `langc abi` with the golden file. Count line: `engine: 4
  modules, 0 mismatches`. Build keeps no OUT on a refusal (Q74 (a)).
- U3 (Q73 (a), Q74 (a), Q76 (a)): `search_visit` in
  `src/front/search.{c,h}` calls a visitor for each schedule of the
  bounded search (`search_pair` does not change). `test/engine.c` (C;
  it links the front end, `src/engine.c` and `src/wasm.c`) writes the
  engine module of each form of `examples/*.merge` and
  `test/report/classes.merge`, also of the refused `inventory.merge`.
  For each schedule, it writes a call script and the expected output
  from the guest values: `bot`; for each step `apply` (0 when the guest
  precondition fails), `encode` and the L words; at the merge `merge`,
  `encode`, the L words and `check`. `test/run-wasm.mjs` has `@k` (the
  result of call line k), `words P L` (the L words at P + 8) and `store
  P W...` (it writes the words at P + 8 and prints P). The gate runs
  node on each script and `build/engine -c` on the output. Count line:
  `engine: 5 forms, 14419 schedules, 206 refusals, 279146 calls, 0
  mismatches`. Schedules: tags 676, owner 147, inventory 33, Shop 620,
  Card 12943. Refusals: inventory 9, Shop 197.
- The decode lines (the first schedule of each form): `decode` of the
  encoded state gives the canonical words; `decode` of the merge state
  (word 0 is not L) gives 0; a word of 2^63 (`store`) gives 0; a bad
  shape word (part 0 GSet 2, MVReg 2 ctx 0 + 2) gives 0.
- No diagnostic code changed.
- U4 mutants: 5/5 KILLED by the engine leg only, `check-clang` rc 0 on
  each, `make -B` for each build. Only the engine count line changed
  (`, M mismatches`). E1 the GSet add stores the old word (no OR): M
  55882 (tags, classes). E2 no precondition on the Counter `dec`: M 9
  (inventory). E3 no `U i` loop in the Escrow `can`: M 7
  (inventory-escrow). E4 `SUB(1, live)` to `MUL(EQ(i, j), SUB(1,
  live))` in the MVReg merge: M 149600 (owner, classes). E5 the
  `nonneg` check with the sums in the wrong sides: M 12 (inventory).
- Known edges:
  - An op with a read step is INTERNAL in `engine_form`. No fixture has
    one.
  - `src/main.c` is not the same as the kit file (its header says
    SHARED): `run_target` takes the declarations.
  - Address 0 is the first block and also the refusal value of `apply`
    and `decode`. A host cannot confuse the two: `bot` never refuses,
    and `apply` and `decode` allocate after their input.
  - `test/engine.c` runs each visit on its own fuel and on a scratch
    arena of 1 << 26 bytes that it releases after each visit. It
    computes `def_value` first (the only `Machine` cache that keeps a
    pointer). The main arena is 1 << 30 bytes. Peak RSS of one run: 389
    MB, 10.0 s.
  - The S7 design gave the count line `engine: F forms, S schedules, C
    calls, 0 mismatches` and a `cmp` of each output. The line also gives
    R (the refusals) and M (the mismatches), and `build/engine -c`
    compares the output (rc 1 on a mismatch; the first 5 mismatches of
    a form go to stderr).
  - The decode lines run in the first schedule of each form only.
  - A Counter or Escrow part has no shape check in `decode`, only the
    word limit.
  - `store P W...` in `test/run-wasm.mjs` is a third pseudo call (Q76
    (a)); Q73 (a) gave `@k` and `words P L` only.
- `make check` EXIT 0: the S6 counts are the same, and there are two
  new lines: `engine: 4 modules, 0 mismatches` and `engine: 5 forms,
  14419 schedules, 206 refusals, 279146 calls, 0 mismatches`.

### S7 review corrections (2026-11-01)

- Write arguments may be constants or expressions over finite parameters.
  `engine_form` evaluates each argument for every mixed radix pack, using
  the same numeral-atoms conversion as the guest op. The engine selects
  those values at runtime. Previously, a checked op such as
  `op fixed := add tags ana` failed with INTERNAL during build.
- Argument selection uses shallow assignments. A valid 4096-member pack
  previously failed with WASM_DEPTH because the digit expressions were
  nested. The maximum pack now builds and its last member runs correctly.
- `test/engine/arguments.merge` covers constant and computed GSet, MVReg,
  Counter and Escrow arguments, including transfer targets and numeral
  atoms. Its C visitor compares the Wasm output with the guest values.
  `test/engine/large-pack.merge` covers the last valid pack and traps on an
  invalid pack, replica or op. Both have golden ABI files.
- The engine gate now reports 6 modules and 7 forms, 14581 schedules,
  320 refusals, 281813 calls and 0 mismatches.

## S6 (2026-11-01)

- Scope (`docs/PLAN.md` section 9, the S6 row; Q60 (a) to Q69 (a)): the
  JSON writer, the coordination report of `check --json` and `build`,
  and five golden reports. The base is the USER S5 commit 290f396.
- U1: the writer `src/front/json.{c,h}`, a port of
  `hosts/tcc-json/src/json.{c,h}`. The output buffer `Out` has an arena,
  a diagnostic and a definition in place of the `Machine` of the port.
  `put_string` uses the UTF-8 rule of `base.c`. `diag_print_json` moves
  from `base.c` to `json.c`. The Makefile `FRONT` list has `json.c`.
- U2: `test/report.c`, the C checks of the report rows of
  `examples/owner.merge`, `tags.merge`, `inventory.merge` (refused, the
  counterexample field by field) and `inventory-escrow.merge`. Count
  line: `report rows: 7 checks`.
- U3 (Q60 (a), Q62 (a), Q64 (a), Q65 (a), Q66 (a)): `langc check --json
  SRC` prints one JSON document: `version`, `file`, `replicas`,
  `bounds`, `progress` (`parsed`, `typed`, `laws`, `pairs`), `pairs` and
  `diagnostics`. Each row of `pairs` is one op pair O1 <= O2 of one
  replicated form: `form`, `a`, `b`, `class` (`independent`,
  `explicit-conflict` or `needs-coordination`), `part` (for
  explicit-conflict), `invariant` and `rule` (for an L7 row), `mark`
  (`proved`, `bounded` or `refuted`), `code`, `fix` and
  `counterexample`. A field that does not apply is `null`.
  `test/json/expect.txt` has one document for each of the 35 fixtures.
- U4 (Q61 (a), Q63 (a), Q69 (a)): `langc build SRC -o OUT` writes the
  report next to OUT: OUT with `.wasm` replaced by `.report.json`, else
  OUT with `.report.json` added. Build writes the report also on a
  refusal, on a lex or parse failure and on a source read failure. The
  rc is 0, 1 (refused) or 2 (source IO, report IO or out of memory).
  Golden reports `test/report/{owner,tags,inventory,inventory-escrow,
  classes}.report.json`. The new fixture `test/report/classes.merge`
  has two MVReg parts and a GSet part: 6 rows, 2 explicit-conflict
  (`lead`, `owner`) and 4 independent, all proved. The gate builds each
  golden into `$TMPDIR` and compares it (`cmp`); for 4 of the 5, the
  report bytes are the bytes of `check --json`. Count line: `report: 5
  golden, 0 mismatches, 4 same as check --json`.
- Fix (U4): with no invariant, a pair that no law covers is independent
  and proved (`COVER_L6A`), not bounded. `classes.merge` found it.
- U5 (Q67 (a)): `langc sim` on a form with no invariant takes the
  invariant as true (`SEARCH_NO_INV`). The output is the header with K
  and no verdict line, rc 0. Golden `test/sim/owner.out` (schedules
  147) and `test/sim/tags.out` (schedules 676). This closes the S5 edge
  `INTERNAL: search: no Bot, Merge, Inv or replica def`. The INTERNAL
  text is now `search: no Bot, Merge or replica def for NAME`.
- No diagnostic code changed.
- U6 mutants: 8/8 KILLED by a golden report, `check-clang` rc 0 on
  each, `make -B` for each build. K1 the L6b MVReg test (classes). K2
  two class names swapped (owner, tags, inventory-escrow, classes). K3
  an L6a row marked bounded (tags, classes). K4 the pair loops from O1
  + 1 (all five). K5 `part` from the first MVReg part (classes). K6
  `rule` for L6b rows only, so an L7 row gives `null`
  (inventory-escrow). K7 the rows in reverse order (inventory-escrow,
  classes). K8 the no-invariant path keeps the cover (classes). O2 (the
  S5 survivor) does not apply: `classes.merge` has no refuted row.
- Known edges:
  - An L7 row names the first invariant only.
  - Step arguments are JSON strings (a row holds text only).
  - A source read failure in `build` also writes a report (rc 2), as
    `check --json` prints a document.
  - On a report IO failure, `build` prints the IO diagnostic and then
    the refusal (if there is one).
  - A build refusal on stderr has the default declaration span (as
    `check`).
  - In a file with forms with an invariant and forms with no invariant,
    `sim` prints the verdict line, and its K counts the schedules of
    all forms.
  - O2 (`next_args` order) stays open: no golden file has a refuted row
    with a 2-argument op. GSet and MVReg text in a transcript is not in
    S6 (Q68 (a)).
- `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 23;
  check refusals 76; evals 206; ir 13 lowered; wasm 16 modules valid;
  differential 1230 cases, 0 mismatches; fmt 163 canonical, 14
  non-canonical pairs; merge examples 4 canonical, 3 checked, 1 refused;
  sim 4 golden, 0 mismatches; report 5 golden, 0 mismatches, 4 same as
  check --json; json 35 fixtures, diagnostics 46 codes (32 with a
  fixture, 14 with no fixture); gate 0 failures; review, domain and
  diagnostic edit regressions passed; cubical 124 checks, 41 round
  trips; sim regressions 7 checks; report rows 7 checks. The S5 counts
  are the same; the sim line has 2 more golden files, and the report
  lines are new.

### Review corrections

- Determine whether a form has an invariant from its form items. An
  unrelated user definition such as `TagsInv : Tags → Bool` must not
  filter a form with no invariant. The regression previously counted
  zero schedules; it now counts all 18 enabled schedules.
- `test/sim.c` adds that case: `sim regressions: 8 checks`.
- `AGENTS.md` and `docs/DIAGNOSTICS.md` describe the single report
  document and its diagnostic array. README clarifies partial reports
  after a refusal and verdicts for files containing mixed forms.

## S5 (2026-10-31)

- Scope (`docs/PLAN.md` section 9, the S5 row; Q56 (a), Q57 (a), Q58
  (b), Q59 (a)): `langc sim`, the transcript, two golden files and the
  determinism gate. The base is the USER S4 commit 5b4138b.
- U1 (Q56 (a), Q57 (a)): the command `langc sim PROG [--replicas N]
  [--prefix P] [--ops M]` in `src/main.c`. The defaults are P 1, M 2
  and N the declared replicas (Q17 (a)). A flag that is given two times,
  a flag with no numeral, or a bound above `SEARCH_CAP` gets `USAGE`.
  `check_program_sim` checks the program in sim mode: a replicated form
  gets no `form_search`, so `REFUSE_NEEDS_COORDINATION` does not occur.
  Each other refusal stays. `sim_form` runs `search_pair` with the
  sentinel `SEARCH_ANY_OP` (`src/front/search.h`): each op in each
  branch, and a leaf needs no pair. K counts all complete, enabled and
  I-valid schedules in the bounds.
- U2 (Q59 (a)): the part renderer (Counter, Escrow), the step and
  `requires` lines and the counterexample transcript (`sim_print` in
  `src/front/check.c`). The first violation in the 5.2 order is the
  minimal counterexample (J5). In a refused transcript, K is the count
  of kept schedules before the counterexample, plus 1. Golden
  `test/sim/inventory.out`: 10 lines, `schedules 33`, `sell ∥ sell`
  refuted with `stock = -1`, the code `REFUSE_NEEDS_COORDINATION` and
  the escrow fix line, rc 1.
- U3 (Q58 (b)): golden `test/sim/inventory-escrow.out`: 3 lines,
  `schedules 620`, `invariant kept on all 620 schedules (bounded: 1, 2,
  2)`, rc 0, stderr empty. `owner.merge` and `tags.merge` get no golden
  file in S5 (S6).
- U4: the determinism gate in `test/gate.sh`, after the merge examples
  loop. For each `test/sim/NAME.out`, the gate runs `build/langc sim
  examples/NAME.merge` two times. Each run must be equal (`cmp`) to the
  golden file, and the two runs must give the same rc (1 for a file of
  `merge_refused`, else 0). Count line: `sim: 2 golden, 0 mismatches`.
- Fix: the class row pair line of `sim_print` prints O1 <= O2 in the
  declaration order, not in the branch order. `inventory.out` does not
  change (`sell ∥ sell`).
- U5 mutants: 4 of 6 KILLED by `test/sim/inventory.out`, `check-clang`
  rc 0 on each, `make -B` for each build. O1 the `dfs` branch loop from
  the last replica down (K 43, not 33). O3 the `place` op loop from the
  last op down (K 49, not 33). N1 the outer loop from `max` down and N2
  no `stop` after the first violation (the transcript differs from line
  3). 2 SURVIVED, as the design expects: O2 `next_args` with the last
  argument as the most significant digit (no golden file refutes with a
  2-argument op, Q58 (b), and K does not change with the order) and N3
  the class row pair from the highest branch (no golden file has 3
  replicas).
- Known edges:
  - `langc sim` on `owner.merge` or `tags.merge` (no `invariant`, so no
    `NAMEInv`) gives `INTERNAL: search: no Bot, Merge, Inv or replica
    def`, rc 1.
  - A GSet or MVReg part in a transcript prints the `value_display`
    text. No example has one.
  - An Escrow transfer is not in the parenthesis of the `merged:` line:
    a transfer does not change the stock.
  - O2 and N3 survive until a golden file has a 2-argument op in a
    refutation and 3 replicas.
- `make check` EXIT 0: examples 13 checked, 0 failed; parse refusals 23;
  check refusals 76; evals 206; ir 13 lowered; wasm 16 modules valid;
  differential 1230 cases, 0 mismatches; fmt 163 canonical, 14
  non-canonical pairs; merge examples 4 canonical, 3 checked, 1 refused;
  sim 2 golden, 0 mismatches; json 35 fixtures, diagnostics 46 codes (32
  with a fixture, 14 with no fixture); gate 0 failures; review, domain
  and diagnostic edit regressions passed; cubical 124 checks, 41 round
  trips. The S4 counts are the same; the sim line is new.

### Review corrections (2026-10-31)

- With `--replicas 0`, reconnect keeps the synced ancestor. The empty
  schedule counts once, and enabled prefix schedules still count.
- Stock text comes from the guest `counterStock` or `escrowStock`
  function. Positive values above `INT64_MAX` stay positive, and the
  transcript identifies the invariant that actually failed.
- An allocation suggestion distributes the declared initial stock,
  so the shares satisfy `REFUSE_ALLOC_SUM` even after prefix restocking.
- A two-step counterexample with one step in each of two branches names
  its operation pair. More complex counterexamples report `schedule:
  needs coordination`, since the first operation of each branch can
  preserve the invariant.
- State rendering grows its buffer and reports `EVAL_PRINT` if a value
  still exceeds the printer depth or 1 MiB limit. It does not accept a
  truncated state as a complete counterexample.
- `test/sim.c` adds seven C regression checks to `make check` and the
  clang syntax check. The two existing golden transcripts keep their
  bytes and schedule counts.

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
- S5: `langc sim` with the bounds flags (Q17 (a)) and sim mode (Q57
  (a)); the sim search with `SEARCH_ANY_OP` (Q56 (a)); the transcript
  renderer for Counter and Escrow parts and the minimal counterexample
  (Q59 (a)); the golden files `test/sim/inventory.out` and
  `test/sim/inventory-escrow.out` (Q58 (b)); the determinism gate in
  `test/gate.sh`.

## Remaining work

No slice of `docs/PLAN.md` section 9 is left; the plan is CLOSED (Q105
(a)). The open items that S10 carries (PLAN section 10):

- O2: the 2024 HN thread is not found; the docs cite "an HN discussion
  (2024)".
- O3: no wasmtime leg in the differential gate (not in v1).
- O5: `Glue` and `ua` are not ported; no merge-lang law needs them.
- O6: a user merge function with proof terms (on a USER ask only).
- O7: counter growth in ℕ; the engine traps at `i64` overflow.
- O8: no S1 list of the evolve-lang O10 to O18 was found.
- O9: the Automerge facts are from 2026-10-27; check them before a public
  README.
- O10: the bounded search is a refuter, not a prover.

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
- `langc sim` needs an `invariant` in each replicated form: a form with
  no invariant gets `INTERNAL` (S5 Known edges).
- The sim transcript renders Counter and Escrow parts. A GSet or MVReg
  part prints its `value_display` text.
- A kept invariant in `langc sim` is bounded, not proved.
- Counters and version counters grow without bound in ℕ. The engine
  traps at `i64` overflow (PLAN O7).
- The bounded search is a refuter, not a prover: it can find a
  counterexample, but it cannot prove a law for all states (PLAN O10).

## Internal boundaries

`src/front/` holds the lexer, the parser, the checker (`src/front/check.c`:
the builtin table and the refusal rules) and the lowering to the IR.
`src/main.c` holds the commands of `langc`, the JSON output and the
`W_NONCANON` test. The C tests are in `test/` (`test/cubical.c`,
`test/diagnostics.c`, `test/fronttool.c`). The guest fixtures are in
`test/check`, `test/eval`, `test/fmt`, `test/ir`, `test/json` and
`test/sim`. The gate compares the refusal, evaluation and JSON output with
`expect.txt` files, formatted text with `.canon` files, sim transcripts
with `test/sim/*.out` files (two runs each), and evaluator results with
Wasm.
A program can declare
CM and CC signatures that pass R1, but cannot declare base data, an axiom
or a general recursive definition (`docs/PLAN.md` section 2).
