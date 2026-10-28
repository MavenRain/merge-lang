# merge-lang status

The build implements slices S1, S1b and S2 of `docs/PLAN.md` section 9:
the port of the evolve-lang 1f87a5a surface and cubical kernel (S1),
induction in the kernel (S1b), and the merge-lang domain with the laws
L1 to L4 as guest terms (S2). It does not complete M0. There is no merge
engine, no replica simulator and no coordination report yet.

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
  indexed families, path constructors of dimension 1, `REFUSE_CELL` for CM
  and CC signatures, codata, the corecursor, `head` and `tail`.
- `Fin n` as the Sigma of `docs/PLAN.md` section 2.2.
- Induction (S1b): `natMax` with its rules on open arguments, a `fold`
  over ℕ with a motive, `elim` for Bool and `Option`, and `sumFin`.
- The domain (S2): `GSet`, `Counter`, `Escrow`, `MVReg`, `Read n`, the
  product join and the laws L1 to L4 as guest terms.

## Remaining M0 work

1. S2b: ℤ (`pos`, `negsuc`, `natDiff`, `intLe`) and the `stock` read of
   `Counter` and `Escrow`. It needs the USER S2 commit.
2. The later slices of `docs/PLAN.md` section 9. Each needs the slice
   before it.

## Known limits

- `langc` stops at the first error (`docs/DIAGNOSTICS.md`).
- `langc check` gives one `W_NONCANON` at most: for the first program region
  that is not canonical.
- A path constructor has dimension 1. A square gets `REFUSE_CELL`.
- `sumFin` at an open `n` is stuck: `langc ir` refuses it with
  `REFUSE_RUNTIME_PATH`.
- The kit names `List`, `nil` and `cons` are dropped. Use a vector
  `Fin n → A`.
- A constructor field is not dependent: `readConflict` takes one Sigma
  field (the set and the proof).
- `nonneg` compares in ℕ. The signed `stock` read waits for ℤ (S2b).

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
