struct BearingPair { north: u8, south: u8 }

fn retune_bearing(w: ref<write,u8>, next_value: u8) -> u8 {
    let prior = replace(w, next_value);
    return prior;
}

struct HarborNode { chain: Option<ptr<HarborNode>>, payload: u8 }

fn detach_and_return_tail(
    head_link: ref<write, Option<ptr<HarborNode>>>,
    p: ptr<HarborNode>, a: Allocation, d: LifetimeDomain
) -> LiveTail {
    let removed = replace(head_link, Option<ptr<HarborNode>>::None);
    let packet = LiveTail { owned_ptr: p, owned_allocation: a, owned_domain: d };
    return packet;
}
fn receive_and_release_tail(p: ptr<HarborNode>, a: Allocation, d: LifetimeDomain) -> unit {
    loan_read(d) { |stable|
        let r = ref_from_ptr(read, p, stable);
        unit
    };
    let empty = loan_exclusive_read(d) { |ending| destroy(p, ending) };
    let full = erase_slot<HarborNode>(empty);
    finalize_domain(d);
    deallocate(a, full);
    unit
}

fn main() -> unit {
    match try_allocate_one<HarborNode>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<HarborNode>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(vacant_h, HarborNode { chain: Option<ptr<HarborNode>>::None, payload: u8(86) }, stable_h)
            };
            match try_allocate_one<HarborNode>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<HarborNode>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<HarborNode>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(vacant_t, HarborNode { chain: Option<ptr<HarborNode>>::None, payload: u8(152) }, stable_t)
                    };
                    let old_none = loan_read(life_h) { |stable_h|
                        let w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(w@chain, Option<ptr<HarborNode>>::Some(ptr_t))
                    };
                    let actual_link = loan_read(life_h) { |stable_h|
                        let r = ref_from_ptr(read, ptr_h, stable_h);
                        read(r@chain)
                    };
                    match actual_link {
                        None => { unit },
                        Some(q) => {
                            loan_read(life_t) { |stable_t|
                                let r_t = ref_from_ptr(read, q, stable_t);
                                unit
                            }
                        },
                    };
                    let LiveTail {
                        owned_ptr,
                        owned_allocation,
                        owned_domain
                    } = loan_read(life_h) { |stable_h|
                        let w2 = ref_from_ptr(write, ptr_h, stable_h);
                        detach_and_return_tail(w2@chain, ptr_h, allocation_t, life_t)
                    };
                    loan_read(owned_domain) { |stable_t|
                        let still_live = ref_from_ptr(read, owned_ptr, stable_t);
                        unit
                    };
                    receive_and_release_tail(owned_ptr, owned_allocation, owned_domain);
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<HarborNode>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    };
    let sensor = BearingPair { south: u8(182), north: u8(49) };
    let replica = BearingPair { north: u8(230), south: u8(7) };
    let previous = loan_write(sensor@south) { |w| retune_bearing(w, u8(12)) };
    let displaced = loan_write(replica@north) { |w| replace(w, previous) };
    let sensor_new = sensor@south;
    let sensor_sibling = sensor@north;
    let replica_new = replica@north;
    let replica_sibling = replica@south;
    unit
}
