// P1468 B2 v1: stay on the genuine H2 Some edge, close H, then install.
// All failure arms remain normal; no owner or numeric success flag escapes.
struct Display { reserve: u8, level: u8 }
struct SampleEvent { next: Option<ptr<SampleEvent>>, sample: u8 }

fn newest_only(has_next: bool, sample: u8) -> u8 {
    if (has_next) { u8(0) } else { sample }
}

fn install_level(out: ref<write, u8>, sample: u8) -> u8 {
    let previous = replace(out, sample);
    previous
}

fn unlink_oldest(
    next: ref<write, Option<ptr<SampleEvent>>>,
    tail: ptr<SampleEvent>, memory: Allocation, life: LifetimeDomain
) -> LiveTail {
    let removed = replace(next, Option<ptr<SampleEvent>>::None);
    let retained = LiveTail {
        owned_ptr: tail, owned_allocation: memory, owned_domain: life
    };
    return retained;
}

fn retire_event(
    event: ptr<SampleEvent>, memory: Allocation, life: LifetimeDomain
) -> unit {
    let vacant = loan_exclusive_read(life) { |ending|
        destroy(event, ending)
    };
    let recovered = erase_slot<SampleEvent>(vacant);
    finalize_domain(life);
    deallocate(memory, recovered);
    unit
}

fn main() -> unit {
    let display = Display { level: u8(87), reserve: u8(13) };
    let previous_at_startup = loan_write(display@level) { |out|
        install_level(out, u8(0))
    };

    match try_allocate_one<SampleEvent>() {
        None => { unit },
        Some(first_backing) => {
            let OneBacking { allocation, raw } = first_backing;
            let first_memory = allocation;
            let first_slot = into_slot<SampleEvent>(raw);
            let first_life = lifetime_domain();
            let first_ptr = loan_read(first_life) { |stable|
                initialize(first_slot,
                    SampleEvent {
                        next: Option<ptr<SampleEvent>>::None,
                        sample: u8(76)
                    }, stable)
            };

            match try_allocate_one<SampleEvent>() {
                None => {
                    let vacant_first = loan_exclusive_read(first_life) { |ending|
                        destroy(first_ptr, ending)
                    };
                    let recovered_first = erase_slot<SampleEvent>(vacant_first);
                    finalize_domain(first_life);
                    deallocate(first_memory, recovered_first);
                    unit
                },
                Some(last_backing) => {
                    let OneBacking { allocation, raw } = last_backing;
                    let last_memory = allocation;
                    let last_slot = into_slot<SampleEvent>(raw);
                    let last_life = lifetime_domain();
                    let last_ptr = loan_read(last_life) { |stable|
                        initialize(last_slot,
                            SampleEvent {
                                next: Option<ptr<SampleEvent>>::None,
                                sample: u8(181)
                            }, stable)
                    };

                    let previous_link = loan_read(first_life) { |stable|
                        let write_first = ref_from_ptr(write, first_ptr, stable);
                        replace(write_first@next,
                            Option<ptr<SampleEvent>>::Some(last_ptr))
                    };
                    let sample = loan_read(last_life) { |sample_scope|
                        let read_last = ref_from_ptr(read, last_ptr, sample_scope);
                        read(read_last@sample)
                    };
                    let following = loan_read(last_life) { |link_scope|
                        let read_last = ref_from_ptr(read, last_ptr, link_scope);
                        read(read_last@next)
                    };
                    let has_next = match following {
                        None => { usize(0) < usize(0) },
                        Some(later) => { usize(0) == usize(0) },
                    };
                    let selected = newest_only(has_next, sample);

                    let packet = loan_read(first_life) { |stable|
                        let write_first = ref_from_ptr(write, first_ptr, stable);
                        unlink_oldest(write_first@next, last_ptr,
                                      last_memory, last_life)
                    };
                    let LiveTail {
                        owned_ptr, owned_allocation, owned_domain
                    } = packet;
                    let vacant_first = loan_exclusive_read(first_life) { |ending|
                        destroy(first_ptr, ending)
                    };
                    let recovered_first = erase_slot<SampleEvent>(vacant_first);
                    finalize_domain(first_life);
                    deallocate(first_memory, recovered_first);
                    retire_event(owned_ptr, owned_allocation, owned_domain);
                    let previous_displayed = loan_write(display@level) { |out|
                        install_level(out, selected)
                    };
                    let retained_previous = previous_displayed;
                    unit
                },
            }
        },
    };

    let retained_startup = previous_at_startup;
    let delivered_level = display@level;
    let routed_reserve = display@reserve;
    unit
}
