// P1528 C1198 FIRST: optional same-D read, closed before terminal teardown.
struct Reading { value: u8, route: u8 }
struct Resource { next: Option<ptr<Resource>>, payload: u8 }

fn write_reading(dst: ref<write, u8>, next: u8) -> u8 {
    let old = replace(dst, next);
    old
}

fn retire(p: ptr<Resource>, a: Allocation, d: LifetimeDomain) -> unit {
    loan_read(d) { |inspect|
        let scoped = ref_from_ptr(read, p, inspect);
        unit
    };
    let empty = loan_exclusive_read(d) { |ending|
        destroy(p, ending)
    };
    let full = erase_slot<Resource>(empty);
    finalize_domain(d);
    deallocate(a, full);
    unit
}

fn main() -> unit {
    let reading = Reading { value: u8(91), route: u8(23) };
    let before_start = loan_write(reading@value) { |w|
        write_reading(w, u8(0))
    };
    match try_allocate_one<Resource>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let a0 = allocation;
            let slot0 = into_slot<Resource>(raw);
            let d0 = lifetime_domain();
            let p0 = loan_read(d0) { |stable0|
                initialize(slot0,
                    Resource { next: Option<ptr<Resource>>::None,
                               payload: u8(67) }, stable0)
            };
            match try_allocate_one<Resource>() {
                None => {
                    let empty0 = loan_exclusive_read(d0) { |end0|
                        destroy(p0, end0)
                    };
                    let full0 = erase_slot<Resource>(empty0);
                    finalize_domain(d0);
                    deallocate(a0, full0);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let a1 = allocation;
                    let slot1 = into_slot<Resource>(raw);
                    let d1 = lifetime_domain();
                    let p1 = loan_read(d1) { |stable1|
                        initialize(slot1,
                            Resource { next: Option<ptr<Resource>>::None,
                                       payload: u8(0) }, stable1)
                    };
                    let sample = loan_read(d1) { |stable_read|
                        let r1 = ref_from_ptr(read, p1, stable_read);
                        read(r1@payload)
                    };
                    retire(p0, a1, d1);
                    let empty0 = loan_exclusive_read(d0) { |end0|
                        destroy(p0, end0)
                    };
                    let full0 = erase_slot<Resource>(empty0);
                    finalize_domain(d0);
                    deallocate(a0, full0);
                    let before_sample = loan_write(reading@value) { |w|
                        write_reading(w, sample)
                    };
                    let retained_sample_old = before_sample;
                    unit
                },
            }
        },
    };
    let retained_start_old = before_start;
    let current = reading@value;
    let sibling = reading@route;
    unit
}
