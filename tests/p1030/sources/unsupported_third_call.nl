struct SignalPair { front: u8, back: u8 }
struct LedgerLink { next: Option<ptr<LedgerLink>>, marker: u8 }
fn a_exchange_back(target: ref<write,u8>, incoming: u8) -> u8 {
    let former = replace(target, incoming);
    return former;
}
fn b_exchange_front(target: ref<write,u8>, incoming: u8) -> u8 {
    return replace(target, incoming);
}
fn unhook(edge: ref<write,Option<ptr<LedgerLink>>>,
          address: ptr<LedgerLink>, allocation: Allocation,
          domain: LifetimeDomain) -> LiveTail {
    let displaced_link = replace(edge, Option<ptr<LedgerLink>>::None);
    let carried = LiveTail{owned_ptr: address, owned_allocation: allocation,
                           owned_domain: domain};
    return carried;
}
fn retire_link(address: ptr<LedgerLink>, allocation: Allocation,
               domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let inspected = ref_from_ptr(read, address, stable);
        unit
    };
    let vacancy = loan_exclusive_read(domain) { |ending|
        destroy(address, ending)
    };
    let storage = erase_slot<LedgerLink>(vacancy);
    finalize_domain(domain);
    deallocate(allocation, storage);
    unit
}
fn main() -> unit {
    let pair = SignalPair{front: u8(58), back: u8(203)};
    let prior_back = loan_write(pair@back) { |slot|
        a_exchange_back(slot, u8(97))
    };
    let prior_front = loan_write(pair@front) { |slot|
        b_exchange_front(slot, prior_back)
    };
    let another_back = loan_write(pair@back) { |slot|
        a_exchange_back(slot, prior_front)
    };
    let front_after = pair@front;
    let back_after = pair@back;
    match try_allocate_one<LedgerLink>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let head_allocation = allocation;
            let head_slot = into_slot<LedgerLink>(raw);
            let head_domain = lifetime_domain();
            let head = loan_read(head_domain) { |stable|
                initialize(head_slot,
                    LedgerLink{next: Option<ptr<LedgerLink>>::None,
                               marker: u8(29)}, stable)
            };
            match try_allocate_one<LedgerLink>() {
                None => {
                    let vacancy = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let storage = erase_slot<LedgerLink>(vacancy);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, storage);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<LedgerLink>(raw);
                    let tail_domain = lifetime_domain();
                    let tail = loan_read(tail_domain) { |stable|
                        initialize(tail_slot,
                            LedgerLink{next: Option<ptr<LedgerLink>>::None,
                                       marker: u8(181)}, stable)
                    };
                    let old_empty = loan_read(head_domain) { |stable|
                        let writable = ref_from_ptr(write, head, stable);
                        replace(writable@next,
                                Option<ptr<LedgerLink>>::Some(tail))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(head_domain) { |stable|
                            let writable = ref_from_ptr(write, head, stable);
                            unhook(writable@next, tail,
                                   tail_allocation, tail_domain)
                        };
                    loan_read(owned_domain) { |stable|
                        let inspected = ref_from_ptr(read, owned_ptr, stable);
                        unit
                    };
                    retire_link(owned_ptr, owned_allocation, owned_domain);
                    let vacancy = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let storage = erase_slot<LedgerLink>(vacancy);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, storage);
                    unit
                },
            }
        },
    }
}
