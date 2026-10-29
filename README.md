# merge-lang

merge-lang is a language for offline replicated state with checked merge
laws. A program declares replicated state, the permitted operations, the
merge of each state part and the domain invariants. The checker puts each
pair of concurrent operations in one of three classes: independent,
explicit conflict, or needs coordination. For a pair that needs
coordination, it gives a concrete counterexample or it accepts a declared
resource allocation rule. The host is TinyCC. The planned targets are a
Wasm merge engine, a deterministic replica simulator (`langc sim`) and a
JSON coordination report.

## Status (S3a)

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
FMSet S → S` as program data) and slice S3a (the parse and `fmt` of the
forms `replicas`, `atoms`, `replicated`, `state`, `op`, `invariant` and
`allocate`; `langc check` refuses a form with `REFUSE_FORM` until S3e;
`docs/STATUS.md`). There is no merge
engine, no replica simulator and no coordination report yet.
`make check` passes. `docs/PLAN.md` section 9 gives the slices.

The compiler `langc` is a native program that tcc builds. It is not a Wasm
module. The planned Wasm output is the merge engine that `langc` writes
for each program (`docs/PLAN.md` 5.1).

merge-lang does not prove facts about the network. It assumes that each
replica state that is sent arrives some time. It does not model message
loss, durability, Byzantine replicas or clocks (`docs/PLAN.md` 6.2). A law
has one of three marks: proved (a guest term type-checks), bounded (the
simulator found no violation in all schedules up to the bounds; this is
not a proof) or refuted (a counterexample). The planned report never says
proved for a bounded result.

## When to use merge-lang

| Task | merge-lang? |
|---|---|
| offline-first replicated state with domain invariants | yes (S4) |
| a tag set where concurrent additions must both stay | yes (S4) |
| an owner field where concurrent writes must stay an explicit conflict | yes (S4) |
| a stock that must never become negative after offline decrements | yes: refused with a counterexample, or kept with an escrow rule (S4, S5) |
| text co-editing | no: merge-lang has no sequence or text type (`docs/PLAN.md` 3.3) |
| a server-only database | no: one server applies its own constraints and needs no merge |
| Redis scripts | no: use tether |

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
- `SPEC.md`, `formers/`, `probe/`, `domain/`, `docs/STATUS.md`,
  `docs/VALIDATION.md`, `docs/host/README.md`: the kit files. The slices
  fill them.

## Build

`make check` builds `build/langc` with tcc and runs the gate. The gate
needs tcc, node, wasm-validate and wasm-objdump.

## License

MIT OR Apache-2.0.
