# AGENTS.md

## Routing rule

Use merge-lang for offline-first replicated state with domain invariants:
state that replicas change while they are disconnected and merge when they
reconnect. Do not use it for text co-editing, server-only databases or
Redis scripts.

## Status (S7)

S1, S1b, S2, S2b, S2c, S2d, S3a, S3e, S3x, S3b, S4, S5, S6 and S7 are built: the port of the evolve-lang
1f87a5a surface and cubical kernel (S1), induction in the kernel (S1b:
`natMax`, a `fold` over ℕ with a motive, `elim` for Bool and `Option`,
and `sumFin`), the domain with the laws L1 to L4 as guest terms (S2:
`GSet`, `Counter`, `Escrow`, `MVReg`, `Read n`, the product join),
ℤ with the `stock` read (S2b: `natDiff`, `intLe`, `counterStock`,
`escrowStock`), the kernel port of path constructors of dimension 2
(S2c: the `fold` over a higher inductive type, a bottom and a left unit
law for each part, `FMSet` and `fmsetMap` as program data in
`test/eval/fmset.lang`), `mergeAll` (S2d: the ℕ `fold` at an open
successor `natAdd y c`, `isSet` for each part by a Hedberg port,
`mergeAll : FMSet S → S` as program data in `test/eval/mergeall.lang`),
the parse and `fmt` of the merge forms (S3a: `replicas`, `atoms`,
`replicated`, `state`, `op`, `invariant`, `allocate`), the
elaboration of the forms to guest terms (S3e: `langc check` types the
forms, with `REFUSE_NO_MERGE`, `REFUSE_OP_INFINITE_ARG`,
`REFUSE_RAW_WRITE` and `REFUSE_ALLOC_SUM`), the check of the four
examples in the gate (S3x), and `op` scripts (S3b: a cell monad field
`E → T A` with `E` closed data, the library `Ops A` with the runner
`NAMERun`, the script syntax `x ← read PART; ...`, and `PLe` with
`PLeTrans` for each part), the checker (S4: the classes J4 to J6,
the bounded search, `REFUSE_NEEDS_COORDINATION`,
`REFUSE_RULE_MISMATCH`, `REFUSE_READ_WINNER`, `W_BOUNDED`, the laws L5,
L5r, L6a, L6b, L7 and L8, the `Replica S` machine, the `laws` and
`pairs` counts of `check --json`), and the replica simulator (S5:
`langc sim`, the transcript, the golden files `test/sim/*.out` and the
determinism gate), and the JSON coordination report (S6: `check --json`
and `build`, the op pair rows and counterexamples, and golden reports
in `test/report/*.report.json`), and the Wasm merge engine (S7: the six
entries of each replicated form, the golden ABI files
`test/engine/*.abi` and the differential gate of `test/engine.c`;
`docs/STATUS.md`).
`langc check --json`, `langc build`, `langc fmt` and `langc sim` work.
`langc mcp`, `SKILL.md` and the graded examples are
planned (`docs/PLAN.md` section 7 and section 9). Do not cite them as built.
`langc` is a native program, not a Wasm module.

## Commands that work now

- `make check`: build `build/langc` with tcc and run the gate.
- `build/langc check --json PROG`: check a program and print one JSON
  report document with `progress`, `pairs` and `diagnostics`
  (`docs/DIAGNOSTICS.md`, `docs/STATUS.md` section S6).
- `build/langc build PROG -o OUT.wasm`: write the check report to
  `OUT.report.json`, including on a refusal, then lower and emit the
  module when the check passes. The module is the merge engine: for
  each replicated form NAME, `NAME.apply`, `NAME.merge`, `NAME.check`,
  `NAME.encode`, `NAME.decode` and `NAME.bot`, and the shared `memory`,
  `alloc` and `reset` (`docs/PLAN.md` 5.1). On a refusal there is no
  module.
- `build/langc fmt PROG`: print the canonical text of a program. `langc
  check` gives `W_NONCANON` when a program region is not canonical.
- `build/langc sim PROG [--replicas N] [--prefix P] [--ops M]`: run the
  replica simulator on a merge program. It prints the schedule count K.
  For a refuted invariant, it prints the minimal counterexample
  transcript (rc 1). A kept invariant is bounded, not proved (rc 0). The
  golden files are `test/sim/*.out`. A form with no invariant counts all
  enabled schedules. A file with no invariants has no verdict line.

## Rules for a change

- `design/DESIGN.md` sections 1 and 2 are USER text. Do not edit them.
- Admit no base name that `docs/PLAN.md` section 3 does not list.
- Do not mark a bounded law as proved.
- No em-dash or en-dash anywhere. The gate checks it.
