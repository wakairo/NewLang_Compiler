struct MailLink { next: Option<ptr<MailLink>>, payload: u8, }

fn park_message(
    head_link: ref<write, Option<ptr<MailLink>>>,
    p: ptr<MailLink>, a: Allocation, d: LifetimeDomain
) -> LiveTail {
    let old_link = replace(head_link, Option<ptr<MailLink>>::None);
    let detached_owner = LiveTail {
        owned_ptr: p,
        owned_allocation: a,
        owned_domain: d
    };
    return detached_owner;
}

fn enqueue_message(
    inbox_slot: ref<write, Option<LiveTail>>,
    message: LiveTail
) -> unit {
    let displaced = replace(
        inbox_slot, Option<LiveTail>::Some(message));
    unit;
    unit
}

fn retire_message(
    tail_ptr: ptr<MailLink>,
    tail_allocation: Allocation,
    tail_life: LifetimeDomain
) -> unit {
    loan_read(tail_life) { |stable_t|
        let tail_r = ref_from_ptr(read, tail_ptr, stable_t);
        unit
    };
    let empty_t = loan_exclusive_read(tail_life) { |ending_t|
        destroy(tail_ptr, ending_t)
    };
    let full_t = erase_slot<MailLink>(empty_t);
    finalize_domain(tail_life);
    deallocate(tail_allocation, full_t);
    unit
}

fn main() -> unit {
    match try_allocate_one<MailLink>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<MailLink>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(vacant_h,
                    MailLink { next: Option<ptr<MailLink>>::None, payload: u8(77) },
                    stable_h)
            };

            match try_allocate_one<MailLink>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<MailLink>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<MailLink>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(vacant_t,
                            MailLink { next: Option<ptr<MailLink>>::None,
                                   payload: u8(152) },
                            stable_t)
                    };
                    let old_none = loan_read(life_h) { |stable_h|
                        let head_w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(head_w@next,
                            Option<ptr<MailLink>>::Some(ptr_t))
                    };
                    let message = loan_read(life_h) { |stable_h|
                        let head_w2 = ref_from_ptr(write, ptr_h, stable_h);
                        park_message(
                            head_w2@next, ptr_t, allocation_t, life_t)
                    };

                    let pending_mail = Option<LiveTail>::None;

                    loan_write(pending_mail) { |inbox_slot|
                        enqueue_message(inbox_slot, message)
                    };

                    let recovered = loan_write(pending_mail) { |inbox_slot|
                        replace(inbox_slot, Option<LiveTail>::None)
                    };
                    match recovered {
                        None => { unit },
                        Some(saved) => {
                            let LiveTail {
                                owned_ptr, owned_allocation, owned_domain
                            } = saved;
                            retire_message(
                                owned_ptr, owned_allocation, owned_domain);
                            unit
                        },
                    };

                    match pending_mail {
                        None => { unit },
                    };

                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<MailLink>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
