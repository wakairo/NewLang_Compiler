struct Sprout { following: Option<ptr<Sprout>>, stamp: u8, }

fn detach_sprout(
    head_link: ref<write, Option<ptr<Sprout>>>,
    p: ptr<Sprout>,
    a: Allocation,
    d: LifetimeDomain
) -> LiveTail {
    let old_link = replace(
        head_link, Option<ptr<Sprout>>::None);
    let outbound = LiveTail {
        owned_allocation: a,
        owned_ptr: p,
        owned_domain: d
    };
    return outbound;
}

fn retire_sprout(
    tail_ptr: ptr<Sprout>,
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
    let full_t = erase_slot<Sprout>(empty_t);
    finalize_domain(tail_life);
    deallocate(tail_allocation, full_t);
    unit
}

fn main() -> unit {
    match try_allocate_one<Sprout>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<Sprout>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(vacant_h,
                    Sprout { following: Option<ptr<Sprout>>::None, stamp: u8(17) },
                    stable_h)
            };

            match try_allocate_one<Sprout>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Sprout>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<Sprout>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(vacant_t,
                            Sprout { following: Option<ptr<Sprout>>::None,
                                   stamp: u8(149) },
                            stable_t)
                    };

                    let old_none = loan_read(life_h) { |stable_h|
                        let head_w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(head_w@following,
                            Option<ptr<Sprout>>::Some(ptr_t))
                    };
                    let ptr_alias = ptr_t;
                    let LiveTail {
                        owned_ptr,
                        owned_allocation,
                        owned_domain
                    } = loan_read(life_h) { |stable_h|
                        let head_w2 = ref_from_ptr(write, ptr_h, stable_h);
                        detach_sprout(
                            head_w2@following, ptr_alias, allocation_t, life_t)
                    };

                    loan_read(owned_domain) { |stable_t|
                        let live_t = ref_from_ptr(read, owned_ptr, stable_t);
                        unit
                    };

                    retire_sprout(
                        owned_ptr, owned_allocation, owned_domain);

                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Sprout>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
