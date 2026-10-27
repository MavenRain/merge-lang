# merge-lang domain design

This file is the only source of the core types and the core operations of
merge-lang. `docs/PLAN.md` sections 2, 3 and 4 cite it. Do not add a core
type or a core operation that this file and the PLAN admission table (PLAN
section 3) do not give.

Section 1 is the USER text of 2026-10-27, verbatim. Do not edit section 1.
Section 2 holds the USER rulings of the same day as recorded at the time.
Section 3 holds the research facts that the target question used. The
sources are in `merge-lang-work/USER-ASK.md` outside this repository.

## 1. The USER ask (2026-10-27, verbatim)

I want to write a new programming language with the following characteristics:

For type constructors/higher-kinded types, admit only cell monads and their duals as represented computationally in Cubical Type Theory.

For base types and operations, admit only what is necessary to represent the following: 

```
merge-lang: offline data with checked merge laws
The problem: Replicas can converge to the same answer while still producing an answer the application considers wrong. A substantial HN discussion about local-first synchronization makes this distinction. That discussion is older, from 2024, so I treat it as evidence of a persistent design problem rather than a new trend.
The language: Describe replicated state, permitted operations, merge behavior, and domain invariants.
Examples:
* Concurrent tag additions should both survive.
* Concurrent assignment of a task owner should remain an explicit conflict.
* An offline inventory decrement needs a resource allocation rule if stock must never become negative.
Why it fits you: This is probably the richest mathematical direction. You could express merge laws and prove selected invariants, while distinguishing operations that work independently from those requiring coordination.
First target: A Wasm merge engine and deterministic replica simulator. Existing Automerge conflict semantics offer a useful comparison.
First compelling demo: Two disconnected replicas make changes, reconnect, and either preserve the declared invariant or produce a concrete counterexample explaining why coordination is necessary.
This would occupy a distinct space from dil-lang’s reactive interfaces and tether’s Redis scripts.
```

The host language shall be TinyCC.  Use your judgment to provide multiple-choice options for compilation target(s).

Finally, I'd like agents to reach for this language when it fits their use cases.  To that end, also adhere to the following criteria: 

