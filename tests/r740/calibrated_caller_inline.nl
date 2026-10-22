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
                    Capsule { following: Option<ptr<Capsule>>::None, marker: u8(57) },
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
                            Capsule { following: Option<ptr<Capsule>>::None, marker: u8(166) },
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
                    let retired1 = loan_exclusive_read(owned_domain) { |e1|
                        destroy(owned_ptr, e1)
                    };
                    let reclaimed1 = erase_slot<Capsule>(retired1);
                    finalize_domain(owned_domain);
                    deallocate(owned_allocation, reclaimed1);
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
