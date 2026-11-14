struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main() -> unit {
    match try_allocate_one<Node>() {
        None => { unit },
        Some(front_bundle) => {
            let OneBacking { allocation, raw } = front_bundle;
            let front_allocation = allocation;
            let front_slot = into_slot<Node>(raw);
            let front_life = lifetime_domain();
            let front_ptr = loan_read(front_life) { |front_init_stable|
                initialize(front_slot,
                    Node { next: Option<ptr<Node>>::None, payload: u8(41) },
                    front_init_stable)
            };
            match try_allocate_one<Node>() {
                None => {
                    let front_empty = loan_exclusive_read(front_life) { |front_end|
                        destroy(front_ptr, front_end)
                    };
                    let front_raw = erase_slot<Node>(front_empty);
                    finalize_domain(front_life);
                    deallocate(front_allocation, front_raw);
                    unit
                },
                Some(back_bundle) => {
                    let OneBacking { allocation, raw } = back_bundle;
                    let back_allocation = allocation;
                    let back_slot = into_slot<Node>(raw);
                    let back_life = lifetime_domain();
                    let back_ptr = loan_read(back_life) { |back_init_stable|
                        initialize(back_slot,
                            Node { next: Option<ptr<Node>>::None, payload: u8(99) },
                            back_init_stable)
                    };
                    let old_front_link = loan_read(front_life) { |front_write_stable|
                        let front_write = ref_from_ptr(write, front_ptr, front_write_stable);
                        replace(front_write@next, Option<ptr<Node>>::Some(back_ptr))
                    };
                    let back_roundtrip = loan_read(back_life) { |back_copy_stable|
                        let back_ref = ref_from_ptr(write, back_ptr, back_copy_stable);
                        ptr_from_ref(back_ref)
                    };
                    // Both operands originate in the same live back original.
                    // The condition is not an original-identity proof.
                    let selected_ptr = if (usize(19) == usize(23)) {
                        back_ptr
                    } else {
                        back_roundtrip
                    };
                    let selected_next = loan_read(back_life) { |back_read_stable|
                        let back_reader = ref_from_ptr(read, selected_ptr, back_read_stable);
                        read(back_reader@next)
                    };
                    let back_empty = loan_exclusive_read(back_life) { |back_end|
                        destroy(back_ptr, back_end)
                    };
                    let back_raw = erase_slot<Node>(back_empty);
                    finalize_domain(back_life);
                    deallocate(back_allocation, back_raw);
                    let front_empty = loan_exclusive_read(front_life) { |front_end|
                        destroy(front_ptr, front_end)
                    };
                    let front_raw = erase_slot<Node>(front_empty);
                    finalize_domain(front_life);
                    deallocate(front_allocation, front_raw);
                    unit
                },
            }
        },
    }
}