```
  The plan for making agents reach for the language includes:
  - one canonical form per operation and a langc fmt canonical printer;
  - langc check --json, giving fix hints and progress counts (parsed, typed, laws
    discharged);
  - a SKILL.md with 3 to 5 graded examples;
  - an MCP server written in TinyCC (langc mcp);
  - AGENTS.md and a README table saying when to use the language;
  - a token-count comparison against Python and Rust.

## 2. Rulings (2026-10-27, AskUserQuestion)

- Q "Which compilation targets should merge-lang v1 emit?" (multi-select) = "Wasm merge engine (Recommended), Replica simulator (Recommended), JSON coordination report (Recommended)". NOT chosen: "Automerge differential harness".
- Q "What form should the first demo (two offline replicas reconnect, invariant kept or counterexample shown) take?" = "CLI transcript (Recommended)". NOT chosen: "Browser page", "Both".

Option texts as offered (Claude's words):
- Wasm merge engine: `langc build` emits a .wasm module that exports apply(op), merge(a, b) and check(state). The same module runs in a browser, Node or wasmtime. It is seeded from the lang-template tcc-wasm kit and its differential gate. This is the first target the brief names.
- Replica simulator: `langc sim`: a deterministic simulator. It runs the engine on N replicas over every partition and reconnect schedule up to a bound, then reports either 'invariant held on all K schedules' or a minimal counterexample trace. Native C built by tcc; the same core can also compile to Wasm.
- JSON coordination report: One row per operation pair: commutes, explicit conflict, or needs coordination (with the allocation rule, or the counterexample). CI and agents read it, and `check --json` shares its schema. Built on the tcc-json kit writer.
- Automerge differential harness (NOT chosen): Emits a JS adapter (tcc-js kit) that maps merge-lang state onto an @automerge/automerge 3.x document, runs the same schedules in Node and diffs the results. Needs a one-time npm install (network), so it stays outside the default offline gate.
- CLI transcript: `langc sim examples/inventory.merge` prints the schedule, the merged state and either 'invariant kept' or the counterexample plus the coordination it needs. It is fully offline, and the gate byte-compares it as a golden file.

Correction to an option text (Claude, 2026-10-27): "the same core can also compile to Wasm" was loose. TinyCC has no Wasm backend (evolve-lang S0 finding), so the simulator is native C in langc. It drives the emitted Wasm engine (wasmtime or node) as the differential leg; the simulator itself is not Wasm.

## 3. Research facts (2026-10-27)

- Automerge JS API v3.1.1: concurrent "put" operations to one map key make the value conflicted; Automerge "chooses one value arbitrarily (but deterministically)"; a plain read gives last-writer-wins, where "last" is the internal operation ID, not wall-clock time; `getConflicts` returns the winner and the losers keyed by the op IDs that wrote them; the next assignment to the key resolves the conflict. Sources: https://automerge.org/automerge/api-docs/js/functions/getConflicts.html , https://automerge.org/docs/reference/documents/conflicts/ . Reading: Automerge keeps the conflict but a plain read hides it; merge-lang can make the conflict a value of the read type, so a program cannot silently take a winner.
- The 2024 HN thread: NOT found by two web searches (2026-10-27). The description dates it 2024; cite it only as "an HN discussion (2024)" unless a later session finds the URL.
- Invariant confluence: Bailis et al., "Coordination Avoidance in Database Systems" (arXiv 1402.2237, VLDB 2015): I-confluence is necessary and sufficient for invariant preservation without coordination; a non-I-confluence proof presents "two I-T-reachable states with a common ancestor that, when merged, are not I-valid". This is the counterexample shape of the first demo. Source: https://arxiv.org/pdf/1402.2237
- Whittaker and Hellerstein: an interactive I-confluence decision procedure (Lucy prototype; foreign keys, escrow). Source: https://blog.acolyer.org/2019/08/28/interactive-checks-for-coordination-avoidance/
- Bounded counter (Balegas et al., arXiv 1503.09052): escrow for CRDTs; rights matrix R (R[i][i] = increments at replica i, R[i][j] = rights moved from i to j) and decrement vector U; a replica decrements locally while it holds rights; when its rights are exhausted a decrement fails or needs to synchronize with a replica that holds rights. Sources: https://arxiv.org/pdf/1503.09052 , https://arxiv.org/pdf/1806.10254 (CRDT overview, 2019). This is a candidate "resource allocation rule" for the inventory example.
- Related tools: Just-Right Consistency (arXiv 1801.06340; CISE, CEC, Repliss), IPA, VeriFx (arXiv 2207.02502; verifies RDTs, finds finite counterexamples), ConLoc (FSE 2025 journal-first, "Consistent Local-First Software: Enforcing Safety and Invariants for Local-First Applications"), TU Darmstadt preprint 2024-08-20 (agreement without coordination). Sources: https://arxiv.org/pdf/2207.02502 , https://conf.researchr.org/details/fse-2025/fse-2025-journal-first/48/Consistent-Local-First-Software-Enforcing-Safety-and-Invariants-for-Local-First-Appl , https://www.st.informatik.tu-darmstadt.de/preprints/2024-08-20-preprint-PLF-agreement-without-coordination.pdf
- Local precedent: /Users/oobi/Documents/automerge-cat (USER Rust crate: CRDTs as join-semilattices whose merge is a colimit in the posetal category of states, on comp-cat-rs; HEAD 61aa688). /Users/oobi/Documents/dil-lang and /Users/oobi/Documents/tether exist (the "distinct space" the description names).
- Local tools 2026-10-27: tcc /Users/oobi/.local/bin/tcc; node v23.10.0 + npm; wasmtime /opt/homebrew/bin/wasmtime; wasm-validate + wat2wasm; clang /usr/bin/clang; lean + lake (elan). lang-template HEAD 5265a2a, hosts: tcc-wasm, tcc-json, tcc-js, tcc-evm*, tcc-media, tcc-webgpu, assay, mech.
- R1 wording differs from the earlier asks: this ask says "cell monads and their duals as represented computationally in Cubical Type Theory" and drops "in a locally presentable locally cartesian closed (infinity,1)-category". Reading: same precedent design (REFUSE_CELL, cocell/cofamily); the dropped clause named the ambient model, not an extra former.
