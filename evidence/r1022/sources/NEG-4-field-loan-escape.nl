struct Register { lane: u8, seal: u8 }

fn carry_old(target: ref<write,u8>, fresh: u8) -> u8 {
    let displaced = replace(target, fresh);
    displaced
}

struct Link { next: Option<ptr<Link>>, mark: u8 }

fn relay_old(target: ref<write,u8>, fresh: u8) -> u8 {
    return replace(target, fresh);
}

fn detach_live(head_link: ref<write,Option<ptr<Link>>>, p: ptr<Link>, a: Allocation, d: LifetimeDomain) -> LiveTail {
    let removed_link = replace(head_link, Option<ptr<Link>>::None);
    return LiveTail { owned_ptr: p, owned_allocation: a, owned_domain: d };
}

fn retire_live(p: ptr<Link>, a: Allocation, d: LifetimeDomain) -> unit {
    loan_read(d) { |stable|
        let view = ref_from_ptr(read, p, stable);
        unit
    };
    let empty = loan_exclusive_read(d) { |ending|
        destroy(p, ending)
    };
    let full = erase_slot<Link>(empty);
    finalize_domain(d);
    deallocate(a, full);
    unit
}

fn main() -> unit {
    let first = Register { seal: u8(42), lane: u8(13) };
    let second = Register { lane: u8(204), seal: u8(91) };
    let escaped_field_ref = loan_write(first@lane) { |w| w };
    let old_first_seal = loan_write(first@seal) { |w|
        carry_old(w, u8(137))
    };
    let old_second_lane = loan_write(second@lane) { |w|
        relay_old(w, old_first_seal)
    };
    let old_first_lane = loan_write(first@lane) { |w|
        replace(w, old_second_lane)
    };
    let final_first_lane = first@lane;
    let final_first_seal = first@seal;
    let final_second_lane = second@lane;
    let final_second_seal = second@seal;

    match try_allocate_one<Link>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let alloc_h = allocation;
            let slot_h = into_slot<Link>(raw);
            let domain_h = lifetime_domain();
            let head = loan_read(domain_h) { |stable_h|
                initialize(slot_h, Link { next: Option<ptr<Link>>::None, mark: u8(63) }, stable_h)
            };

            match try_allocate_one<Link>() {
                None => {
                    let empty_h = loan_exclusive_read(domain_h) { |end_h|
                        destroy(head, end_h)
                    };
                    let raw_h = erase_slot<Link>(empty_h);
                    finalize_domain(domain_h);
                    deallocate(alloc_h, raw_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let alloc_t = allocation;
                    let slot_t = into_slot<Link>(raw);
                    let domain_t = lifetime_domain();
                    let tail = loan_read(domain_t) { |stable_t|
                        initialize(slot_t, Link { next: Option<ptr<Link>>::None, mark: u8(171) }, stable_t)
                    };
                    let replaced_none = loan_read(domain_h) { |stable_h|
                        let head_writer = ref_from_ptr(write, head, stable_h);
                        replace(head_writer@next, Option<ptr<Link>>::Some(tail))
                    };
                    let observed_link = loan_read(domain_h) { |stable_h|
                        let head_reader = ref_from_ptr(read, head, stable_h);
                        read(head_reader@next)
                    };
                    match observed_link {
                        None => { unit },
                        Some(q) => {
                            loan_read(domain_t) { |stable_t|
                                let tail_reader = ref_from_ptr(read, q, stable_t);
                                unit
                            }
                        },
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(domain_h) { |stable_h|
                        let head_writer = ref_from_ptr(write, head, stable_h);
                        detach_live(head_writer@next, tail, alloc_t, domain_t)
                    };
                    loan_read(owned_domain) { |stable_t|
                        let tail_reader = ref_from_ptr(read, owned_ptr, stable_t);
                        unit
                    };
                    retire_live(owned_ptr, owned_allocation, owned_domain);
                    let empty_h = loan_exclusive_read(domain_h) { |end_h|
                        destroy(head, end_h)
                    };
                    let raw_h = erase_slot<Link>(empty_h);
                    finalize_domain(domain_h);
                    deallocate(alloc_h, raw_h);
                    unit
                },
            }
        },
    }
}
