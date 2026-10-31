# merge-lang diagnostics

`langc` stops at the first error. `langc check` can also give one warning,
`W_NONCANON`. In text mode, each diagnostic is one line on stderr:
`langc: CODE: NAME: message`. NAME is the definition, or `-` when there is
no definition. With `langc check --json FILE`, each diagnostic is one JSON
object on one line of stdout. The error comes first (at most one), then the
warning (at most one), then the progress object. The exit code is the same
in the two modes: 0 ok, 1 refused, 2 usage or IO. A warning does not change
the exit code.

## The JSON record

    {"code":"CODE","severity":"error","span":{"file":"F","line":1,"col":1,"end_line":1,"end_col":2},"expected":null,"found":null,"fix":null,"retry":true,"message":"..."}

- `severity`: `"warning"` for a code that starts with `W_`, else `"error"`.
- `span`: lines and columns start at 1. A column counts code points, not
  bytes. The value is `null` when the diagnostic has no position.
  For `W_NONCANON`, the span covers exactly `found`, including comments,
  blank lines and the final newline when present. Replace this range with
  `fix` to apply the edit; the end position is exclusive.
- `expected`, `found`, `fix`: strings, or `null` when the code does not set
  them. The strings are JSON-escaped: valid UTF-8 stays raw, and a control
  byte or an invalid UTF-8 byte is `\u00XX`.
- `retry`: `false` for `OOM`, `IO`, `IO_SIZE`, `USAGE`, `INTERNAL` and each
  `EVAL_` code (a change to the program text cannot remove them), `true`
  for all other codes.
- `message`: the last part of the text-mode line `langc: CODE: NAME: message`.

The S1 reading of the port sources (agent-lang 06854bf, dil-lang 9e57528)
found nothing to port (S1 HANDOFF). docs/STATUS.md records the read.

## The progress object

    {"parsed":"3 of 3","typed":"2 of 3","laws":{"total":0,"proved":0,"bounded":0,"refuted":0},"pairs":{"total":0,"classified":0}}

- m (the second number) is the number of program declarations. After a parse
  error, m is the complete declarations plus the one that failed. After a lex
  error, m is 0.
- `parsed`: the declarations that parse. `typed`: the declarations that
  type-check before the first check error (all m when the program is ok).
- `laws` (PLAN 5.3, Q50 (a)): the laws of the replicated forms. Each form
  has L1 to L4, L5 and L5r, one L6a for each GSet part, one L6b for each
  MVReg part, one L7 for each allocated part, and one L8 for each op pair
  (an op with itself too) that no library law covers. `total` counts all
  of them; `proved` the laws that type-check (L6a and L6b are domain
  lemmas); `bounded` the L8 pairs that the bounded search keeps; `refuted`
  the L8 pair of REFUSE_NEEDS_COORDINATION. After a check error, the
  counts stop at the error. A program with no form gives 0 for each key.
- `pairs`: `total` is the op pairs of the forms; `classified` the pairs
  with a class (a library law, the bounded search, or no invariant).

## Codes

The gate (test/gate.sh) compares this list with the codes in src/. Each code
has the fixture test/json/CODE.lang, or its row gives `no fixture:` and a
reason. test/json/expect.txt holds the exact JSON lines of each fixture.

