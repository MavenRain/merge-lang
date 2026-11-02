---
name: merge-lang
description: Write, check and repair merge-lang programs (a cubical language for offline replicated state with checked merge laws and a coordination class for each pair of concurrent operations) with the langc checker. Use it when the task is in a yes row of the merge-lang README table.
---

# merge-lang

## Routing rule

Use merge-lang when the task is in a "yes" row of the README table
(`## When to use merge-lang`): offline-first replicated state with domain
invariants, a tag set where concurrent additions must both stay, an owner
field where concurrent writes must stay an explicit conflict, a stock that
must never become negative after offline decrements. Do not use
merge-lang for text co-editing: merge-lang has no sequence or text type.
Do not use it for a server-only database: one server applies its own
constraints and needs no merge. For Redis scripts, use tether.

## The loop

1. Write FILE.merge: the `replicas` line, the `atoms` declarations, and
   one `replicated` form for each replicated value. A form has its
   `state` parts, its `op` declarations, its `invariant` lines and, for a
   part that an invariant bounds, an `allocate PART by escrow (...)` rule.
2. Run `build/langc fmt FILE.merge`. When the output is not the same as
   FILE.merge, write the output to a new file and replace FILE.merge with
   it.
3. Run `build/langc check --json FILE.merge`. The stdout is one JSON
   document. `progress` gives the counts, `pairs` gives one row for each
   classified pair of operations of each replicated form, and
   `diagnostics` gives the diagnostics (docs/DIAGNOSTICS.md). Exit 0: the
   merge laws hold and each pair has a class. Exit 1: the checker refuses
   the file. Exit 2: usage or IO.
4. A row gives the class (`independent`, `explicit-conflict` or
   `needs-coordination`) and the mark (`proved`, `bounded` or `refuted`).
   A refused row also gives the code, the fix and the counterexample: the
   ancestor state, the operations of each replica, the merged state and
   the values that break the invariant.
5. A diagnostic with `"retry":true`: change the file at its `span`. On
   `REFUSE_NEEDS_COORDINATION`, run `build/langc sim FILE.merge`. The
   `fix:` line of its output gives an allocation rule for the part. Add
   the rule, or change the operation, or remove the invariant. Then do
   step 3 again.
6. A diagnostic with `"retry":false`: a change to the program text cannot
   remove it. Correct the command or the environment.
7. When the check gives exit 0, run `build/langc build FILE.merge -o
   OUT.wasm` for the Wasm merge engine. It also writes the JSON document
   to `OUT.report.json`.

## Verbs

- `langc check [--json] PROG`: check the program. Text: `ok`, or one
  line on stderr. JSON: the document of step 3 on stdout.
- `langc fmt PROG`: print the canonical text of PROG.
- `langc sim PROG [--replicas N] [--prefix P] [--ops M]`: the
  deterministic replica simulator. It counts the schedules of each
  replicated form in the bounds. For a refuted invariant, it gives the
  schedule and a `fix:` line.
- `langc build PROG -o OUT [--runtime]`: write the Wasm merge engine to
  OUT and the JSON document to `OUT.report.json`, also when the checker
  refuses the program.
- `langc eval PROG NAME [ARGS...]`, `langc ir PROG` and `langc abi
  PROG`: the kit back end.
- `langc explain-code CODE`: the row of CODE in docs/DIAGNOSTICS.md as
  one JSON line (`code`, `severity`, `stage`, `meaning`). An unknown
  code gives exit 1.
- `langc mcp`: a JSON-RPC 2.0 server on stdin and stdout, one message on
  each line. Its 4 tools are `check` (`path`), `fmt` (`path`), `sim`
  (`path`, and the integers `replicas`, `prefix` and `ops`) and
  `explain-code` (`code`). A tool result has the stdout of the verb as
  its first text item, and the stderr as a second item when it is not
  empty. `isError` is true when the exit code is not 0.

## Examples

The examples go from easy to hard. Run each command from the repository
root. The gate (`build/skill-test`) runs each program and compares the
output with the block below it.

### 1. Tags: independent additions

Two replicas add tags to one grow-only set. Two concurrent `tag`
operations commute, so the pair is independent and the check passes.

```merge-lang examples/tags.merge
-- Tags (D3): two concurrent tag ops are independent.
replicas A B

atoms Tag := urgent ∣ home ∣ work

replicated Tags
  state tags : GSet Tag
  op tag (t : Tag) := add tags t
```

`build/langc check --json examples/tags.merge` gives:

```json
{"version":1,"file":"examples/tags.merge","replicas":["A","B"],"bounds":{"prefix":1,"ops":2},"progress":{"parsed":"3 of 3","typed":"3 of 3","laws":{"total":7,"proved":7,"bounded":0,"refuted":0},"pairs":{"total":1,"classified":1}},"pairs":[{"form":"Tags","a":"tag","b":"tag","class":"independent","part":null,"invariant":null,"rule":null,"mark":"proved","code":null,"fix":null,"counterexample":null}],"diagnostics":[]}
```

The exit code is 0.

### 2. Task owner: an explicit conflict

