struct BeaconHeap {
    passage: Option<ptr<BeaconHeap>>,
    mark: u8,
}
struct DialPair {
    east: u8,
    west: u8,
}
fn retune(cell: ref<write,u8>, next: u8) -> u8 {
    let before = replace(cell, next);
    return before;
}
fn unlink_return(
    edge: ref<write,Option<ptr<BeaconHeap>>>,
    live: ptr<BeaconHeap>,
    claim: Allocation,
    lifetime: LifetimeDomain
) -> LiveTail {
    let prior_edge = replace(edge, Option<ptr<BeaconHeap>>::None);
    let handoff = LiveTail {
        owned_domain: lifetime,
        owned_ptr: live,
        owned_allocation: claim
    };
    return handoff;
}
fn retire_tail(
    live: ptr<BeaconHeap>,
    claim: Allocation,
    lifetime: LifetimeDomain
) -> unit {
    loan_read(lifetime) { |stable|
        let observer = ref_from_ptr(read, live, stable);
        unit
    };
    let vacant = loan_exclusive_read(lifetime) { |ending|
        destroy(live, ending)
    };
    let raw = erase_slot<BeaconHeap>(vacant);
    finalize_domain(lifetime);
    deallocate(claim, raw);
    unit
}
fn main() -> unit {
    match try_allocate_one<BeaconHeap>() {
        None => { unit },
        Some(first_bundle) => {
            let OneBacking { allocation, raw } = first_bundle;
            let first_claim = allocation;
            let first_slot = into_slot<BeaconHeap>(raw);
            let first_life = lifetime_domain();
            let first_ptr = loan_read(first_life) { |stable|
                initialize(first_slot,
                    BeaconHeap {
                        mark: u8(33),
                        passage: Option<ptr<BeaconHeap>>::None
                    }, stable)
            };
            match try_allocate_one<BeaconHeap>() {
                None => {
                    let vacant = loan_exclusive_read(first_life) { |ending|
                        destroy(first_ptr, ending)
                    };
                    let raw_first = erase_slot<BeaconHeap>(vacant);
                    finalize_domain(first_life);
                    deallocate(first_claim, raw_first);
                    unit
                },
                Some(second_bundle) => {
                    let OneBacking { allocation, raw } = second_bundle;
                    let second_claim = allocation;
                    let second_slot = into_slot<BeaconHeap>(raw);
                    let second_life = lifetime_domain();
                    let second_ptr = loan_read(second_life) { |stable|
                        initialize(second_slot,
                            BeaconHeap {
                                passage: Option<ptr<BeaconHeap>>::None,
                                mark: u8(146)
                            }, stable)
                    };
                    let original_none = loan_read(first_life) { |stable|
                        let first_write = ref_from_ptr(write, first_ptr, stable);
                        replace(first_write@passage,
                            Option<ptr<BeaconHeap>>::Some(second_ptr))
                    };
                    let visible_edge = loan_read(first_life) { |stable|
                        let first_read = ref_from_ptr(read, first_ptr, stable);
                        read(first_read@passage)
                    };
                    match visible_edge {
                        None => { unit },
                        Some(next_ptr) => {
                            loan_read(second_life) { |stable|
                                let next_read = ref_from_ptr(read, next_ptr, stable);
                                unit
                            }
                        },
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(first_life) { |stable|
                            let first_write = ref_from_ptr(write, first_ptr, stable);
                            unlink_return(first_write@passage,
                                second_ptr, second_claim, second_life)
                        };
                    retire_tail(owned_ptr, owned_allocation, owned_domain);
                    retire_tail(owned_ptr, owned_allocation, owned_domain);
                    let first_vacant = loan_exclusive_read(first_life) { |ending|
                        destroy(first_ptr, ending)
                    };
                    let first_raw = erase_slot<BeaconHeap>(first_vacant);
                    finalize_domain(first_life);
                    deallocate(first_claim, first_raw);

                    let sunrise = DialPair { west: u8(17), east: u8(109) };
                    let twilight = DialPair { east: u8(61), west: u8(204) };
                    let from_west = loan_write(sunrise@west) { |gate|
                        retune(gate, u8(92))
                    };
                    let displaced = loan_write(twilight@east) { |gate|
                        replace(gate, from_west)
                    };
                    let after_first = sunrise@west;
                    let unchanged_first = sunrise@east;
                    let after_second = twilight@east;
                    let unchanged_second = twilight@west;
                    unit
                },
            }
        },
    }
}
