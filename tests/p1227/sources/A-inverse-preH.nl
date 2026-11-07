struct Spool { chain: Option<ptr<Spool>>, tag: u8 }
struct Meter { channel: u8, watermark: u8 }

fn disconnect(field: ref<write,Option<ptr<Spool>>>, address: ptr<Spool>, allocation: Allocation, life: LifetimeDomain) -> LiveTail {
    let old_link = replace(field, Option<ptr<Spool>>::None);
    return LiveTail { owned_ptr: address, owned_allocation: allocation, owned_domain: life };
}

fn retire(address: ptr<Spool>, allocation: Allocation, life: LifetimeDomain) -> unit {
    loan_read(life) { |stable|
        let observed = ref_from_ptr(read, address, stable);
        unit
    };
    let empty = loan_exclusive_read(life) { |ending| destroy(address, ending) };
    let full = erase_slot<Spool>(empty);
    finalize_domain(life);
    deallocate(allocation, full);
    unit
}

fn sample(position: usize) -> u8 {
    let normal = if (position < usize(1)) {
        u8(53)
    } else {
        return u8(149);
    };
    let copied = normal;
    copied
}

fn main() -> unit {
    let original = Meter { channel: u8(31), watermark: u8(203) };
    match try_allocate_one<Spool>() {
        None => { unit },
        Some(first_bundle) => {
            let OneBacking { allocation, raw } = first_bundle;
            let first_allocation = allocation;
            let first_slot = into_slot<Spool>(raw);
            let first_life = lifetime_domain();
            let first_ptr = loan_read(first_life) { |stable_first|
                initialize(first_slot, Spool { chain: Option<ptr<Spool>>::None, tag: u8(41) }, stable_first)
            };
            match try_allocate_one<Spool>() {
                None => {
                    let vacant_first = loan_exclusive_read(first_life) { |ending_first| destroy(first_ptr, ending_first) };
                    let full_first = erase_slot<Spool>(vacant_first);
                    finalize_domain(first_life);
                    deallocate(first_allocation, full_first);
                    unit
                },
                Some(second_bundle) => {
                    let OneBacking { allocation, raw } = second_bundle;
                    let second_allocation = allocation;
                    let second_slot = into_slot<Spool>(raw);
                    let second_life = lifetime_domain();
                    let second_ptr = loan_read(second_life) { |stable_second|
                        initialize(second_slot, Spool { chain: Option<ptr<Spool>>::None, tag: u8(167) }, stable_second)
                    };
                    let initial_none = loan_read(first_life) { |stable_first|
                        let writer = ref_from_ptr(write, first_ptr, stable_first);
                        replace(writer@chain, Option<ptr<Spool>>::Some(second_ptr))
                    };
                    let copied_link = loan_read(first_life) { |stable_first|
                        let reader = ref_from_ptr(read, first_ptr, stable_first);
                        read(reader@chain)
                    };
                    match copied_link {
                        None => { unit },
                        Some(reachable) => {
                            loan_read(second_life) { |stable_second|
                                let checked = ref_from_ptr(read, reachable, stable_second);
                                unit
                            }
                        },
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(first_life) { |stable_first|
                        let writer_again = ref_from_ptr(write, first_ptr, stable_first);
                        disconnect(writer_again@chain, second_ptr, second_allocation, second_life)
                    };
                    loan_read(owned_domain) { |stable_second|
                        let returned = ref_from_ptr(read, owned_ptr, stable_second);
                        unit
                    };
                    retire(owned_ptr, owned_allocation, owned_domain);
                    let vacant_first = loan_exclusive_read(first_life) { |ending_first| destroy(first_ptr, ending_first) };
                    let full_first = erase_slot<Spool>(vacant_first);
                    finalize_domain(first_life);
                    deallocate(first_allocation, full_first);
                    unit
                },
            }
        },
    };
    let initial = usize(0);
    let start = initial;
    let finish = loop (phase = start) {
        if (phase < usize(3)) {
            let argument = phase;
            let incoming = sample(argument);
            let old = loan_write(original@channel) { |writer| replace(writer, incoming) };
            let old_copy = old;
            old_copy;
            let current = original@channel;
            let sibling = original@watermark;
            current;
            sibling;
            continue(phase + usize(1));
        } else {
            break phase;
        }
    };
    let final = original@channel;
    let sibling = original@watermark;
    let iterations = finish;
    unit
}