Two replicas write the owner of one task. The `MVReg` part keeps the two
concurrent writes as an explicit conflict, so the check passes and the
row has the class `explicit-conflict`.

```merge-lang examples/owner.merge
-- Task owner (D4): two concurrent assign ops are an explicit conflict.
replicas A B

atoms User := ana ∣ ben

replicated Task
  state owner : MVReg User
  op assign (u : User) := write owner u
```

`build/langc check --json examples/owner.merge` gives:

```json
{"version":1,"file":"examples/owner.merge","replicas":["A","B"],"bounds":{"prefix":1,"ops":2},"progress":{"parsed":"3 of 3","typed":"3 of 3","laws":{"total":7,"proved":7,"bounded":0,"refuted":0},"pairs":{"total":1,"classified":1}},"pairs":[{"form":"Task","a":"assign","b":"assign","class":"explicit-conflict","part":"owner","invariant":null,"rule":null,"mark":"proved","code":null,"fix":null,"counterexample":null}],"diagnostics":[]}
```

The exit code is 0.

### 3. Inventory: a refused sale

Two replicas sell from one stock. The form has `invariant nonneg stock`
and no allocation rule. Two concurrent sales can take the merged stock
below 0, so the checker refuses `sell ∥ sell` with
`REFUSE_NEEDS_COORDINATION`. The counterexample row gives the schedule.

```merge-lang examples/inventory.merge
-- Inventory (D5): two concurrent sell ops need coordination.
replicas A B

atoms Amount := 1 ∣ 2

replicated Shop
  state stock : Counter (init 1)
  op sell (k : Amount) := dec stock k
  op refill (k : Amount) := restock stock k
  invariant nonneg stock
```

`build/langc check --json examples/inventory.merge` gives:

```json
{"version":1,"file":"examples/inventory.merge","replicas":["A","B"],"bounds":{"prefix":1,"ops":2},"progress":{"parsed":"3 of 3","typed":"2 of 3","laws":{"total":9,"proved":6,"bounded":0,"refuted":1},"pairs":{"total":3,"classified":1}},"pairs":[{"form":"Shop","a":"sell","b":"sell","class":"needs-coordination","part":null,"invariant":"nonneg stock","rule":null,"mark":"refuted","code":"REFUSE_NEEDS_COORDINATION","fix":"declare an allocation rule, for example `allocate stock by escrow (...)`, or remove the invariant","counterexample":{"bounds":{"prefix":1,"ops":2,"replicas":2},"ancestor":{"ops":[],"state":"stock 1 (init 1)"},"branches":[{"replica":"A","ops":[{"op":"sell","args":["1"],"state":"stock 0 (init 1, decrements A 1)"}]},{"replica":"B","ops":[{"op":"sell","args":["1"],"state":"stock 0 (init 1, decrements B 1)"}]}],"merged":"stock -1 (init 1, decrements A 1, B 1)","invariant":{"name":"nonneg stock","values":"stock = -1"},"row":{"class":"needs-coordination","code":"REFUSE_NEEDS_COORDINATION","fix":"declare an allocation rule, for example `allocate stock by escrow (...)`, or remove the invariant"}}}],"diagnostics":[{"code":"REFUSE_NEEDS_COORDINATION","severity":"error","span":{"file":"examples/inventory.merge","line":8,"col":3,"end_line":8,"end_col":5},"expected":null,"found":"pack (λ x → 0) (λ x → natMax (flagIf (natEq (witness x) 0) 1 0) (flagIf (natEq (witness x) 1) 1 0))","fix":"declare an allocation rule, for example `allocate stock by escrow (...)`, or remove the invariant","retry":true,"message":"sell ∥ sell: the pair breaks an invariant after merge; the schedule A: sell 1; B: sell 1"}]}
```

The exit code is 1.

### 4. Inventory with escrow: a kept invariant

The same shop with `allocate stock by escrow (A ↦ 1, B ↦ 0)`. Each
replica sells only from its own rights, so the invariant holds after
each merge. Each pair is independent, and the escrow rule proves it.

```merge-lang examples/inventory-escrow.merge
-- Inventory with escrow (D5): each replica sells only its own rights.
replicas A B

atoms Amount := 1 ∣ 2

replicated Shop
  state stock : Counter (init 1)
  op sell (k : Amount) := dec stock k
  op refill (k : Amount) := restock stock k
  invariant nonneg stock
  allocate stock by escrow (A ↦ 1, B ↦ 0)
```

`build/langc check --json examples/inventory-escrow.merge` gives:

```json
{"version":1,"file":"examples/inventory-escrow.merge","replicas":["A","B"],"bounds":{"prefix":1,"ops":2},"progress":{"parsed":"3 of 3","typed":"3 of 3","laws":{"total":7,"proved":7,"bounded":0,"refuted":0},"pairs":{"total":3,"classified":3}},"pairs":[{"form":"Shop","a":"sell","b":"sell","class":"independent","part":null,"invariant":"nonneg stock","rule":"allocate stock by escrow (A ↦ 1, B ↦ 0)","mark":"proved","code":null,"fix":null,"counterexample":null},{"form":"Shop","a":"sell","b":"refill","class":"independent","part":null,"invariant":"nonneg stock","rule":"allocate stock by escrow (A ↦ 1, B ↦ 0)","mark":"proved","code":null,"fix":null,"counterexample":null},{"form":"Shop","a":"refill","b":"refill","class":"independent","part":null,"invariant":"nonneg stock","rule":"allocate stock by escrow (A ↦ 1, B ↦ 0)","mark":"proved","code":null,"fix":null,"counterexample":null}],"diagnostics":[]}
```

