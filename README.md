# merge-lang

merge-lang is a language for offline replicated state with checked merge
laws. A program declares replicated state, the permitted operations, the
merge of each state part and the domain invariants. The checker puts each
pair of concurrent operations in one of three classes: independent,
explicit conflict, or needs coordination. For a pair that needs
coordination, it gives a concrete counterexample or it accepts a declared
resource allocation rule. The host is TinyCC. The deterministic replica simulator (`langc sim`)
the JSON coordination report (`langc check --json`, `langc build`) and
the Wasm merge engine that `langc build` writes are built.

## Status (S10)

This repository is the tcc-wasm kit seed of lang-template, the design, the
plan, slice S1 (the port of the evolve-lang 1f87a5a surface and cubical
kernel), slice S1b (induction in the kernel: `natMax`, a `fold` over ℕ
with a motive, `elim` for Bool and `Option`, and `sumFin`), slice S2
(the domain: `GSet`, `Counter`, `Escrow`, `MVReg`, `Read n`, the product
join and the laws L1 to L4 as guest terms), slice S2b (ℤ, `natDiff`,
`intLe` and the `stock` read of `Counter` and `Escrow`), slice S2c
(path constructors of dimension 1 and 2, the `fold` over a higher
inductive type, a bottom and a left unit law for each part, and `FMSet`
as program data), slice S2d (the ℕ `fold` at an open successor
`natAdd y c`, `isSet` for each part by a Hedberg port, and `mergeAll :
FMSet S → S` as program data), slice S3a (the parse and `fmt` of the
forms `replicas`, `atoms`, `replicated`, `state`, `op`, `invariant` and
`allocate`), slice S3e (`langc check` elaborates the forms to guest
terms, with the codes `REFUSE_NO_MERGE`, `REFUSE_OP_INFINITE_ARG`,
`REFUSE_RAW_WRITE` and `REFUSE_ALLOC_SUM`), slice S3x (the gate
runs `langc check` on the four examples `examples/*.merge`), slice
S3b (a cell monad field `E → T A` with `E` closed data, the library
`Ops A`, an `op` body as a script of part reads and part operations,
and `PLe` with `PLeTrans` for each part), slice S4 (the checker: the
classes J4 to J6, the bounded search, `REFUSE_NEEDS_COORDINATION`,
`REFUSE_RULE_MISMATCH`, `REFUSE_READ_WINNER`, `W_BOUNDED`, the laws L5,
L5r, L6a, L6b, L7 and L8, the `Replica S` machine, and the `laws` and
`pairs` counts of `check --json`), slice S5 (`langc sim`: the
schedules in the 5.2 order, the transcript, the minimal counterexample,
the golden files `test/sim/*.out` and the determinism gate), slice
S6 (the JSON coordination report of `check --json` and `build`, and the
golden reports `test/report/*.report.json`), slice S7 (the Wasm merge
engine of `langc build`: six entries for each replicated form, one
template for each part, the lowered invariants, and a differential gate
against the guest on each simulator schedule) and slice S8 (the agent
kit: `langc explain-code CODE`, the MCP server `langc mcp` with the
tools `check`, `fmt`, `sim` and `explain-code` (protocol revision
2025-11-25), `skills/merge-lang/SKILL.md` and its 5 graded examples;
`docs/STATUS.md`).
Slice S9 adds the token baseline `docs/TOKEN-BASELINE.md` and its gate
leg (`build/tokcount --check`). Slice S10 is the close:
`docs/VALIDATION.md` and the mutant summary in `docs/STATUS.md`. The
plan is CLOSED.
`make check` passes. `docs/PLAN.md` section 9 gives the slices.

The compiler `langc` is a native program that tcc builds. It is not a Wasm
module. The Wasm output is the merge engine that `langc build` writes
for each program (`docs/PLAN.md` 5.1).

merge-lang does not prove facts about the network. It assumes that each
replica state that is sent arrives some time. It does not model message
loss, durability, Byzantine replicas or clocks (`docs/PLAN.md` 6.2). A law
has one of three marks: proved (a guest term type-checks), bounded (the
simulator found no violation in all schedules up to the bounds; this is
not a proof) or refuted (a counterexample). The report never says proved
for a bounded result.

## Coordination report

`langc check --json SRC` prints one JSON document. It has the progress
counts, the diagnostics and one row for each classified pair of operations
of each replicated form. A refusal stops classification at the first
violation, so its rows end with the refuted pair. A row gives the class (`independent`,
`explicit-conflict` or `needs-coordination`), the mark (`proved`,
`bounded` or `refuted`) and, for a refuted row, the code, the fix and
the counterexample. `langc build SRC -o OUT.wasm` writes the same
document to `OUT.report.json`, also when the program is refused.
`docs/STATUS.md` (`## S6`) gives the fields.

`langc sim SRC` counts the schedules of each replicated form in the
bounds. A form with no `invariant` counts all enabled schedules. A file
with no invariants prints the header with the schedule count and no
verdict line.

## When to use merge-lang

| Task | merge-lang? | Parts |
|---|---|---|
| offline-first replicated state with domain invariants | yes | `GSet`, `MVReg`, `Counter`, `Escrow` |
| a tag set where concurrent additions must both stay | yes | `GSet` |
| an owner field where concurrent writes must stay an explicit conflict | yes | `MVReg` |
| a stock that must never become negative after offline decrements | yes: refused with a counterexample, or kept with an escrow rule | `Counter`, `Escrow` |
| text co-editing | no: merge-lang has no sequence or text type (`docs/PLAN.md` 3.3) | |
| a server-only database | no: one server applies its own constraints and needs no merge | |
| Redis scripts | no: use tether | |

The Parts column names the admitted parts of `docs/PLAN.md` 3.2 that do
the task. The gate (`build/readme-test`) checks the table.

## A distinct space

dil-lang is a language for reactive interfaces. It compiles a program to a
JavaScript module and a JSON file. tether is a language for Redis scripts.
It compiles a program to a Wasm module and a shell script that share the
same Lua bodies. merge-lang is for replicated state: it checks the merge
laws and puts each pair of concurrent operations in a coordination class.

## Files

- `design/DESIGN.md`: the USER ask (section 1, verbatim), the rulings
  (section 2) and the research facts (section 3).
- `docs/PLAN.md`: rulings, the R1 reading, the base signature, the merge
  model, targets, laws, agent adoption, slices and questions.
- `docs/COMPARISON.md`: the Automerge conflict semantics and the planned
  merge-lang semantics.
- `AGENTS.md`: how an agent uses this repository.
- `skills/merge-lang/SKILL.md` and `examples/agent/`: the agent skill and
  its graded examples.
- `SPEC.md`, `formers/`, `probe/`, `domain/`, `docs/STATUS.md`,
  `docs/VALIDATION.md`, `docs/host/README.md`: the kit files. The slices
  fill them.

## Build

`make check` builds `build/langc` with tcc and runs the gate. The gate
needs tcc, node, wasm-validate and wasm-objdump.

## License

MIT OR Apache-2.0.
