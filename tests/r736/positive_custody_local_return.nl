struct Link { payload: u8, next: Option<ptr<Link>>, }

fn extract_to_local(
    forward: ref<write, Option<ptr<Link>>>,
    element: ptr<Link>, permit: Allocation, tenure: LifetimeDomain
) -> LiveTail {
    let before = replace(forward, Option<ptr<Link>>::None);
    let parcel = LiveTail {
        owned_domain: tenure,
        owned_ptr: element,
        owned_allocation: permit
    };
    return parcel;
}

fn store_transfer(
    cell: ref<write, Option<LiveTail>>, incoming: LiveTail
) -> unit {
    let removed = replace(cell, Option<LiveTail>::Some(incoming));
    match removed { None => { unit }, };
    unit
}

fn finish_tail(
    cursor: ptr<Link>, ticket: Allocation, scope: LifetimeDomain
) -> unit {
    loan_read(scope) { |active|
        let r = ref_from_ptr(read, cursor, active);
        unit
    };
    let vacated = loan_exclusive_read(scope) { |end|
        destroy(cursor, end)
    };
    let bytes = erase_slot<Link>(vacated);
    finalize_domain(scope);
    deallocate(ticket, bytes);
    unit
}

fn main() -> unit {
    match try_allocate_one<Link>() {
        None => { unit },
        Some(outer) => {
            let OneBacking { allocation, raw } = outer;
            let head_ticket = allocation;
            let head_slot = into_slot<Link>(raw);
            let head_scope = lifetime_domain();
            let first_ptr = loan_read(head_scope) { |hd|
                initialize(head_slot,
                    Link { payload: u8(19), next: Option<ptr<Link>>::None },
                    hd)
            };
            match try_allocate_one<Link>() {
                None => {
                    let empty = loan_exclusive_read(head_scope) { |h_end|
                        destroy(first_ptr, h_end)
                    };
                    let bytes = erase_slot<Link>(empty);
                    finalize_domain(head_scope);
                    deallocate(head_ticket, bytes);
                    unit
                },
                Some(inner) => {
                    let OneBacking { allocation, raw } = inner;
                    let tail_ticket = allocation;
                    let tail_slot = into_slot<Link>(raw);
                    let tail_scope = lifetime_domain();
                    let last_ptr = loan_read(tail_scope) { |td|
                        initialize(tail_slot,
                            Link { payload: u8(211), next: Option<ptr<Link>>::None },
                            td)
                    };
                    let displaced = loan_read(head_scope) { |hd|
                        let hw = ref_from_ptr(write, first_ptr, hd);
                        replace(hw@next, Option<ptr<Link>>::Some(last_ptr))
                    };
                    let parcel = loan_read(head_scope) { |hd|
                        let hw2 = ref_from_ptr(write, first_ptr, hd);
                        extract_to_local(
                            hw2@next, last_ptr, tail_ticket, tail_scope)
                    };
                    let receptacle = Option<LiveTail>::None;
                    loan_write(receptacle) { |dest|
                        store_transfer(dest, parcel)
                    };
                    let recovered = loan_write(receptacle) { |dest|
                        replace(dest, Option<LiveTail>::None)
                    };
                    match recovered {
                        None => { unit },
                        Some(delivered) => {
                            let LiveTail {
                                owned_ptr, owned_allocation, owned_domain
                            } = delivered;
                            finish_tail(
                                owned_ptr, owned_allocation, owned_domain);
                            unit
                        },
                    };
                    match receptacle { None => { unit }, };
                    let head_empty = loan_exclusive_read(head_scope) { |hend|
                        destroy(first_ptr, hend)
                    };
                    let head_bytes = erase_slot<Link>(head_empty);
                    finalize_domain(head_scope);
                    deallocate(head_ticket, head_bytes);
                    unit
                },
            }
        },
    }
}
