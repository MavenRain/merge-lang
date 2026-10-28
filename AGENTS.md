# AGENTS.md

## Routing rule

Use merge-lang for offline-first replicated state with domain invariants:
state that replicas change while they are disconnected and merge when they
reconnect. Do not use it for text co-editing, server-only databases or
Redis scripts.

## Status (S2c)

S1, S1b, S2, S2b and S2c are built: the port of the evolve-lang 1f87a5a
surface and cubical kernel (S1), induction in the kernel (S1b: `natMax`,
a `fold` over ℕ with a motive, `elim` for Bool and `Option`, and
`sumFin`), the domain with the laws L1 to L4 as guest terms (S2:
`GSet`, `Counter`, `Escrow`, `MVReg`, `Read n`, the product join),
ℤ with the `stock` read (S2b: `natDiff`, `intLe`, `counterStock`,
`escrowStock`), and the kernel port of path constructors of dimension 2
(S2c: the `fold` over a higher inductive type, a bottom and a left unit
law for each part, `FMSet` and `fmsetMap` as program data in
`test/eval/fmset.lang`; `docs/STATUS.md`).
`langc check --json` and `langc fmt` work. `isSet` for each part and
`mergeAll` (S2d),
`langc sim`, the Wasm merge engine, the JSON
coordination report, `langc mcp`, `SKILL.md` and the graded examples are
planned (`docs/PLAN.md` section 7 and section 9). Do not cite them as built.
`langc` is a native program, not a Wasm module.

## Commands that work now

- `make check`: build `build/langc` with tcc and run the gate.
- `build/langc check --json PROG`: check a program. Each diagnostic is one
  JSON line (`docs/DIAGNOSTICS.md`). The last line gives the `parsed` and
  `typed` counts.
- `build/langc fmt PROG`: print the canonical text of a program. `langc
  check` gives `W_NONCANON` when a program region is not canonical.

## Rules for a change

- `design/DESIGN.md` sections 1 and 2 are USER text. Do not edit them.
- Admit no base name that `docs/PLAN.md` section 3 does not list.
- Do not mark a bounded law as proved.
- No em-dash or en-dash anywhere. The gate checks it.
