struct Gauge { level: u8, stamp: u8 }

fn choose(tick: usize) -> u8 {
    if (tick < usize(3)) {
        u8(109)
    } else {
        u8(231)
    }
}

fn main() -> unit {
    let root = Gauge{stamp: u8(208), level: u8(14)};
    let entry_selected = root@level;
    let entry_sibling = root@stamp;
    let starting_phase = usize(0);
    let stopped = loop (phase = usize(0)) {
        if (phase < usize(5)) {
            let returned = choose(phase);
            let incoming = returned;
            let old = loan_write(root@level) { |destination|
                replace(destination, incoming)
            };
            let old_copy = old;
            let current_copy = root@level;
            let sibling_copy = root@stamp;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let post_selected = root@level;
    let post_sibling = root@stamp;
    let stopped_copy = stopped;
    let starting_phase_copy = starting_phase;
    unit
}
