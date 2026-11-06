struct Orbit { ray: u8, arc: u8, }

fn chime(notch: usize) -> u8 {
    if (notch == usize(4)) {
        u8(79)
    } else {
        usize(122)
    }
}

fn main() -> unit {
    let vessel = Orbit { arc: u8(183), ray: u8(49) };
    let entry = vessel@ray;
    let entry_copy = entry;
    let break_value = loop (phase = usize(4)) {
        if (phase < usize(8)) {
            let before_selected = vessel@ray;
            let from_call = chime(phase);
            let replacement = from_call;
            let old = loan_write(vessel@ray) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@ray;
            let sibling = vessel@arc;
            let entry_still = entry_copy;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@ray;
    let post_sibling = vessel@arc;
    let entry_after = entry_copy;
    unit
}