The exit code is 0.

### 5. Seats: three replicas, a transfer and a tag set

Three box offices sell the 2 seats of one hall. The Hall form has the
rule `allocate seats by escrow (A ↦ 1, B ↦ 1, C ↦ 0)` and the operation
`give k := transfer seats C k`. The Door form has the same sales and a
tag set, but no rule.

The checker refuses `sell ∥ sell` in Door. Its counterexample row is the
schedule: A `sell 1` (seats 1), B `sell 2` (seats 0), C no operations,
merged seats -1. In Hall, the rights bound the same sales (A 1, B 1, C
0). B `sell 2` fails locally, because the rights of B are 1 and 1 < 2.
A sale at C needs `give 1` at A or B first, that is `transfer seats C
1`. The `fix` text of the row names `stock` for each part (it is one
constant text); the part here is `seats`.

```merge-lang examples/agent/seats.merge
-- Seats: three box offices sell the seats of one hall.
replicas A B C

atoms Amount := 1 ∣ 2

atoms Tag := vip ∣ late

replicated Hall
  state seats : Counter (init 2)
  op sell (k : Amount) := dec seats k
  op give (k : Amount) := transfer seats C k
  invariant nonneg seats
  allocate seats by escrow (A ↦ 1, B ↦ 1, C ↦ 0)

replicated Door
  state seats : Counter (init 2)
  state tags : GSet Tag
  op sell (k : Amount) := dec seats k
  op tag (t : Tag) := add tags t
  invariant nonneg seats
```

`build/langc check --json examples/agent/seats.merge` gives:

```json
{"version":1,"file":"examples/agent/seats.merge","replicas":["A","B","C"],"bounds":{"prefix":1,"ops":2},"progress":{"parsed":"5 of 5","typed":"4 of 5","laws":{"total":16,"proved":14,"bounded":0,"refuted":1},"pairs":{"total":6,"classified":4}},"pairs":[{"form":"Hall","a":"sell","b":"sell","class":"independent","part":null,"invariant":"nonneg seats","rule":"allocate seats by escrow (A ↦ 1, B ↦ 1, C ↦ 0)","mark":"proved","code":null,"fix":null,"counterexample":null},{"form":"Hall","a":"sell","b":"give","class":"independent","part":null,"invariant":"nonneg seats","rule":"allocate seats by escrow (A ↦ 1, B ↦ 1, C ↦ 0)","mark":"proved","code":null,"fix":null,"counterexample":null},{"form":"Hall","a":"give","b":"give","class":"independent","part":null,"invariant":"nonneg seats","rule":"allocate seats by escrow (A ↦ 1, B ↦ 1, C ↦ 0)","mark":"proved","code":null,"fix":null,"counterexample":null},{"form":"Door","a":"sell","b":"sell","class":"needs-coordination","part":null,"invariant":"nonneg seats","rule":null,"mark":"refuted","code":"REFUSE_NEEDS_COORDINATION","fix":"declare an allocation rule, for example `allocate stock by escrow (...)`, or remove the invariant","counterexample":{"bounds":{"prefix":1,"ops":2,"replicas":3},"ancestor":{"ops":[],"state":"seats 2 (init 2)   tags λ x → false"},"branches":[{"replica":"A","ops":[{"op":"sell","args":["1"],"state":"seats 1 (init 2, decrements A 1)   tags λ x → false"}]},{"replica":"B","ops":[{"op":"sell","args":["2"],"state":"seats 0 (init 2, decrements B 2)   tags λ x → false"}]},{"replica":"C","ops":[]}],"merged":"seats -1 (init 2, decrements A 1, B 2)   tags λ x → false","invariant":{"name":"nonneg seats","values":"seats = -1"},"row":{"class":"needs-coordination","code":"REFUSE_NEEDS_COORDINATION","fix":"declare an allocation rule, for example `allocate stock by escrow (...)`, or remove the invariant"}}}],"diagnostics":[{"code":"REFUSE_NEEDS_COORDINATION","severity":"error","span":{"file":"examples/agent/seats.merge","line":18,"col":3,"end_line":18,"end_col":5},"expected":null,"found":"pack (pack (λ x → 0) (λ x → natMax (flagIf (natEq (witness x) 0) 1 0) (flagIf (natEq (witness x) 1) 2 0))) (λ x → false)","fix":"declare an allocation rule, for example `allocate stock by escrow (...)`, or remove the invariant","retry":true,"message":"sell ∥ sell: the pair breaks an invariant after merge; the schedule A: sell 1; B: sell 2"}]}
```

The exit code is 1.
