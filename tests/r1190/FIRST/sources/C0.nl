struct Datum { anchor: u8, ripple: u8, }

fn dial(step: usize) -> u8 {
    if (step < usize(4)) {
        u8(21)
    } else {
        u8(46)
    }
}

fn main() -> unit {
    let vessel = Datum { ripple: u8(33), anchor: u8(157) };
    let entry = vessel@ripple;
    let entry_copy = entry;
    let break_value = loop (phase = usize(2)) {
        if (phase < usize(8)) {
            let before_selected = vessel@ripple;
            let from_call = dial(phase);
            let replacement = from_call;
            let old = loan_write(vessel@ripple) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@ripple;
            let sibling = vessel@anchor;
            let entry_still = entry_copy;
            continue(phase + usize(3));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@ripple;
    let post_sibling = vessel@anchor;
    let entry_after = entry_copy;
    unit
}
