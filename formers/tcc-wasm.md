# TinyCC Wasm realization

Paths below are relative to the root of merge-lang. `formers/FORMERS.md`
gives the host-neutral laws. `src/front/check.c` lines 82-126 are rows of
the builtin table. `src/front/check.c` lines 1424-1440 are rows of the
dropped kit names. The other `src/front/check.c` lines are the
elaboration. `src/front/parser.c` lines are the grammar.
`src/front/lower.c` lines are the lowering to the IR.

DONE means: `langc check` accepts the former, `langc eval` computes it,
and `langc build` lowers it to Wasm when its value is first-order at run
time. Types and universes erase on Wasm. Paths compute during
normalization; a residual path at run time gives `REFUSE_RUNTIME_PATH`.
Traps propagate when the computation forces their value. PLANNED means:
since S1, no program name gives the
former. The checker refuses the dropped kit name with the unknown-name
code and migration advice. `fix` names a replacement only when one name
can replace it; otherwise `fix` is `null` (`docs/DIAGNOSTICS.md`).

| ID | Former | Status | Where | Evidence |
|---|---|---|---|---|
| F1 | Product `A × B` | DONE | `src/front/parser.c` (`parse_prod_body`), `src/front/check.c` (`rule_pack`, `rule_sigma_part`), `src/front/lower.c` (`lower_canon`, `lower_stuck`) | `examples/formers.lang`, `examples/laws.lang` |
| F2 | Coproduct | DONE as a base sum: a domain data type with more than one constructor, eliminated by `fold`. S2d removed the kit `Sum`, `inl`, `inr`, `either` (Q-S2d-4 (a)): `NAME_UNKNOWN`, hint "use Option or a base sum" | `domain/domain.lang` (`Color`), `src/front/check.c` (`rule_fold`) | `examples/algebra.lang`, `test/check/sum.lang` |
| F3 | Option | DONE | `src/front/check.c` (`rule_type1`, `rule_empty`, `rule_some`, `rule_option`), `src/front/lower.c` (`two_cases`) | `examples/formers.lang`, `examples/laws.lang` |
| F4 | List | PLANNED: no program name since S1 (O18). The message recommends a vector `Fin n → A` (S2); `fix` is `null` | `src/front/check.c` (`DROPPED`) | No program form |
| F5 | Monad pure/map/bind | DONE over Option | `src/front/check.c` (`rule_pure`, `rule_map`, `rule_bind`), `src/front/lower.c` (`carrier_map`, `carrier_bind`) | `examples/monad.lang`, `examples/laws.lang` |
| F6 | Algebra fold | DONE over ℕ and non-indexed data types with no path constructors. HOST-LIMIT: a run-time tree | `src/front/check.c` (`rule_fold`), `src/front/lower.c` (`fold_nat`, `family_fold`) | `examples/algebra.lang`, `domain/domain.lang`, `test/ir/spine.lang`, `test/ir/tree.lang` |
| F7 | Algebra unfold | PLANNED: no program name since S1 (O18) | `src/front/check.c` (`DROPPED`) | No program form |
| F8 | Filter | PLANNED: no program name since S1 (O18). No core name replaces it | `src/front/check.c` (`DROPPED`) | No program form |
| F9 | Non-dependent Pi | DONE. HOST-LIMIT: a function value at run time | `src/front/check.c` (`elab_binder`, `infer_lam`), `src/front/lower.c` (`REFUSE_RUNTIME_CLOSURE`) | `examples/functions.lang`, `examples/laws.lang`, `test/ir/closure.lang` |
| F10 | Dependent Pi | DONE over `Type 0` and over ℕ | `src/front/check.c` (`elab_binder`, `apply_one`) | `examples/functions.lang`, `examples/equality.lang`, `examples/universes.lang` |
| F11 | Sigma | DONE | `src/front/check.c` (`elab_binder`, `rule_pack`, `rule_sigma_part`) | `examples/sigma.lang` |
| F12 | Eq refl/symm/trans | DONE | `src/front/check.c` (`rule_refl`, `rule_symm`, `rule_trans`), `src/front/eval.c` (`reduce_papp`) | `examples/equality.lang`, `test/check/bad-refl.lang`, `test/ir/proof-trap.lang` |
| F13 | Eq transport/cong | DONE | `src/front/check.c` (`rule_transport`, `rule_cong`), `src/front/eval.c` (`reduce_transp`) | `examples/equality.lang`, `test/ir/proof-trap.lang` |
| F14 | Universes | DONE for Type 0 and Type 1, without cumulativity | `src/front/check.c` (`UNIVERSE_MAX`, `elab_inner`) | `examples/universes.lang`, `test/check/universe.lang`, `test/check/universe-range.lang` |
| F15 | Generic indexed family | PARTIAL: base indexed data comes from the domain; CM value indices are allowed, and dependent folds are not implemented | `domain/domain.lang` (`Lot`, `Hop`), `src/front/check.c` (`check_family`, `ctor_app`, `proj_app`) | `examples/inventory.lang`, `examples/laws.lang` |

F4, F7 and F8: S1 removed the kit names `List`, `nil`, `cons`, `unfold`
and `filter` (PLAN 3.3). Their core operations and lowering code were
removed in the port; the `DROPPED` table in `src/front/check.c` keeps
the migration diagnostics.

F6: a fold over a data type with one recursive field (for example `Stack`)
lowers to a loop over the spine. A constructor with two or more recursive
fields (for example `Tree`) needs a tree at run time. The lowering refuses
it with `REFUSE_RUNTIME_TREE` when the tree exists only at run time.
A fold over a closed tree works, because normalization folds it at
compile time.

F9: a lambda stays only as the direct function argument of an eliminator.
When a function value must exist at run time, the lowering refuses it
with `REFUSE_RUNTIME_CLOSURE`. Both refusals are HOST-LIMIT reasons, not
defects. `docs/host/CAPABILITY.md` gives the reasons.

F15: `Lot` has the uniform value parameter `size`, and
`makeLot 3 250 : Lot 3`. `Hop` has two result indices:
`up n : Hop n (natAdd n 1)`, and `hopFrom (up n)` computes to `n`.
The checker keeps distinct parameter and index values apart. A program
cannot declare base data with no type argument (rule R2), but a CM can
have value indices. A fold over an indexed family or a family with path
constructors is refused until the dependent eliminator in S3. Indices
erase on Wasm.

The refusal mutants are in `test/check/`: R1 `cell-list.lang`,
R2 `data.lang` and `axiom.lang`,
R3 `rec.lang`, `self.lang` and `later.lang`, R4 `core-name.lang`,
`domain-name.lang`, `reserved.lang` and `duplicate.lang`.
`test/check/expect.txt` gives the code for each mutant.
