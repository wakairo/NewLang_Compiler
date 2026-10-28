struct BirchNode { next: Option<ptr<BirchNode>>, payload: u8 }

struct GaugePair { inner: u8, outer: u8 }

fn adjust_measure(w: ref<write,u8>, next_value: u8) -> u8 {
    let prior = replace(w, next_value);
    return prior;
}

fn detach_and_return_tail(
    head_link: ref<write, Option<ptr<BirchNode>>>,
    p: ptr<BirchNode>, a: Allocation, d: LifetimeDomain
) -> LiveTail {
    let removed = replace(head_link, Option<ptr<BirchNode>>::None);
    let packet = LiveTail { owned_ptr: p, owned_allocation: a, owned_domain: d };
    return packet;
}
fn receive_and_release_tail(p: ptr<BirchNode>, a: Allocation, d: LifetimeDomain) -> unit {
    loan_read(d) { |stable|
        let r = ref_from_ptr(read, p, stable);
        unit
    };
    let empty = loan_exclusive_read(d) { |ending| destroy(p, ending) };
    let full = erase_slot<BirchNode>(empty);
    finalize_domain(d);
    deallocate(a, full);
    unit
}

fn main() -> unit {
    let probe = GaugePair { outer: u8(91), inner: u8(14) };
    let ledger = GaugePair { inner: u8(8), outer: u8(207) };
    let previous = if (usize(1) == usize(1)) {
        loan_write(probe@inner) { |w| adjust_measure(w, u8(65)) }
    } else {
        loan_write(probe@inner) { |w| adjust_measure(w, u8(66)) }
    };
    let displaced = loan_write(ledger@outer) { |w| replace(w, previous) };
    let probe_new = probe@inner;
    let probe_sibling = probe@outer;
    let prior_sibling = if (usize(2) == usize(3)) {
        loan_write(ledger@inner) { |w| replace(w, u8(111)) }
    } else {
        loan_write(ledger@inner) { |w| replace(w, u8(109)) }
    };
    let ledger_new = ledger@outer;
    let ledger_sibling = ledger@inner;
    match try_allocate_one<BirchNode>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<BirchNode>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(vacant_h, BirchNode { next: Option<ptr<BirchNode>>::None, payload: u8(43) }, stable_h)
            };
            match try_allocate_one<BirchNode>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<BirchNode>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<BirchNode>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(vacant_t, BirchNode { next: Option<ptr<BirchNode>>::None, payload: u8(179) }, stable_t)
                    };
                    let old_none = loan_read(life_h) { |stable_h|
                        let w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(w@next, Option<ptr<BirchNode>>::Some(ptr_t))
                    };
                    let actual_link = loan_read(life_h) { |stable_h|
                        let r = ref_from_ptr(read, ptr_h, stable_h);
                        read(r@next)
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
                        detach_and_return_tail(w2@next, ptr_t, allocation_t, life_t)
                    };
                    loan_read(owned_domain) { |stable_t|
                        let still_live = ref_from_ptr(read, owned_ptr, stable_t);
                        unit
                    };
                    receive_and_release_tail(owned_ptr, owned_allocation, owned_domain);
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<BirchNode>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    };
    unit
}
