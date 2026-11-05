struct DoubleCell { hot: u8, cold: u8 }

fn sample(phase: usize) -> u8 {
    if (phase < usize(2)) {
        u8(58)
    } else {
        u8(219)
    }
}

fn main() -> unit {
    let root = DoubleCell{hot: u8(37), cold: u8(166)};
    let entry_selected = root@hot;
    let entry_sibling = root@cold;
    let starting_phase = usize(0);
    let stopped = loop (phase = usize(0)) {
        if (phase < usize(3)) {
            let first = sample(phase);
            let shifted = phase + usize(1);
            let returned = sample(shifted);
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
