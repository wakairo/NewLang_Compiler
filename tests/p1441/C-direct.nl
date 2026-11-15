// P1441 source-law direct-read control; original P1435 bytes remain separate.
struct GaugeCell { next: Option<ptr<GaugeCell>>, payload: u8 }
struct Observation { level: u8, spare: u8 }

fn install_sample(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    displaced
}
fn unlink_cell(edge: ref<write,Option<ptr<GaugeCell>>>, cell: ptr<GaugeCell>,
               allocation: Allocation, domain: LifetimeDomain) -> LiveTail {
    let former = replace(edge, Option<ptr<GaugeCell>>::None);
    return LiveTail { owned_ptr: cell, owned_allocation: allocation,
                      owned_domain: domain };
}
fn release_cell(cell: ptr<GaugeCell>, allocation: Allocation,
                domain: LifetimeDomain) -> unit {
    loan_read(domain) { |held|
        let live = ref_from_ptr(read, cell, held);
        unit
    };
    let empty = loan_exclusive_read(domain) { |ending| destroy(cell, ending) };
    let bytes = erase_slot<GaugeCell>(empty);
    finalize_domain(domain);
    deallocate(allocation, bytes);
    unit
}
fn main() -> unit {
    let completed = match try_allocate_one<GaugeCell>() {
        None => { u8(7) },
        Some(front_bundle) => {
            let OneBacking { allocation, raw } = front_bundle;
            let front_allocation = allocation;
            let front_slot = into_slot<GaugeCell>(raw);
            let front_domain = lifetime_domain();
            let front = loan_read(front_domain) { |initializing|
                initialize(front_slot,
                    GaugeCell { next: Option<ptr<GaugeCell>>::None, payload: u8(41) },
                    initializing)
            };
            match try_allocate_one<GaugeCell>() {
                None => {
                    let empty = loan_exclusive_read(front_domain) { |ending|
                        destroy(front, ending)
                    };
                    let bytes = erase_slot<GaugeCell>(empty);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, bytes);
                    u8(11)
                },
                Some(back_bundle) => {
                    let OneBacking { allocation, raw } = back_bundle;
                    let back_allocation = allocation;
                    let back_slot = into_slot<GaugeCell>(raw);
                    let back_domain = lifetime_domain();
                    let back = loan_read(back_domain) { |initializing|
                        initialize(back_slot,
                            GaugeCell { next: Option<ptr<GaugeCell>>::None, payload: u8(197) },
                            initializing)
                    };
                    let previous = loan_read(front_domain) { |attaching|
                        let edge = ref_from_ptr(write, front, attaching);
                        replace(edge@next, Option<ptr<GaugeCell>>::Some(back))
                    };
                    let state = loan_read(front_domain) { |observing|
                        let reader = ref_from_ptr(read, front, observing);
                        read(reader@next)
                    };
                    let sampled = match state {
                        None => { u8(13) },
                        Some(actual_back) => {
                            let payload = loan_read(back_domain) { |reading|
                                let reader = ref_from_ptr(read, actual_back, reading);
                                read(reader@payload)
                            };
                            let observation = Observation { level: u8(31), spare: u8(251) };
                            let old = loan_write(observation@level) { |writing|
                                install_sample(writing, payload)
                            };
                            let old_copy = old;
                            let current = observation@level;
                            let sibling = observation@spare;
                            old_copy
                        }
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(front_domain) { |detaching|
                            let edge = ref_from_ptr(write, front, detaching);
                            unlink_cell(edge@next, back, back_allocation, back_domain)
                        };
                    release_cell(owned_ptr, owned_allocation, owned_domain);
                    let empty = loan_exclusive_read(front_domain) { |ending|
                        destroy(front, ending)
                    };
                    let bytes = erase_slot<GaugeCell>(empty);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, bytes);
                    sampled
                }
            }
        }
    };
    let retained = completed;
    retained;
    unit
}
