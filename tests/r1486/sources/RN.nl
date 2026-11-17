// R1483-N: complete independent SOURCE V1; no host-created roots.
struct Gauge { reading: u8, sentinel: u8 }
struct Seed { next: Option<ptr<Seed>>, stamp: u8 }

fn install(target: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(target, incoming);
    displaced
}

fn main() -> unit {
    let panel = Gauge { sentinel: u8(203), reading: u8(29) };
    let old_boot = loan_write(panel@reading) { |dst|
        install(dst, u8(91))
    };
    match try_allocate_one<Seed>() {
        None => {
            unit
        },
        Some(first_bundle) => {
            let OneBacking { allocation, raw } = first_bundle;
            let owner_first = allocation;
            let vacant_first = into_slot<Seed>(raw);
            let domain_first = lifetime_domain();
            let ptr_first = loan_read(domain_first) { |stable_first|
                initialize(vacant_first,
                    Seed { next: Option<ptr<Seed>>::None, stamp: u8(47) },
                    stable_first)
            };

            match try_allocate_one<Seed>() {
                None => {
                    let empty_first = loan_exclusive_read(domain_first) { |end_first|
                        destroy(ptr_first, end_first)
                    };
                    let full_first = erase_slot<Seed>(empty_first);
                    finalize_domain(domain_first);
                    deallocate(owner_first, full_first);
                    unit
                },
                Some(second_bundle) => {
                    let OneBacking { allocation, raw } = second_bundle;
                    let owner_second = allocation;
                    let vacant_second = into_slot<Seed>(raw);
                    let domain_second = lifetime_domain();
                    let ptr_second = loan_read(domain_second) { |stable_second|
                        initialize(vacant_second,
                            Seed { next: Option<ptr<Seed>>::None, stamp: u8(176) },
                            stable_second)
                    };
                    let actual_stamp = loan_read(domain_second) { |stable_for_read|
                        let live_second = ref_from_ptr(read, ptr_second, stable_for_read);
                        read(live_second@stamp)
                    };

                    let empty_second = loan_exclusive_read(domain_second) { |end_second|
                        destroy(ptr_second, end_second)
                    };
                    let full_second = erase_slot<Seed>(empty_second);
                    finalize_domain(domain_second);
                    deallocate(owner_second, full_second);

                    let empty_first = loan_exclusive_read(domain_first) { |end_first|
                        destroy(ptr_first, end_first)
                    };
                    let full_first = erase_slot<Seed>(empty_first);
                    finalize_domain(domain_first);
                    deallocate(owner_first, full_first);

                    let old_second = loan_write(panel@reading) { |dst|
                        install(dst, actual_stamp)
                    };
                    unit
                },
            };
            unit
        },
    };
    let joined_reading = panel@reading;
    let joined_sentinel = panel@sentinel;
    unit
}
