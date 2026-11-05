struct Beacon { crest: u8, guard: u8 }

fn choose(tick: usize) -> u8 {
    if (tick < usize(4)) {
        u8(31)
    } else {
        u8(202)
    }
}

fn main() -> unit {
    let root = Beacon{crest: u8(91), guard: u8(174)};
    let entry_selected = root@crest;
    let entry_sibling = root@guard;
    let starting_phase = usize(5);
    let stopped = loop (phase = usize(5)) {
        if (phase < usize(5)) {
            let returned = choose(phase);
            let incoming = returned;
            let old = loan_write(root@crest) { |destination|
                replace(destination, incoming)
            };
            let old_copy = old;
            let current_copy = root@crest;
            let sibling_copy = root@guard;
            continue(phase + usize(3));
        } else {
            break phase;
        }
    };
    let post_selected = root@crest;
    let post_sibling = root@guard;
    let stopped_copy = stopped;
    let starting_phase_copy = starting_phase;
    unit
}
