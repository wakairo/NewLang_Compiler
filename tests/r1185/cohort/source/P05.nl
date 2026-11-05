struct Badge { face: u8, seal: u8 }

fn choose(tick: usize) -> u8 {
    if (tick < usize(2)) {
        u8(13)
    } else {
        u8(198)
    }
}

fn main() -> unit {
    let root = Badge{face: u8(198), seal: u8(49)};
    let entry_selected = root@face;
    let entry_sibling = root@seal;
    let starting_phase = usize(3);
    let stopped = loop (phase = usize(3)) {
        if (phase < usize(4)) {
            let returned = choose(phase);
            let incoming = returned;
            let old = loan_write(root@face) { |destination|
                replace(destination, incoming)
            };
            let old_copy = old;
            let current_copy = root@face;
            let sibling_copy = root@seal;
            continue(phase + usize(1));
        } else {
            break phase;
        }
    };
    let post_selected = root@face;
    let post_sibling = root@seal;
    let stopped_copy = stopped;
    let starting_phase_copy = starting_phase;
    unit
}
