struct HeapSample {
    next: Option<ptr<HeapSample>>,
    datum: u8,
}

struct AuditPair {
    metric: u8,
    guard: u8,
}

fn install_metric(place: ref<write,u8>, incoming: u8) -> u8 {
    let previous = replace(place, incoming);
    return previous;
}

fn main() -> unit {
    let target = AuditPair { metric: u8(84), guard: u8(211) };
    let first_previous = loan_write(target@metric) { |writer|
        install_metric(writer, u8(31))
    };

    match try_allocate_one<HeapSample>() {
        None => {
            return unit;
        },
        Some(front_bundle) => {
            let OneBacking { allocation, raw } = front_bundle;
            let allocation_front = allocation;
            let vacant_front = into_slot<HeapSample>(raw);
            let domain_front = lifetime_domain();
            let ptr_front = loan_read(domain_front) { |stable_front|
                initialize(
                    vacant_front,
                    HeapSample { next: Option<ptr<HeapSample>>::None, datum: u8(107) },
                    stable_front
                )
            };

            match try_allocate_one<HeapSample>() {
                None => {
                    let empty_front = loan_exclusive_read(domain_front) { |end_front|
                        destroy(ptr_front, end_front)
                    };
                    let raw_front = erase_slot<HeapSample>(empty_front);
                    finalize_domain(domain_front);
                    deallocate(allocation_front, raw_front);
                    return unit;
                },
                Some(back_bundle) => {
                    let OneBacking { allocation, raw } = back_bundle;
                    let allocation_back = allocation;
                    let vacant_back = into_slot<HeapSample>(raw);
                    let domain_back = lifetime_domain();
                    let ptr_back = loan_read(domain_back) { |stable_back|
                        initialize(
                            vacant_back,
                            HeapSample { next: Option<ptr<HeapSample>>::None, datum: u8(173) },
                            stable_back
                        )
                    };

                    let previous_link = loan_read(domain_front) { |stable_front|
                        let front_writer = ref_from_ptr(write, ptr_front, stable_front);
                        replace(front_writer@next, Option<ptr<HeapSample>>::Some(ptr_back))
                    };
                    let selected_link = loan_read(domain_front) { |stable_front|
                        let front_reader = ref_from_ptr(read, ptr_front, stable_front);
                        read(front_reader@next)
                    };
                    let from_heap = match selected_link {
                        None => {
                            let empty_back = loan_exclusive_read(domain_back) { |end_back|
                                destroy(ptr_back, end_back)
                            };
                            let raw_back = erase_slot<HeapSample>(empty_back);
                            finalize_domain(domain_back);
                            deallocate(allocation_back, raw_back);
                            let empty_front = loan_exclusive_read(domain_front) { |end_front|
                                destroy(ptr_front, end_front)
                            };
                            let raw_front = erase_slot<HeapSample>(empty_front);
                            finalize_domain(domain_front);
                            deallocate(allocation_front, raw_front);
                            return unit;
                        },
                        Some(chosen_ptr) => {
                            loan_read(domain_front) { |stable_front|
                                loan_read(domain_back) { |stable_back|
                                    let chosen_reader = ref_from_ptr(read, chosen_ptr, stable_back);
                                    read(chosen_reader@datum)
                                }
                            }
                        },
                    };
                    let detached_link = loan_read(domain_front) { |stable_front|
                        let front_writer_again = ref_from_ptr(write, ptr_front, stable_front);
                        replace(front_writer_again@next, Option<ptr<HeapSample>>::None)
                    };

                    let empty_back = loan_exclusive_read(domain_back) { |end_back|
                        destroy(ptr_back, end_back)
                    };
                    let raw_back = erase_slot<HeapSample>(empty_back);
                    finalize_domain(domain_back);
                    deallocate(allocation_back, raw_back);

                    let empty_front = loan_exclusive_read(domain_front) { |end_front|
                        destroy(ptr_front, end_front)
                    };
                    let raw_front = erase_slot<HeapSample>(empty_front);
                    finalize_domain(domain_front);
                    deallocate(allocation_front, raw_front);

                    let second_previous = loan_write(target@metric) { |writer|
                        install_metric(writer, from_heap)
                    };
                    let observed_metric = target@metric;
                    let observed_guard = target@guard;
                    unit
                },
            }
        },
    };
    unit
}
