struct Gauge { tip: u8, sentinel: u8 }
struct Vessel { next: Option<ptr<Vessel>>, tint: u8 }

fn set_digit(place: ref<write, u8>, digit: u8) -> u8 {
    let outgoing = replace(place, digit);
    outgoing
}

fn main() -> unit {
    let board = Gauge { sentinel: u8(201), tip: u8(64) };
    let displaced = loan_write(board@tip) { |out|
        set_digit(out, u8(23))
    };
    match try_allocate_one<Vessel>() {
        None => { unit },
        Some(first_bundle) => {
            let OneBacking { allocation, raw } = first_bundle;
            let alloc_front = allocation;
            let slot_front = into_slot<Vessel>(raw);
            let domain_front = lifetime_domain();
            let ptr_front = loan_read(domain_front) { |stable_front|
                initialize(slot_front, Vessel {
                    next: Option<ptr<Vessel>>::None, tint: u8(47)
                }, stable_front)
            };
            match try_allocate_one<Vessel>() {
                None => {
                    let ended_front = loan_exclusive_read(domain_front) { |ending_front|
                        destroy(ptr_front, ending_front)
                    };
                    let restored_front = erase_slot<Vessel>(ended_front);
                    finalize_domain(domain_front);
                    deallocate(alloc_front, restored_front);
                    unit
                },
                Some(second_bundle) => {
                    let OneBacking { allocation, raw } = second_bundle;
                    let alloc_back = allocation;
                    let slot_back = into_slot<Vessel>(raw);
                    let domain_back = lifetime_domain();
                    let ptr_back = loan_read(domain_back) { |stable_back|
                        initialize(slot_back, Vessel {
                            next: Option<ptr<Vessel>>::None, tint: u8(0)
                        }, stable_back)
                    };
                    let sampled = loan_read(domain_back) { |stable_back|
                        let actual_read = ref_from_ptr(read, ptr_back, stable_back);
                        read(actual_read@tint)
                    };
                    let ended_back = loan_exclusive_read(domain_back) { |ending_back|
                        destroy(ptr_back, ending_back)
                    };
                    let restored_back = erase_slot<Vessel>(ended_back);
                    finalize_domain(domain_back);
                    deallocate(alloc_front, restored_back);
                    let ended_front = loan_exclusive_read(domain_front) { |ending_front|
                        destroy(ptr_front, ending_front)
                    };
                    let restored_front = erase_slot<Vessel>(ended_front);
                    finalize_domain(domain_front);
                    deallocate(alloc_front, restored_front);
                    let prior_final = loan_write(board@tip) { |out|
                        set_digit(out, sampled)
                    };
                    let saved_final = prior_final;
                    unit
                },
            }
        },
    };
    let startup_old = displaced;
    let observed_tip = board@tip;
    let observed_sentinel = board@sentinel;
    unit
}
