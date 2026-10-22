struct Cell { next: Option<ptr<Cell>>, payload: u8, }

fn liberate(
    edge: ref<write, Option<ptr<Cell>>>,
    target: ptr<Cell>, entitlement: Allocation, era: LifetimeDomain
) -> LiveTail {
    let cleared = replace(edge, Option<ptr<Cell>>::Some(target));
    let handoff = LiveTail {
        owned_ptr: target,
        owned_allocation: entitlement,
        owned_domain: era
    };
    return handoff;
}

fn retire(
    victim: ptr<Cell>, receipt: Allocation, epoch: LifetimeDomain
) -> unit {
    loan_read(epoch) { |stable|
        let observed = ref_from_ptr(read, victim, stable);
        unit
    };
    let empty = loan_exclusive_read(epoch) { |end|
        destroy(victim, end)
    };
    let full = erase_slot<Cell>(empty);
    finalize_domain(epoch);
    deallocate(receipt, full);
    unit
}

fn main() -> unit {
    match try_allocate_one<Cell>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let receipt0 = allocation;
            let vacancy0 = into_slot<Cell>(raw);
            let epoch0 = lifetime_domain();
            let node0 = loan_read(epoch0) { |s0|
                initialize(vacancy0,
                    Cell { next: Option<ptr<Cell>>::None, payload: u8(41) },
                    s0)
            };
            match try_allocate_one<Cell>() {
                None => {
                    let empty0 = loan_exclusive_read(epoch0) { |e0|
                        destroy(node0, e0)
                    };
                    let raw0 = erase_slot<Cell>(empty0);
                    finalize_domain(epoch0);
                    deallocate(receipt0, raw0);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let receipt1 = allocation;
                    let vacancy1 = into_slot<Cell>(raw);
                    let epoch1 = lifetime_domain();
                    let node1 = loan_read(epoch1) { |s1|
                        initialize(vacancy1,
                            Cell { next: Option<ptr<Cell>>::None, payload: u8(73) },
                            s1)
                    };
                    let prior = loan_read(epoch0) { |s0|
                        let wr = ref_from_ptr(write, node0, s0);
                        replace(wr@next, Option<ptr<Cell>>::Some(node1))
                    };
                    let LiveTail {
                        owned_ptr, owned_allocation, owned_domain
                    } = loan_read(epoch0) { |s0|
                        let wr2 = ref_from_ptr(write, node0, s0);
                        liberate(wr2@next, node1, receipt1, epoch1)
                    };
                    loan_read(owned_domain) { |still|
                        let witness = ref_from_ptr(read, owned_ptr, still);
                        unit
                    };
                    retire(owned_ptr, owned_allocation, owned_domain);
                    let empty0 = loan_exclusive_read(epoch0) { |e0|
                        destroy(node0, e0)
                    };
                    let raw0 = erase_slot<Cell>(empty0);
                    finalize_domain(epoch0);
                    deallocate(receipt0, raw0);
                    unit
                },
            }
        },
    }
}
