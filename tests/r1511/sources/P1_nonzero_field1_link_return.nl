struct RegisterPair {
    low: u8,
    high: u8,
}
fn record(slot: ref<write,u8>, incoming: u8) -> u8 {
    let replaced = replace(slot, incoming);
    return replaced;
}
struct EchoCell {
    trail: Option<ptr<EchoCell>>,
    pitch: u8,
}
fn main() -> unit {
    let origin = RegisterPair { high: u8(42), low: u8(9) };
    let witness = RegisterPair { low: u8(77), high: u8(21) };
    let initial_high = origin@high;
    let sibling_low = origin@low;
    let old_first = loan_write(origin@high) { |w| record(w, u8(91)) };
    let witness_old = loan_write(witness@low) { |w| record(w, old_first) };
    match try_allocate_one<EchoCell>() {
        None => { unit },
        Some(bundle_front) => {
            let OneBacking { allocation, raw } = bundle_front;
            let front_allocation = allocation;
            let front_slot = into_slot<EchoCell>(raw);
            let front_domain = lifetime_domain();
            let front_ptr = loan_read(front_domain) { |stable_front|
                initialize(front_slot, EchoCell {
                    trail: Option<ptr<EchoCell>>::None,
                    pitch: u8(6)
                }, stable_front)
            };
            match try_allocate_one<EchoCell>() {
                None => {
                    let ended_front = loan_exclusive_read(front_domain) { |ending_front|
                        destroy(front_ptr, ending_front)
                    };
                    let full_front = erase_slot<EchoCell>(ended_front);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, full_front);
                    unit
                },
                Some(bundle_back) => {
                    let OneBacking { allocation, raw } = bundle_back;
                    let back_allocation = allocation;
                    let back_slot = into_slot<EchoCell>(raw);
                    let back_domain = lifetime_domain();
                    let back_ptr = loan_read(back_domain) { |stable_back|
                        initialize(back_slot, EchoCell {
                            trail: Option<ptr<EchoCell>>::None,
                            pitch: u8(173)
                        }, stable_back)
                    };
                    let old_trail = loan_read(front_domain) { |stable_front|
                        let front_write = ref_from_ptr(write, front_ptr, stable_front);
                        replace(front_write@trail, Option<ptr<EchoCell>>::Some(back_ptr))
                    };
                    let seen = loan_read(front_domain) { |stable_front|
                        let front_read = unchecked ref_from_ptr(read, front_ptr, stable_front);
                        read(front_read@trail)
                    };
                    let actual = match seen {
                        None => {
                            let clear_trail = loan_read(front_domain) { |stable_front|
                                let fw = ref_from_ptr(write, front_ptr, stable_front);
                                replace(fw@trail, Option<ptr<EchoCell>>::None)
                            };
                            let ended_front = loan_exclusive_read(front_domain) { |ending_front|
                                destroy(front_ptr, ending_front)
                            };
                            let full_front = erase_slot<EchoCell>(ended_front);
                            finalize_domain(front_domain);
                            deallocate(front_allocation, full_front);
                            let ended_back = loan_exclusive_read(back_domain) { |ending_back|
                                destroy(back_ptr, ending_back)
                            };
                            let full_back = erase_slot<EchoCell>(ended_back);
                            finalize_domain(back_domain);
                            deallocate(back_allocation, full_back);
                            return unit;
                        },
                        Some(q) => {
                            loan_read(back_domain) { |stable_back|
                                let back_read = unchecked ref_from_ptr(read, q, stable_back);
                                read(back_read@pitch)
                            }
                        },
                    };
                    let clear_trail = loan_read(front_domain) { |stable_front|
                        let fw2 = ref_from_ptr(write, front_ptr, stable_front);
                        replace(fw2@trail, Option<ptr<EchoCell>>::None)
                    };
                    let ended_front = loan_exclusive_read(front_domain) { |ending_front|
                        destroy(front_ptr, ending_front)
                    };
                    let full_front = erase_slot<EchoCell>(ended_front);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, full_front);
                    let ended_back = loan_exclusive_read(back_domain) { |ending_back|
                        destroy(back_ptr, ending_back)
                    };
                    let full_back = erase_slot<EchoCell>(ended_back);
                    finalize_domain(back_domain);
                    deallocate(back_allocation, full_back);
                    let old_second = loan_write(origin@high) { |w| record(w, actual) };
                    let current_high = origin@high;
                    let current_low = origin@low;
                    let witness_low = witness@low;
                    unit
                },
            }
        },
    }
}
