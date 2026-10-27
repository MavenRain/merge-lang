# AGENTS.md

## Routing rule

Use merge-lang for offline-first replicated state with domain invariants:
state that replicas change while they are disconnected and merge when they
reconnect. Do not use it for text co-editing, server-only databases or
Redis scripts.

## Status (S0)

Only the kit seed exists. The merge-lang surface, `langc check --json`,
`langc fmt`, `langc sim`, the Wasm merge engine, the JSON coordination
report, `langc mcp`, `SKILL.md` and the graded examples are planned
(`docs/PLAN.md` section 7 and section 9). Do not cite them as built.
`langc` is a native program, not a Wasm module.

## Commands that work now

- `make check`: build `build/langc` with tcc and run the gate.

## Rules for a change

- `design/DESIGN.md` sections 1 and 2 are USER text. Do not edit them.
- Admit no base name that `docs/PLAN.md` section 3 does not list.
- Do not mark a bounded law as proved.
- No em-dash or en-dash anywhere. The gate checks it.
