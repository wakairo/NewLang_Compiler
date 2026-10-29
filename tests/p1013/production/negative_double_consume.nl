struct Gauge { amber: u8, jade: u8 }
struct LinkCell { next: Option<ptr<LinkCell>>, mark: u8 }
fn exchange_alpha(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    return displaced;
}
fn exchange_beta(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    return displaced;
}
fn detach_live(edge: ref<write,Option<ptr<LinkCell>>>,
               address: ptr<LinkCell>, allocation: Allocation,
               domain: LifetimeDomain) -> LiveTail {
    let previous_link = replace(edge, Option<ptr<LinkCell>>::None);
    let result = LiveTail{owned_ptr: address, owned_allocation: allocation,
                          owned_domain: domain};
    return result;
}
fn finish_live(address: ptr<LinkCell>, allocation: Allocation,
               domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let inspected = ref_from_ptr(read, address, stable);
        unit
    };
    let vacant = loan_exclusive_read(domain) { |ending|
        destroy(address, ending)
    };
    let storage = erase_slot<LinkCell>(vacant);
    finalize_domain(domain);
    deallocate(allocation, storage);
    unit
}
fn main() -> unit {
    let gauge = Gauge{amber: u8(41), jade: u8(173)};
    let first_old = loan_write(gauge@amber) { |slot|
        exchange_alpha(slot, u8(67))
    };
    let second_old = loan_write(gauge@amber) { |slot|
        exchange_beta(slot, u8(109))
    };
    match try_allocate_one<LinkCell>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let head_allocation = allocation;
            let head_slot = into_slot<LinkCell>(raw);
            let head_domain = lifetime_domain();
            let head = loan_read(head_domain) { |stable|
                initialize(head_slot,
                    LinkCell{next: Option<ptr<LinkCell>>::None, mark: u8(23)},
                    stable)
            };
            match try_allocate_one<LinkCell>() {
                None => {
                    let vacant = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let storage = erase_slot<LinkCell>(vacant);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, storage);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<LinkCell>(raw);
                    let tail_domain = lifetime_domain();
                    let tail = loan_read(tail_domain) { |stable|
                        initialize(tail_slot,
                            LinkCell{next: Option<ptr<LinkCell>>::None,
                                     mark: u8(199)}, stable)
                    };
                    let old_empty = loan_read(head_domain) { |stable|
                        let write_head = ref_from_ptr(write, head, stable);
                        replace(write_head@next, Option<ptr<LinkCell>>::Some(tail))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(head_domain) { |stable|
                            let write_head = ref_from_ptr(write, head, stable);
                            detach_live(write_head@next, tail,
                                        tail_allocation, tail_domain)
                        };
                    loan_read(owned_domain) { |stable|
                        let inspected = ref_from_ptr(read, owned_ptr, stable);
                        unit
                    };
                    finish_live(owned_ptr, owned_allocation, owned_domain);
                    finish_live(owned_ptr, owned_allocation, owned_domain);
                    let vacant = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let storage = erase_slot<LinkCell>(vacant);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, storage);
                    unit
                },
            }
        },
    }
}
