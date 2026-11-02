"""Inventory with escrow (D5): each replica sells only its own rights.

The Python form of examples/inventory-escrow.merge. Two replicas share a
stock counter. The initial stock is split into rights per replica. A
replica sells only from its own rights, so the stock is never below zero
after a merge. The tests check the merge laws and the invariant on the
bounded domain of `langc check`: a synced prefix of 1 operation, then 2
operations per replica, then the merge.
"""

from dataclasses import dataclass
from itertools import product

REPLICAS = ("A", "B")
AMOUNTS = (1, 2)
INIT = 1
SPLIT = (1, 0)  # escrow: A gets 1, B gets 0
PREFIX = 1
OPS = 2


@dataclass(frozen=True)
class Shop:
    """The state at one replica. Replica i writes only entry i."""

    restocked: tuple = (0, 0)
    sold: tuple = (0, 0)

    def stock(self):
        return INIT + sum(self.restocked) - sum(self.sold)

    def rights(self, i):
        return SPLIT[i] + self.restocked[i] - self.sold[i]

    def sell(self, i, k):
        """Sell k at replica i. Return None when i has fewer rights than k."""
        if self.rights(i) < k:
            return None
        return Shop(self.restocked, add(self.sold, i, k))

    def refill(self, i, k):
        """Restock k at replica i. This adds k to the rights of i."""
        return Shop(add(self.restocked, i, k), self.sold)

    def merge(self, other):
        return Shop(
            tuple(map(max, self.restocked, other.restocked)),
            tuple(map(max, self.sold, other.sold)),
        )

    def invariant(self):
        return self.stock() >= 0


def add(entries, i, k):
    return tuple(n + k if j == i else n for j, n in enumerate(entries))


def reach(state, replicas, depth):
    """Yield each state that the replicas reach in at most depth operations."""
    yield state
    if depth == 0:
        return
    for i, op, k in product(replicas, (Shop.sell, Shop.refill), AMOUNTS):
        nxt = op(state, i, k)
        if nxt is not None:
            yield from reach(nxt, replicas, depth - 1)


def schedules():
    """Yield the states (a, b) of A and B before each merge."""
    everyone = range(len(REPLICAS))
    for base in set(reach(Shop(), everyone, PREFIX)):
        for a in set(reach(base, (0,), OPS)):
            for b in set(reach(base, (1,), OPS)):
                yield a, b


def test_split():
    assert sum(SPLIT) == INIT


def test_sell_needs_rights():
    assert Shop().sell(1, 1) is None
    assert Shop().sell(0, 1) is not None


def test_invariant():
    for a, b in schedules():
        assert a.invariant() and b.invariant()
        assert a.merge(b).invariant()


def test_merge_laws():
    states = {s for pair in schedules() for s in pair}
    for x, y in product(states, repeat=2):
        assert x.merge(y) == y.merge(x)
    for x in states:
        assert x.merge(x) == x
    for x, y, z in product(states, repeat=3):
        assert x.merge(y).merge(z) == x.merge(y.merge(z))


if __name__ == "__main__":
    test_split()
    test_sell_needs_rights()
    test_invariant()
    test_merge_laws()
