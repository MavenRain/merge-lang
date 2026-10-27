# Conflict semantics: Automerge and merge-lang

Status: S0. No merge-lang form in this file is built yet. The merge-lang
side is the plan (`docs/PLAN.md` 4.2, 4.4 and 4.5). The Automerge side is
the research fact of `design/DESIGN.md` section 3, from the two sources at
the end of this file.

## Automerge (JS API v3.1.1)

- Two concurrent `put` operations on one map key make the value
  conflicted.
- Automerge "chooses one value arbitrarily (but deterministically)".
- A plain read gives last-writer-wins. "Last" is the internal operation
  ID, not wall-clock time.
- `getConflicts` returns the winner and the losers, keyed by the
  operation IDs that wrote them.
- The next assignment to the key resolves the conflict.

Automerge keeps the conflict, but a plain read hides it. A program that
does not call `getConflicts` takes the winner and does not know it.

## merge-lang (planned)

- An owner field is an `MVReg n r` part: one slot for each replica and a
  version vector (`docs/PLAN.md` 4.2).
- `read : MVReg n r → Read n`. A `Read n` value has one of three forms:
  `empty`, `one x`, or `conflict s`, where `s` has 2 or more members
  (Q11).
- Replica A writes `ana` and replica B writes `ben` concurrently. After
  the merge, `read owner` gives `conflict {ana, ben}`. The conflict is a
  value of the read type (`docs/PLAN.md` 4.4).
- A program that uses `read owner` where it needs a user gets the refusal
  `REFUSE_READ_WINNER` with the fix "case on `Read`: `one u`,
  `conflict s`, `empty`" (`docs/PLAN.md` 4.5). So no program takes a
  winner in silence.
- A later `write` at a replica that has the conflict clears both slots.
  This resolves the conflict, as the next assignment does in Automerge.

## Summary

| Question | Automerge | merge-lang (planned) |
|---|---|---|
| two concurrent writes to one key | the value is conflicted | `read` gives `conflict {x, y}` |
| a plain read | one winner, by operation ID | a `Read n` value, no winner |
| all the concurrent values | `getConflicts` | a case on `Read n` |
| a read used as one value | permitted, the winner | refused (`REFUSE_READ_WINNER`) |
| resolution | the next assignment | the next `write` |
| order | internal operation IDs, no wall clock | a version vector, no clocks |

## Sources

- https://automerge.org/automerge/api-docs/js/functions/getConflicts.html
- https://automerge.org/docs/reference/documents/conflicts/
