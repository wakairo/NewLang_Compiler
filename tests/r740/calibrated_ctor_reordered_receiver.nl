struct Capsule { following: Option<ptr<Capsule>>, marker: u8, }

fn release_edge(
    edge: ref<write, Option<ptr<Capsule>>>,
    target: ptr<Capsule>, entitlement: Allocation, era: LifetimeDomain
) -> LiveTail {
    let cleared = replace(edge, Option<ptr<Capsule>>::None);
    let cargo = LiveTail {
        owned_ptr: target,
        owned_allocation: entitlement,
        owned_domain: era
    };
    return cargo;
}

fn retire_capsule(
    victim: ptr<Capsule>, receipt: Allocation, epoch: LifetimeDomain
) -> unit {
    loan_read(epoch) { |stable|
        let observed = ref_from_ptr(read, victim, stable);
        unit
    };
    let empty = loan_exclusive_read(epoch) { |end|
        destroy(victim, end)
    };
    let full = erase_slot<Capsule>(empty);
    finalize_domain(epoch);
    deallocate(receipt, full);
    unit
}

fn main() -> unit {
    match try_allocate_one<Capsule>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let receipt0 = allocation;
            let vacancy0 = into_slot<Capsule>(raw);
            let epoch0 = lifetime_domain();
            let node0 = loan_read(epoch0) { |s0|
                initialize(vacancy0,
                    Capsule { marker: u8(57), following: Option<ptr<Capsule>>::None },
                    s0)
            };
            match try_allocate_one<Capsule>() {
                None => {
                    let empty0 = loan_exclusive_read(epoch0) { |e0|
                        destroy(node0, e0)
                    };
                    let raw0 = erase_slot<Capsule>(empty0);
                    finalize_domain(epoch0);
                    deallocate(receipt0, raw0);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let receipt1 = allocation;
                    let vacancy1 = into_slot<Capsule>(raw);
                    let epoch1 = lifetime_domain();
                    let node1 = loan_read(epoch1) { |s1|
                        initialize(vacancy1,
                            Capsule { marker: u8(166), following: Option<ptr<Capsule>>::None },
                            s1)
                    };
                    let prior = loan_read(epoch0) { |s0|
                        let wr = ref_from_ptr(write, node0, s0);
                        replace(wr@following, Option<ptr<Capsule>>::Some(node1))
                    };
                    let LiveTail {
                        owned_ptr, owned_allocation, owned_domain
                    } = loan_read(epoch0) { |s0|
                        let wr2 = ref_from_ptr(write, node0, s0);
                        release_edge(wr2@following, node1, receipt1, epoch1)
                    };
                    loan_read(owned_domain) { |still|
                        let witness = ref_from_ptr(read, owned_ptr, still);
                        unit
                    };
                    retire_capsule(owned_ptr, owned_allocation, owned_domain);
                    let empty0 = loan_exclusive_read(epoch0) { |e0|
                        destroy(node0, e0)
                    };
                    let raw0 = erase_slot<Capsule>(empty0);
                    finalize_domain(epoch0);
                    deallocate(receipt0, raw0);
                    unit
                },
            }
        },
    }
}
