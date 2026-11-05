struct Emblem { anchor: u8, moving: u8 }

fn main() -> unit {
    let root = Emblem{moving: u8(77), anchor: u8(221)};
    let entry_selected = root@moving;
    let entry_sibling = root@anchor;
    let phase = usize(2);
    let stopped = loop (phase = usize(2)) {
        if (phase < usize(9)) {
            let returned = choose(phase);
            let incoming = returned;
            let old = loan_write(root@moving) { |destination|
                replace(destination, incoming)
            };
            let old_copy = old;
            let current_copy = root@moving;
            let sibling_copy = root@anchor;
            continue(phase + usize(3));
        } else {
            break phase;
        }
    };
    let post_selected = root@moving;
    let post_sibling = root@anchor;
    let stopped_copy = stopped;
    let starting_phase_copy = phase;
    unit
}

fn choose(phase: usize) -> u8 {
    let chosen = if (phase == usize(5)) {
        u8(244)
    } else {
        u8(18)
    };
    let chosen_copy = chosen;
    chosen_copy
}
