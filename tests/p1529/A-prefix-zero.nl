// P1529 C1202 own FIRST: finite two-H/four-step terminal consumer composition.
struct Meter { count: u8, kind: u8 }
struct Cell { next: Option<ptr<Cell>>, payload: u8 }

fn set_count(dst: ref<write, u8>, next: u8) -> u8 {
    let old = replace(dst, next);
    old
}

fn close_cell(p: ptr<Cell>, a: Allocation, d: LifetimeDomain) -> unit {
    let empty = loan_exclusive_read(d) { |ending|
        destroy(p, ending)
    };
    let full = erase_slot<Cell>(empty);
    finalize_domain(d);
    deallocate(a, full);
    unit
}

fn main() -> unit {
    let meter = Meter { count: u8(83), kind: u8(27) };
    let before_start = loan_write(meter@count) { |w|
        set_count(w, u8(0))
    };
    let observed_current = meter@count;
    let observed_sibling = meter@kind;
    match try_allocate_one<Cell>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let a0 = allocation;
            let slot0 = into_slot<Cell>(raw);
            let d0 = lifetime_domain();
            let p0 = loan_read(d0) { |stable0|
                initialize(slot0,
                    Cell { next: Option<ptr<Cell>>::None,
                               payload: u8(83) }, stable0)
            };
            match try_allocate_one<Cell>() {
                None => {
                    let empty0 = loan_exclusive_read(d0) { |end0|
                        destroy(p0, end0)
                    };
                    let full0 = erase_slot<Cell>(empty0);
                    finalize_domain(d0);
                    deallocate(a0, full0);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let a1 = allocation;
                    let slot1 = into_slot<Cell>(raw);
                    let d1 = lifetime_domain();
                    let p1 = loan_read(d1) { |stable1|
                        initialize(slot1,
                            Cell { next: Option<ptr<Cell>>::None,
                                       payload: u8(0) }, stable1)
                    };
                    let sample = loan_read(d1) { |stable_read|
                        let r1 = ref_from_ptr(read, p1, stable_read);
                        read(r1@payload)
                    };
                    close_cell(p1, a1, d1);
                    let empty0 = loan_exclusive_read(d0) { |end0|
                        destroy(p0, end0)
                    };
                    let full0 = erase_slot<Cell>(empty0);
                    finalize_domain(d0);
                    deallocate(a0, full0);
                    let before_sample = loan_write(meter@count) { |w|
                        set_count(w, sample)
                    };
                    let retained_sample_old = before_sample;
                    unit
                },
            }
        },
    };
    let retained_start_old = before_start;
    let current = meter@count;
    let sibling = meter@kind;
    unit
}
