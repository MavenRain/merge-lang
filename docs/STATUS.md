# merge-lang status

The build implements slices S1, S1b, S2, S2b and S2c of `docs/PLAN.md`
section 9: the port of the evolve-lang 1f87a5a surface and cubical kernel
(S1), induction in the kernel (S1b), the merge-lang domain with the laws
L1 to L4 as guest terms (S2), ℤ with the `stock` read (S2b), and path
constructors of dimension 2 with the `fold` over a higher inductive type
(S2c). It does
not complete M0. There is no merge engine, no replica simulator and no
coordination report yet.

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

## Remaining M0 work

1. S2d: `isSet` for each part (a Hedberg port) and `mergeAll : FMSet S →
   S` on the program side (Q29 (a), Q30 (b)). It needs the USER S2c
   commit.
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
  (O52). A `fold` into a part over a type with a 2-cell needs `isSet` of
  the part (S2d).
- `sumFin` at an open `n` is stuck: `langc ir` refuses it with
  `REFUSE_RUNTIME_PATH`.
- The kit names `List`, `nil` and `cons` are dropped. Use a vector
  `Fin n → A`.
- A constructor field is not dependent: `readConflict` takes one Sigma
  field (the set and the proof).
- `nonneg` compares in ℕ. The signed `stock` read gives ℤ (S2b).
- ℤ has only `natDiff` and `intLe`: there is no addition, subtraction or
  negation on ℤ. The print rule applies only to the domain data `ℤ`.

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