| Code | Severity | Stage | Meaning | Fixture |
|---|---|---|---|---|
| `LEX_CHAR` | error | lex | A character outside a comment that starts no token: a non-ASCII character other than `→`, `λ`, `×`, `ℕ`, `∧`, `∨`, `↦` and `∣`, or an ASCII character that the lexer does not use. | test/json/LEX_CHAR.lang |
| `LEX_NAT_RANGE` | error | lex | A numeral larger than 2^64-1. | test/json/LEX_NAT_RANGE.lang |
| `PARSE_EXPECT` | error | parse | A token that the grammar does not permit at that position. | test/json/PARSE_EXPECT.lang |
| `PARSE_DEPTH` | error | parse | The terms nest deeper than the parser limit. | test/json/PARSE_DEPTH.lang |
| `PARSE_CLAUSE_NAME` | error | parse | A clause name is not the name of the signature above it, or a clause has no signature. | test/json/PARSE_CLAUSE_NAME.lang |
| `PARSE_CTOR_RESULT` | error | parse | A constructor type does not end in the data type applied to its parameters in order, or a field has no name. | test/json/PARSE_CTOR_RESULT.lang |
| `REFUSE_DATA` | error | check | A program declares a data type with no type argument in its parameters or indices (rule R2). Only the domain can declare a base type. | test/json/REFUSE_DATA.lang |
| `REFUSE_CELL` | error | check | A program data type with a type argument is not a cell signature (rule R1). A cell signature has only universe parameters and no type index. Each field is a type parameter (then it is the one field of its constructor), closed data, the family at its own parameters or a path in the family. Each type parameter has exactly one generator: a point constructor with one field of that type. A path constructor has the type `Path T s t`, where T is the family at its own parameters and s, t are point terms; S2 has dimension 1 only, so a square is refused. A program `codata` type is a cocell signature (the CC criterion): one type parameter, no index and one clause that is not a path constructor. Each field is the observation A, a field D A or a labelled field E → D A, where E is closed and E is in the universe of A. There is exactly one observation. | test/json/REFUSE_CELL.lang |
| `REFUSE_AXIOM` | error | check | A program declares a postulate. | test/json/REFUSE_AXIOM.lang |
| `REFUSE_REC` | error | check | A `rec` declaration, or a definition that refers to itself or to a later definition (rule R3). Use `fold`. | test/json/REFUSE_REC.lang |
| `REFUSE_NO_MERGE` | error | check | A `state` line of a `replicated` form has a part type with no merge (J1). A part type is `GSet T` or `MVReg T`, where T is an atoms name, `Counter` (the one part type with an `init`) or `Escrow`. A `replicated` form with no `state` line is also refused. An `allocate` on a part that is not a Counter, or a second `allocate` on a part, also gives this code (an Escrow part has no `init`). The span is the `state` word, the `replicated` word (no `state` line) or the `allocate` word. | test/json/REFUSE_NO_MERGE.lang |
| `REFUSE_OP_INFINITE_ARG` | error | check | An `op` parameter type is not finite (J1). An `op` parameter type is an atoms name, `Replica` or `Fin m` with a numeral m. The span is the parameter name. | test/json/REFUSE_OP_INFINITE_ARG.lang |
| `REFUSE_RAW_WRITE` | error | check | An `op` body is not one part operation `WORD PART ARG..` (J2, Q35 (a)): `add p x` on a GSet, `write p x` on an MVReg, `dec p k` or `restock p k` on a Counter, `dec p k`, `transfer p j k` or `restock p k` on an allocated Counter. PART is a `state` name of the form. An `invariant` line that is not `nonneg PART` on a Counter or an allocated Counter also gives this code (Q37 (a)). The span is the `op` word, or the `invariant` word. | test/json/REFUSE_RAW_WRITE.lang |
| `REFUSE_ALLOC_SUM` | error | check | The shares of an `allocate` line do not sum to the init of its Counter part, or a replica has no share or two shares (J1). An `allocate` on a part that is not a Counter (or a second `allocate`) gives `REFUSE_NO_MERGE`. The span is the `allocate` word. | test/json/REFUSE_ALLOC_SUM.lang |
| `REFUSE_NEEDS_COORDINATION` | error | check | An op pair of a replicated form breaks an invariant after the merge (J4): the bounded search finds a schedule (prefix 1, 2 ops for each replica, the declared replicas) where the merged state makes the invariant false. The message gives the pair and the schedule. `found` is the merged state. `fix` tells you to declare an allocation rule or to remove the invariant. The span is the `op` word of the first op of the pair. | test/json/REFUSE_NEEDS_COORDINATION.lang |
| `REFUSE_READ_WINNER` | error | check | An argument of a part operation in an `op` is the name of an MVReg read `x ← read p`. An MVReg read is a `Read` set, not a value: fold over `Read` (`readEmpty`, `readOne u`, `readConflict s`). The span is the argument and `found` is its name. A GSet read in that position gives `TYPE_MISMATCH`. | test/json/REFUSE_READ_WINNER.lang |
| `REFUSE_RULE_MISMATCH` | error | check | The `for` clause of an `allocate` line names an invariant that L7 does not prove (J6). An escrow rule on c covers `nonneg c` only, and the clause is `nonneg d` with d not c. `expected` is c and `found` is d. A clause that is not `nonneg d` on a Counter or an allocated Counter gives `REFUSE_RAW_WRITE`, and an unknown d gives `NAME_UNKNOWN`. The span is the `for` word. | test/json/REFUSE_RULE_MISMATCH.lang |
| `DOMAIN_FAMILY` | error | check | A family or a fold is outside the S2 data rules. A recursive field must have the type of its family, at the parameters of the family. A fold over an indexed family or a higher inductive family needs a dependent eliminator (S3). A path constructor is only in a cell signature. | test/json/DOMAIN_FAMILY.lang |
| `REFUSE_NAME` | error | check | A declaration uses a core name, a domain name, a reserved word or a name that is already declared. A `codata` field must not be the type name, the clause name or a name that is already a field of the clause. Each `state` name in a `replicated` form must be distinct. | test/json/REFUSE_NAME.lang |
| `TYPE_REFL` | error | check | `refl` where the two sides do not have the same normal form. The message gives the two normal forms. | test/json/TYPE_REFL.lang |
| `TYPE_MISMATCH` | error | check | A term does not have the type that its position needs. | test/json/TYPE_MISMATCH.lang |
| `NAME_UNKNOWN` | error | check | A name that is not in scope. The span is the name. For a kit name that S1 or S2 removed, the message says what to use. `fix` is the new name for `Flag` (`Bool`), `flagYes` (`true`), `flagNo` (`false`), `pair` (`pack`), `first` (`witness`), `second` (`payload`) and `Eq` (`Path`). `fix` is `null` for `Prod`, `List`, `nil`, `cons`, `filter` and `unfold`, because a name cannot replace them. | test/json/NAME_UNKNOWN.lang |
| `TYPE_UNIVERSE` | error | check | A universe level that is out of range. | test/json/TYPE_UNIVERSE.lang |
| `TYPE_INFER` | error | check | The checker cannot find the type of a term at that position. Add a type: an annotated binder or a signature. | test/json/TYPE_INFER.lang |
| `TYPE_ARITY` | error | check | A term has the wrong number of arguments. | test/json/TYPE_ARITY.lang |
| `TYPE_NOT_FUNCTION` | error | check | An application of a term that is not a function. | test/json/TYPE_NOT_FUNCTION.lang |
| `TYPE_PRETYPE` | error | check | A pretype where the checker needs a type in a universe: the type of a definition, a field or a parameter, a part of a Σ type, or the type argument of `Path` or `Option`. The pretypes are `I` and the function types over `I`. Their sort is SSet, which is in no universe. | test/json/TYPE_PRETYPE.lang |
| `TYPE_ENDPOINT` | error | check | A path `λ i → t` where `t` at `i0` or at `i1` does not have the same normal form as the end that the `Path` or `PathP` type gives. The message gives the two normal forms. | test/json/TYPE_ENDPOINT.lang |
| `TYPE_SYSTEM` | error | check | A system `[ φ ↦ u , ... ]` that does not agree with its `Partial ψ A` type: the join of the cell faces is not `ψ`, two branches do not have the same normal form where their faces meet, a face overlap is not in the face bounds (16 meets of 8 literals), or the system has more than 16 cells. An `hcomp` face that depends on its side binder is also refused. The message identifies the failing condition. | test/json/TYPE_SYSTEM.lang |
| `TYPE_LINE` | error | check | A `transp A r u` where the line `A` is not constant where `r` holds. The checker compares `A i` with `A i0` on each meet of `r` and of the faces of the enclosing system cells. An atom in a meet stays free, so the test is conservative there. A face that is not in the face bounds needs a constant line. The message gives the line. | test/json/TYPE_LINE.lang |
| `W_BOUNDED` | warning | check | An op pair of a replicated form with an invariant that no library law covers (L6a, L6b, L7), and the bounded search finds no violation on its K schedules (prefix 1, 2 ops for each replica, the declared replicas). One record for each such pair, at the `op` word of the first op. The mark of the pair is bounded, not proved. | test/json/W_BOUNDED.lang |
| `W_NONCANON` | warning | check | The first declaration (or the tail of the file) whose text is not the `langc fmt` text. `found` is the input text; `expected` and `fix` are the canonical text. | test/json/W_NONCANON.lang |
| `REFUSE_RUNTIME_CLOSURE` | error | lowering | A function value must exist at run time. The Wasm target cannot hold one (HOST-LIMIT). | no fixture: `langc ir` and `langc build` have no JSON mode; test/gate.sh checks test/ir/closure.lang |
| `REFUSE_RUNTIME_TREE` | error | lowering | A fold over a tree must run at run time. The Wasm target cannot run it (HOST-LIMIT). | no fixture: `langc ir` and `langc build` have no JSON mode; test/gate.sh checks test/ir/tree.lang |
| `REFUSE_RUNTIME_PATH` | error | lowering | An interval point or a path value must exist at run time, for example in the state of a fold. A path is a proof. It has no run-time form. | no fixture: `langc ir` and `langc build` have no JSON mode; test/gate.sh checks test/ir/path.lang |
| `EVAL_ARGS` | error | eval | The arguments of `langc eval` do not agree with the entry (count or range). | no fixture: `langc eval` has no JSON mode; test/gate.sh checks it (`eval_refused`) |
| `EVAL_ENTRY` | error | eval | `langc eval` names no definition, or the entry does not compute a number. | no fixture: `langc eval` has no JSON mode; test/gate.sh checks it (`eval_refused`) |
| `EVAL_PRINT` | error | eval | The value is too large to print. | no fixture: `langc eval` has no JSON mode; test/review.sh checks large.lang |
| `EVAL_OVERFLOW` | error | eval | A ℕ operation overflows past 2^64-1. | no fixture: `langc eval` has no JSON mode; the `trap` lines of test/eval/expect.txt check it |
| `EVAL_DEPTH` | error | eval, lowering | The evaluation or the lowering nests deeper than 2000 levels. | no fixture: no S1 `check` input reaches this limit, and `langc eval` has no JSON mode |
| `EVAL_FUEL` | error | eval | The evaluation takes more than 20000000 steps. | no fixture: a fixture needs more than 20000000 steps, which is too slow for the gate |
| `INTERNAL` | error | check, eval, lowering | An internal rule of `langc` fails. This is a defect in `langc`. | no fixture: no valid input reaches it |
| `IO` | error | all | `langc` cannot read the program file or write its output. | no fixture: the cause is the file system, not the program text; test/gate.sh checks a missing file (exit 2) |
| `IO_SIZE` | error | all | The program file is larger than the size limit. | no fixture: the fixture would be larger than the size limit |
| `OOM` | error | all | `langc` has no free memory. | no fixture: no program text causes it reliably |
| `USAGE` | error | all | The command line is not valid. | no fixture: the cause is the command line, not the program text; test/gate.sh checks exit 2 |
