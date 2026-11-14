struct Lantern {
    peer: Option<ptr<Lantern>>,
    mark: u8,
}

fn main() -> unit {
    match try_allocate_one<Lantern>() {
        None => { unit },
        Some(amber_bundle) => {
            let OneBacking { allocation, raw } = amber_bundle;
            let amber_allocation = allocation;
            let amber_slot = into_slot<Lantern>(raw);
            let amber_life = lifetime_domain();
            let amber_ptr = loan_read(amber_life) { |amber_stable|
                initialize(
                    amber_slot,
                    Lantern {
                        peer: Option<ptr<Lantern>>::None,
                        mark: u8(47)
                    },
                    amber_stable
                )
            };
            match try_allocate_one<Lantern>() {
                None => {
                    let amber_empty = loan_exclusive_read(amber_life) { |amber_end|
                        destroy(amber_ptr, amber_end)
                    };
                    let amber_raw = erase_slot<Lantern>(amber_empty);
                    finalize_domain(amber_life);
                    deallocate(amber_allocation, amber_raw);
                    unit
                },
                Some(violet_bundle) => {
                    let OneBacking { allocation, raw } = violet_bundle;
                    let violet_allocation = allocation;
                    let violet_slot = into_slot<Lantern>(raw);
                    let violet_life = lifetime_domain();
                    let violet_ptr = loan_read(violet_life) { |violet_stable|
                        initialize(
                            violet_slot,
                            Lantern {
                                peer: Option<ptr<Lantern>>::None,
                                mark: u8(193)
                            },
                            violet_stable
                        )
                    };
                    let choose_amber = usize(7) < usize(11);
                    let selected_ptr = if (choose_amber) {
                        amber_ptr
                    } else {
                        violet_ptr
                    };
                    let displaced_none = loan_read(amber_life) { |amber_write_stable|
                        let amber_writer = ref_from_ptr(write, amber_ptr, amber_write_stable);
                        replace(amber_writer@peer, Option<ptr<Lantern>>::Some(selected_ptr))
                    };
                    let amber_observed = loan_read(amber_life) { |amber_read_stable|
                        let amber_reader = ref_from_ptr(read, amber_ptr, amber_read_stable);
                        read(amber_reader@peer)
                    };
                    match amber_observed {
                        None => { unit },
                        Some(amber_echo) => { unit },
                    };
                    let violet_observed = loan_read(violet_life) { |violet_read_stable|
                        let violet_reader = ref_from_ptr(read, violet_ptr, violet_read_stable);
                        read(violet_reader@peer)
                    };
                    match violet_observed {
                        None => { unit },
                        Some(violet_echo) => { unit },
                    };
                    let amber_empty = loan_exclusive_read(amber_life) { |amber_end|
                        destroy(amber_ptr, amber_end)
                    };
                    let amber_raw = erase_slot<Lantern>(amber_empty);
                    finalize_domain(amber_life);
                    deallocate(amber_allocation, amber_raw);
                    let violet_empty = loan_exclusive_read(violet_life) { |violet_end|
                        destroy(violet_ptr, violet_end)
                    };
                    let violet_raw = erase_slot<Lantern>(violet_empty);
                    finalize_domain(violet_life);
                    deallocate(violet_allocation, violet_raw);
                    unit
                },
            }
        },
    }
}
