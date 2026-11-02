//! Inventory with escrow (D5): each replica sells only its own rights.
//!
//! The Rust form of examples/inventory-escrow.merge. Two replicas share a
//! stock counter. The initial stock is split into rights per replica. A
//! replica sells only from its own rights, so the stock is never below
//! zero after a merge. The tests check the merge laws and the invariant on
//! the bounded domain of `langc check`: a synced prefix of 1 operation,
//! then 2 operations per replica, then the merge.

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Replica {
    A,
    B,
}

pub const REPLICAS: [Replica; 2] = [Replica::A, Replica::B];
pub const AMOUNTS: [u32; 2] = [1, 2];
pub const INIT: u32 = 1;
pub const PREFIX: usize = 1;
pub const OPS: usize = 2;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Op {
    Sell(u32),
    Refill(u32),
}

/// One entry per replica. Replica r writes only its own entry.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub struct PerReplica {
    a: u32,
    b: u32,
}

impl PerReplica {
    fn get(self, r: Replica) -> u32 {
        match r {
            Replica::A => self.a,
            Replica::B => self.b,
        }
    }

    fn add(self, r: Replica, k: u32) -> Self {
        match r {
            Replica::A => Self {
                a: self.a + k,
                ..self
            },
            Replica::B => Self {
                b: self.b + k,
                ..self
            },
        }
    }

    fn max(self, other: Self) -> Self {
        Self {
            a: self.a.max(other.a),
            b: self.b.max(other.b),
        }
    }

    fn sum(self) -> u32 {
        self.a + self.b
    }
}

/// The escrow allocation: A gets 1, B gets 0.
const SPLIT: PerReplica = PerReplica { a: 1, b: 0 };

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub struct Shop {
    restocked: PerReplica,
    sold: PerReplica,
}

impl Shop {
    pub fn stock(&self) -> i64 {
        i64::from(INIT) + i64::from(self.restocked.sum()) - i64::from(self.sold.sum())
    }

    pub fn rights(&self, r: Replica) -> i64 {
        i64::from(SPLIT.get(r)) + i64::from(self.restocked.get(r)) - i64::from(self.sold.get(r))
    }

    /// Sell k at replica r. None when r has fewer rights than k.
    pub fn sell(&self, r: Replica, k: u32) -> Option<Shop> {
        (self.rights(r) >= i64::from(k)).then(|| Shop {
            sold: self.sold.add(r, k),
            ..*self
        })
    }

    /// Restock k at replica r. This adds k to the rights of r.
    pub fn refill(&self, r: Replica, k: u32) -> Shop {
        Shop {
            restocked: self.restocked.add(r, k),
            ..*self
        }
    }

    pub fn apply(&self, r: Replica, op: Op) -> Option<Shop> {
        match op {
            Op::Sell(k) => self.sell(r, k),
            Op::Refill(k) => Some(self.refill(r, k)),
        }
    }

    pub fn merge(&self, other: &Shop) -> Shop {
        Shop {
            restocked: self.restocked.max(other.restocked),
            sold: self.sold.max(other.sold),
        }
    }

    pub fn invariant(&self) -> bool {
        self.stock() >= 0
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::HashSet;

    fn ops() -> impl Iterator<Item = Op> {
        AMOUNTS
            .into_iter()
            .flat_map(|k| [Op::Sell(k), Op::Refill(k)])
    }

    /// Each state that the replicas reach in at most depth operations.
    fn reach(state: Shop, replicas: &[Replica], depth: usize) -> HashSet<Shop> {
        let later = depth.checked_sub(1).into_iter().flat_map(move |d| {
            replicas
                .iter()
                .flat_map(move |&r| ops().filter_map(move |op| state.apply(r, op)))
                .flat_map(move |s| reach(s, replicas, d))
        });
        std::iter::once(state).chain(later).collect()
    }

    /// The states (a, b) of A and B before each merge.
    fn schedules() -> Vec<(Shop, Shop)> {
        reach(Shop::default(), &REPLICAS, PREFIX)
            .into_iter()
            .flat_map(|base| {
                let bs = reach(base, &[Replica::B], OPS);
                reach(base, &[Replica::A], OPS)
                    .into_iter()
                    .flat_map(|a| bs.iter().map(move |&b| (a, b)))
                    .collect::<Vec<_>>()
            })
            .collect()
    }

    #[test]
    fn split_sums_to_init() {
        assert_eq!(SPLIT.sum(), INIT);
    }

    #[test]
    fn sell_needs_rights() {
        assert_eq!(Shop::default().sell(Replica::B, 1), None);
        assert!(Shop::default().sell(Replica::A, 1).is_some());
    }

    #[test]
    fn invariant_holds_after_merge() {
        assert!(schedules()
            .iter()
            .all(|(a, b)| a.invariant() && b.invariant() && a.merge(b).invariant()));
    }

    #[test]
    fn merge_laws() {
        let states: HashSet<Shop> = schedules().into_iter().flat_map(|(a, b)| [a, b]).collect();
        assert!(states.iter().all(|x| x.merge(x) == *x));
        assert!(states
            .iter()
            .all(|x| states.iter().all(|y| x.merge(y) == y.merge(x))));
        assert!(states.iter().all(|x| {
            states.iter().all(|y| {
                states
                    .iter()
                    .all(|z| x.merge(y).merge(z) == x.merge(&y.merge(z)))
            })
        }));
    }
}
