struct Mirror { reserve: u8, visible: u8 }
struct Cinder { link: Option<ptr<Cinder>>, charge: u8 }

fn carry_old(place: ref<write,u8>, incoming: u8) -> u8 {
    let previous = replace(place, incoming);
    return previous;
}

fn separate_tail(
    head_link: ref<write,Option<ptr<Cinder>>>,
    p: ptr<Cinder>,
    a: Allocation,
    d: LifetimeDomain
) -> LiveTail {
    let previous_link = replace(head_link, Option<ptr<Cinder>>::None);
    return LiveTail {
        owned_ptr: p,
        owned_allocation: a,
        owned_domain: d
    };
}

fn finish_tail(
    tail_ptr: ptr<Cinder>,
    tail_allocation: Allocation,
    tail_life: LifetimeDomain
) -> unit {
    loan_read(tail_life) { |stable_tail|
        let checked_tail = ref_from_ptr(read, tail_ptr, stable_tail);
        unit
    };
    let empty_tail = loan_exclusive_read(tail_life) { |ending_tail|
        destroy(tail_ptr, ending_tail)
    };
    let full_tail = erase_slot<Cinder>(empty_tail);
    finalize_domain(tail_life);
    deallocate(tail_allocation, full_tail);
    unit
}

fn main() -> unit {
    match try_allocate_one<Cinder>() {
        None => { unit },
        Some(front_bundle) => {
            let OneBacking { allocation, raw } = front_bundle;
            let front_allocation = allocation;
            let front_slot = into_slot<Cinder>(raw);
            let front_domain = lifetime_domain();
            let front_ptr = loan_read(front_domain) { |stable_front|
                initialize(
                    front_slot,
                    Cinder { link: Option<ptr<Cinder>>::None, charge: u8(29) },
                    stable_front
                )
            };

            match try_allocate_one<Cinder>() {
                None => {
                    let empty_front = loan_exclusive_read(front_domain) { |ending_front|
                        destroy(front_ptr, ending_front)
                    };
                    let full_front = erase_slot<Cinder>(empty_front);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, full_front);
                    unit
                },
                Some(rear_bundle) => {
                    let OneBacking { allocation, raw } = rear_bundle;
                    let rear_allocation = allocation;
                    let rear_slot = into_slot<Cinder>(raw);
                    let rear_domain = lifetime_domain();
                    let rear_ptr = loan_read(rear_domain) { |stable_rear|
                        initialize(
                            rear_slot,
                            Cinder { link: Option<ptr<Cinder>>::None, charge: u8(173) },
                            stable_rear
                        )
                    };

                    let old_unlinked = loan_read(front_domain) { |stable_front|
                        let front_w = ref_from_ptr(write, front_ptr, stable_front);
                        replace(front_w@link, Option<ptr<Cinder>>::Some(rear_ptr))
                    };
                    let copied_link = loan_read(front_domain) { |stable_front|
                        let front_r = ref_from_ptr(read, front_ptr, stable_front);
                        read(front_r@link)
                    };
                    match copied_link {
                        None => { unit },
                        Some(selected_rear) => {
                            loan_read(rear_domain) { |stable_rear|
                                let rear_ref = ref_from_ptr(read, selected_rear, stable_rear);
                                let heat_value = read(rear_ref@charge);
                                let left = Mirror { visible: u8(97), reserve: u8(13) };
                                let right = Mirror { reserve: u8(6), visible: u8(198) };
                                let escaped_ref = loan_write(left@visible) { |temporary| temporary };
                                let old_visible = loan_write(left@visible) { |w|
                                    carry_old(w, heat_value)
                                };
                                let current_visible = left@visible;
                                let old_right_reserve = loan_write(right@reserve) { |w|
                                    replace(w, old_visible)
                                };
                                let old_left_reserve = loan_write(left@reserve) { |w|
                                    replace(w, current_visible)
                                };
                                let old_right_visible = loan_write(right@visible) { |w|
                                    replace(w, old_right_reserve)
                                };
                                let observed_left_reserve = left@reserve;
                                let observed_left_visible = left@visible;
                                let observed_right_reserve = right@reserve;
                                let observed_right_visible = right@visible;
                                unit
                            }
                        },
                    };

                    let LiveTail {
                        owned_ptr,
                        owned_allocation,
                        owned_domain
                    } = loan_read(front_domain) { |stable_front|
                        let front_w2 = ref_from_ptr(write, front_ptr, stable_front);
                        separate_tail(
                            front_w2@link, rear_ptr, rear_allocation, rear_domain)
                    };

                    loan_read(owned_domain) { |stable_tail|
                        let last_tail_read = ref_from_ptr(read, owned_ptr, stable_tail);
                        unit
                    };
                    finish_tail(owned_ptr, owned_allocation, owned_domain);

                    let empty_front = loan_exclusive_read(front_domain) { |ending_front|
                        destroy(front_ptr, ending_front)
                    };
                    let full_front = erase_slot<Cinder>(empty_front);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, full_front);
                    unit
                },
            }
        },
    }
}
