struct Readout { level: u8, lane: u8 }
struct Frame { link: Option<ptr<Frame>>, code: u8 }

fn install(slot: ref<write, u8>, value: u8) -> u8 {
    let old = replace(slot, value);
    old
}

fn main() -> unit {
    let view = Readout { lane: u8(91), level: u8(37) };
    let old_start = loan_write(view@level) { |slot|
        install(slot, u8(7))
    };
    match try_allocate_one<Frame>() {
        None => { unit },
        Some(b0) => {
            let OneBacking { allocation, raw } = b0;
            let a0 = allocation;
            let s0 = into_slot<Frame>(raw);
            let d0 = lifetime_domain();
            let p0 = loan_read(d0) { |stable0|
                initialize(s0, Frame {
                    link: Option<ptr<Frame>>::None, code: u8(51)
                }, stable0)
            };
            match try_allocate_one<Frame>() {
                None => {
                    let e0 = loan_exclusive_read(d0) { |ending0|
                        destroy(p0, ending0)
                    };
                    let r0 = erase_slot<Frame>(e0);
                    finalize_domain(d0);
                    deallocate(a0, r0);
                    unit
                },
                Some(b1) => {
                    let OneBacking { allocation, raw } = b1;
                    let a1 = allocation;
                    let s1 = into_slot<Frame>(raw);
                    let d1 = lifetime_domain();
                    let p1 = loan_read(d1) { |stable1|
                        initialize(s1, Frame {
                            link: Option<ptr<Frame>>::None, code: u8(0)
                        }, stable1)
                    };
                    let sample = loan_read(d1) { |stable1|
                        let r = ref_from_ptr(read, p1, stable1);
                        read(r@code)
                    };
                    let e1 = loan_exclusive_read(d1) { |ending1|
                        destroy(p1, ending1)
                    };
                    let r1 = erase_slot<Frame>(e1);
                    finalize_domain(d1);
                    deallocate(a1, r1);
                    let e0 = loan_exclusive_read(d0) { |ending0|
                        destroy(p0, ending0)
                    };
                    let r0 = erase_slot<Frame>(e0);
                    finalize_domain(d0);
                    deallocate(a0, r0);
                    let old_success = loan_write(view@level) { |slot|
                        install(slot, sample)
                    };
                    let saved_success = old_success;
                    unit
                },
            }
        },
    };
    let saved_start = old_start;
    let observed = view@level;
    let sibling = view@lane;
    unit
}
