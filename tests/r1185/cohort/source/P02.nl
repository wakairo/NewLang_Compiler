struct Ledger { reserve: u8, signal: u8 }

fn main() -> unit {
    let root = Ledger{signal: u8(42), reserve: u8(155)};
    let entry_selected = root@signal;
    let entry_sibling = root@reserve;
    let starting_phase = usize(1);
    let stopped = loop (phase = usize(1)) {
        if (phase < usize(3)) {
            let returned = choose(phase);
            let incoming = returned;
            let old = loan_write(root@signal) { |destination|
                replace(destination, incoming)
            };
            let old_copy = old;
            let current_copy = root@signal;
            let sibling_copy = root@reserve;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let post_selected = root@signal;
    let post_sibling = root@reserve;
    let stopped_copy = stopped;
    let starting_phase_copy = starting_phase;
    unit
}

fn choose(tick: usize) -> u8 {
    let chosen = if (tick == usize(1)) {
        u8(67)
    } else {
        u8(183)
    };
    let chosen_copy = chosen;
    chosen_copy
}
