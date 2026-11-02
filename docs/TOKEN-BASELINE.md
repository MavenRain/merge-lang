# Token baseline (S9)

This file compares the tokens of one task in merge-lang, Python and Rust
(PLAN 7, row 6). The tool `build/tokcount` writes the table. The gate
makes the table again and compares it with this file.

## The task

The task is the inventory with escrow of PLAN 4.4 (D5). Two replicas `A`
and `B` share a stock counter with an initial stock of 1. The operation
`sell k` decrements the stock and the operation `refill k` restocks it.
The amount `k` is 1 or 2. The invariant `nonneg stock` says that the
stock is never below zero. The escrow rule splits the initial stock into
rights: `A` gets 1 and `B` gets 0. A replica sells only from its own
rights, so the invariant holds after a merge. Each file states the
replicas, the operations, the invariant and the escrow rule. Each file
also checks the merge laws and the invariant.

## The files

- `examples/inventory-escrow.merge`: the merge-lang program. `langc
  check` is its checker. It checks the merge laws and the invariant on a
  bounded domain: a synced prefix of 1 operation, then 2 operations for
  each replica, then the merge.
- `test/baseline/inventory.py`: the same task in Python. It has the data
  of the shop, a model of the operations and of the merge, and 4 tests
  (`def test_`) on the same bounded domain.
- `test/baseline/inventory.rs`: the same task in Rust. It has the same
  data, the same model and 4 tests (`#[test]`).

The checker of merge-lang is in `langc`, so the Python and Rust samples
carry their own check. The gate counts the two samples. It does not run
them, and it does not build the Rust sample.

## The method

Each count is a fixed offline proxy, not the tokenizer of a model (PLAN
Q19 (a)). The tool counts all of each file, comments too, with two rules:

- rule A (column `tokens A`): a run of ASCII letters, digits and `_` is 1
  token. Rule A is the rule of leaf-lang 2fa3ea9 `test/tokcount.c`, so
  column A compares with leaf-lang.
- rule B (column `tokens B`): a run of length L is ceil(L / 4) tokens,
  that is (L + 3) / 4. Rule B is the word rule of quorum-lang 0c729b6
  `test/tokproxy.c`, so column B compares with quorum-lang.

In both rules, each other ASCII byte that is not white space is 1 token,
each byte from 0x80 is 1 token, and white space and control bytes are 0.
The column `code points` counts the bytes below 0x80 or from 0xc0.

The column `non-ASCII tokens` counts the tokens from the bytes from 0x80.
This count is the same under rule A and rule B. It gives the cost of the
Unicode surface (PLAN 7, row 6). In `examples/inventory-escrow.merge`,
the symbol `∣` (line 4) and the two symbols `↦` (line 11) are 3 UTF-8
bytes each, so they are 9 tokens under both rules.

merge-lang has no ASCII spelling of these symbols (PLAN Q93 (a)), so the
table has no ASCII row. Under the proxy, an ASCII spelling `|->` for `↦`
is 3 tokens, the same as the 3 tokens of `↦`. An ASCII spelling `|` for
`∣` is 1 token, 2 fewer than the 3 tokens of `∣`. Thus an ASCII spelling
of the program has 2 fewer tokens under each rule.

## The table

From the repo root, this command writes the lines between the two markers
below:

```
build/tokcount examples/inventory-escrow.merge test/baseline/inventory.py test/baseline/inventory.rs
```

The gate runs the check command:

```
build/tokcount --check docs/TOKEN-BASELINE.md examples/inventory-escrow.merge test/baseline/inventory.py test/baseline/inventory.rs
```

The check makes the same text and compares it with the lines between the
markers, byte for byte. It also checks the half-up rule on fixed pairs
and runs `langc check` on the `.merge` file.

The document must be text without embedded NUL bytes. An invalid document,
a read error or a write error exits 2. `make check` also runs the C CLI
regressions in `test/tokcount-io.c`, including an unwritable stdout in
both modes and a document with extra bytes after a NUL.

<!-- tokcount begin -->
| file | bytes | code points | tokens A | tokens B | non-ASCII tokens |
| :--- | ---: | ---: | ---: | ---: | ---: |
| examples/inventory-escrow.merge | 312 | 306 | 83 | 110 | 9 |
| test/baseline/inventory.py | 3071 | 3071 | 847 | 1068 | 0 |
| test/baseline/inventory.rs | 5345 | 5345 | 1569 | 1856 | 0 |

ratio of the tokens A (test/baseline/inventory.py over examples/inventory-escrow.merge): 10.20
ratio of the tokens B (test/baseline/inventory.py over examples/inventory-escrow.merge): 9.71
ratio of the tokens A (test/baseline/inventory.rs over examples/inventory-escrow.merge): 18.90
ratio of the tokens B (test/baseline/inventory.rs over examples/inventory-escrow.merge): 16.87
share of the non-ASCII tokens in the tokens A of examples/inventory-escrow.merge: 0.11
share of the non-ASCII tokens in the tokens B of examples/inventory-escrow.merge: 0.08
<!-- tokcount end -->

The ratio lines below the table give the result. For each file after
`examples/inventory-escrow.merge`, one line divides its tokens A by the
tokens A of `examples/inventory-escrow.merge`, and the next line does the
same for tokens B (half up, 2 decimals). The two share lines divide the
non-ASCII tokens of `examples/inventory-escrow.merge` by its tokens A and
by its tokens B (half up, 2 decimals). The share lines do not compare two
files. Each file is a full alternative, so the table has no total row.
