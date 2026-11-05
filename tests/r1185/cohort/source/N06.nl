struct NestedCell { hot: u8, cold: u8 }

fn sample(phase: usize) -> u8 {
    if (phase < usize(2)) {
        if (phase == usize(0)) {
            u8(41)
        } else {
            u8(93)
        }
    } else {
        u8(157)
    }
}

fn main() -> unit {
    let root = NestedCell{hot: u8(25), cold: u8(234)};
    let entry_selected = root@hot;
    let entry_sibling = root@cold;
    let starting_phase = usize(0);
    let stopped = loop (phase = usize(0)) {
        if (phase < usize(3)) {
            let returned = sample(phase);
            let incoming = returned;
            let old = loan_write(root@hot) { |destination|
                replace(destination, incoming)
            };
            let old_copy = old;
            let current_copy = root@hot;
            let sibling_copy = root@cold;
            continue(phase + usize(1));
        } else {
            break phase;
        }
    };
    let post_selected = root@hot;
    let post_sibling = root@cold;
    let stopped_copy = stopped;
    let starting_phase_copy = starting_phase;
    unit
}
