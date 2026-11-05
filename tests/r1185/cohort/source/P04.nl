struct Marker { unchanged: u8, active: u8 }

fn main() -> unit {
    let root = Marker{unchanged: u8(120), active: u8(33)};
    let entry_selected = root@active;
    let entry_sibling = root@unchanged;
    let starting_phase = usize(1);
    let stopped = loop (phase = usize(1)) {
        if (phase < usize(8)) {
            let returned = choose(phase);
            let incoming = returned;
            let old = loan_write(root@active) { |destination|
                replace(destination, incoming)
            };
            let old_copy = old;
            let current_copy = root@active;
            let sibling_copy = root@unchanged;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let post_selected = root@active;
    let post_sibling = root@unchanged;
    let stopped_copy = stopped;
    let starting_phase_copy = starting_phase;
    unit
}

fn choose(tick: usize) -> u8 {
    let chosen = if (tick == usize(5)) {
        u8(76)
    } else {
        u8(187)
    };
    let chosen_copy = chosen;
    chosen_copy
}
