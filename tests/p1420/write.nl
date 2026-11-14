struct Cell { next: Option<ptr<Cell>>, payload: u8, }
fn reflexive_count(n: usize) -> bool { n == n }
fn main() -> unit {
    match try_allocate_one<Cell>() {
        None => { unit },
        Some(left_bundle) => {
            let OneBacking { allocation, raw } = left_bundle;
            let left_allocation = allocation;
            let left_slot = into_slot<Cell>(raw);
            let left_domain = lifetime_domain();
            let left_ptr = loan_read(left_domain) { |left_initial|
                initialize(left_slot,
                    Cell { next: Option<ptr<Cell>>::None, payload: u8(37) },
                    left_initial)
            };
            match try_allocate_one<Cell>() {
                None => {
                    let left_empty = loan_exclusive_read(left_domain) { |left_end|
                        destroy(left_ptr, left_end)
                    };
                    let left_raw = erase_slot<Cell>(left_empty);
                    finalize_domain(left_domain);
                    deallocate(left_allocation, left_raw);
                    unit
                },
                Some(right_bundle) => {
                    let OneBacking { allocation, raw } = right_bundle;
                    let right_allocation = allocation;
                    let right_slot = into_slot<Cell>(raw);
                    let right_domain = lifetime_domain();
                    let right_ptr = loan_read(right_domain) { |right_initial|
                        initialize(right_slot,
                            Cell { next: Option<ptr<Cell>>::None, payload: u8(181) },
                            right_initial)
                    };
                    let old_left_next = loan_read(left_domain) { |left_write_stable|
                        let left_writer = ref_from_ptr(write, left_ptr, left_write_stable);
                        replace(left_writer@next, Option<ptr<Cell>>::Some(right_ptr))
                    };
                    // For every exact usize n, reflexive_count(n) is true.
                    // This source-level lemma never issues pointer/Domain facts.
                    let decision = reflexive_count(usize(11));
                    let candidate_ptr = if (decision) {
                        right_ptr
                    } else {
                        left_ptr
                    };
                    let observed_right_next = loan_read(right_domain) { |right_stable|
                        let reader = unchecked ref_from_ptr(write, candidate_ptr, right_stable);
                        read(reader@next)
                    };
                    let right_empty = loan_exclusive_read(right_domain) { |right_end|
                        destroy(right_ptr, right_end)
                    };
                    let right_raw = erase_slot<Cell>(right_empty);
                    finalize_domain(right_domain);
                    deallocate(right_allocation, right_raw);
                    let left_empty = loan_exclusive_read(left_domain) { |left_end|
                        destroy(left_ptr, left_end)
                    };
                    let left_raw = erase_slot<Cell>(left_empty);
                    finalize_domain(left_domain);
                    deallocate(left_allocation, left_raw);
                    unit
                },
            }
        },
    }
}
