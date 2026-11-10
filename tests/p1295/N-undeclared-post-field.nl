// P1295 C439: a completed H result changes an original that predates H.
struct RouteCell { link: Option<ptr<RouteCell>>, reading: u8 }
struct MeterState { keep: u8, mark: u8 }

fn install_reading(destination: ref<write,u8>, sampled: u8) -> u8 {
    let removed = replace(destination, sampled);
    removed
}
fn extract_cell(edge: ref<write,Option<ptr<RouteCell>>>, tail: ptr<RouteCell>,
                ticket: Allocation, life: LifetimeDomain) -> LiveTail {
    let former = replace(edge, Option<ptr<RouteCell>>::None);
    return LiveTail { owned_ptr: tail, owned_allocation: ticket,
                      owned_domain: life };
}
fn dispose_cell(cell: ptr<RouteCell>, ticket: Allocation,
                life: LifetimeDomain) -> unit {
    loan_read(life) { |held|
        let existing = ref_from_ptr(read, cell, held);
        unit
    };
    let empty = loan_exclusive_read(life) { |ending| destroy(cell, ending) };
    let bytes = erase_slot<RouteCell>(empty);
    finalize_domain(life);
    deallocate(ticket, bytes);
    unit
}
fn main() -> unit {
    let retained = MeterState { mark: u8(101), keep: u8(213) };
    let computed = match try_allocate_one<RouteCell>() {
        None => { u8(13) },
        Some(front_packet) => {
            let OneBacking { allocation, raw } = front_packet;
            let front_ticket = allocation;
            let front_slot = into_slot<RouteCell>(raw);
            let front_life = lifetime_domain();
            let front = loan_read(front_life) { |making|
                initialize(front_slot,
                    RouteCell { link: Option<ptr<RouteCell>>::None,
                                reading: u8(233) }, making)
            };
            match try_allocate_one<RouteCell>() {
                None => {
                    let empty = loan_exclusive_read(front_life) { |ending|
                        destroy(front, ending)
                    };
                    let bytes = erase_slot<RouteCell>(empty);
                    finalize_domain(front_life);
                    deallocate(front_ticket, bytes);
                    u8(29)
                },
                Some(tail_packet) => {
                    let OneBacking { allocation, raw } = tail_packet;
                    let tail_ticket = allocation;
                    let tail_slot = into_slot<RouteCell>(raw);
                    let tail_life = lifetime_domain();
                    let tail = loan_read(tail_life) { |making|
                        initialize(tail_slot,
                            RouteCell { link: Option<ptr<RouteCell>>::None,
                                        reading: u8(163) }, making)
                    };
                    let former = loan_read(front_life) { |access|
                        let edge = ref_from_ptr(write, front, access);
                        replace(edge@link, Option<ptr<RouteCell>>::Some(tail))
                    };
                    let present = loan_read(front_life) { |access|
                        let root = ref_from_ptr(read, front, access);
                        read(root@link)
                    };
                    let payload_result = match present {
                        None => { u8(47) },
                        Some(actual_tail) => {
                            let sampled = loan_read(tail_life) { |access|
                                let root = ref_from_ptr(read, actual_tail, access);
                                read(root@reading)
                            };
                            let working = MeterState { keep: u8(179), mark: u8(71) };
                            let prior = loan_write(working@mark) { |destination|
                                install_reading(destination, sampled)
                            };
                            let copy = prior;
                            copy
                        }
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(front_life) { |access|
                        let edge = ref_from_ptr(write, front, access);
                        extract_cell(edge@link, tail, tail_ticket, tail_life)
                    };
                    dispose_cell(owned_ptr, owned_allocation, owned_domain);
                    let empty = loan_exclusive_read(front_life) { |ending|
                        destroy(front, ending)
                    };
                    let bytes = erase_slot<RouteCell>(empty);
                    finalize_domain(front_life);
                    deallocate(front_ticket, bytes);
                    payload_result
                }
            }
        }
    };
    let supplied = computed;
    let previous_mark = loan_write(retained@absent) { |destination|
        replace(destination, supplied)
    };
    let actual_current = retained@mark;
    let observed_sibling = retained@keep;
    let saved_old = previous_mark;
    unit
}
