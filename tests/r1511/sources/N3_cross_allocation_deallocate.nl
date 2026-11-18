struct Parcel {
    successor: Option<ptr<Parcel>>,
    color: u8,
}
struct Signals {
    first: u8,
    second: u8,
}
fn seal(dest: ref<write,u8>, byte: u8) -> u8 {
    let prior = replace(dest, byte);
    return prior;
}
fn main() -> unit {
    let living = Signals { first: u8(14), second: u8(29) };
    let scratch = Signals { first: u8(43), second: u8(61) };
    let initial_first = living@first;
    let sibling_before = living@second;
    let old_initial = loan_write(living@first) { |w| seal(w, u8(37)) };
    let scratch_old = loan_write(scratch@second) { |w| seal(w, old_initial) };
    match try_allocate_one<Parcel>() {
        None => { unit },
        Some(bundle_a) => {
            let OneBacking { allocation, raw } = bundle_a;
            let alloc_a = allocation;
            let slot_a = into_slot<Parcel>(raw);
            let life_a = lifetime_domain();
            let a_ptr = loan_read(life_a) { |stable_a|
                initialize(slot_a, Parcel {
                    successor: Option<ptr<Parcel>>::None,
                    color: u8(8)
                }, stable_a)
            };
            match try_allocate_one<Parcel>() {
                None => {
                    let dead_a = loan_exclusive_read(life_a) { |ending_a|
                        destroy(a_ptr, ending_a)
                    };
                    let whole_a = erase_slot<Parcel>(dead_a);
                    finalize_domain(life_a);
                    deallocate(alloc_a, whole_a);
                    unit
                },
                Some(bundle_b) => {
                    let OneBacking { allocation, raw } = bundle_b;
                    let alloc_b = allocation;
                    let slot_b = into_slot<Parcel>(raw);
                    let life_b = lifetime_domain();
                    let b_ptr = loan_read(life_b) { |stable_b|
                        initialize(slot_b, Parcel {
                            successor: Option<ptr<Parcel>>::None,
                            color: u8(0)
                        }, stable_b)
                    };
                    let old_link = loan_read(life_a) { |stable_a|
                        let owner_write = ref_from_ptr(write, a_ptr, stable_a);
                        replace(owner_write@successor, Option<ptr<Parcel>>::Some(b_ptr))
                    };
                    let scalar = loan_read(life_b) { |stable_b|
                        let observed_b = unchecked ref_from_ptr(read, b_ptr, stable_b);
                        read(observed_b@color)
                    };
                    let removed_link = loan_read(life_a) { |stable_a|
                        let owner_write2 = ref_from_ptr(write, a_ptr, stable_a);
                        replace(owner_write2@successor, Option<ptr<Parcel>>::None)
                    };
                    let dead_b = loan_exclusive_read(life_b) { |ending_b|
                        destroy(b_ptr, ending_b)
                    };
                    let whole_b = erase_slot<Parcel>(dead_b);
                    finalize_domain(life_b);
                    deallocate(alloc_a, whole_b);
                    let dead_a = loan_exclusive_read(life_a) { |ending_a|
                        destroy(a_ptr, ending_a)
                    };
                    let whole_a = erase_slot<Parcel>(dead_a);
                    finalize_domain(life_a);
                    deallocate(alloc_a, whole_a);
                    let old_second = loan_write(living@first) { |w| seal(w, scalar) };
                    let final_first = living@first;
                    let sibling_after = living@second;
                    let scratch_after = scratch@second;
                    unit
                },
            }
        },
    }
}
