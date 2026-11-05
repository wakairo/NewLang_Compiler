struct Mosaic { frame: u8, ink: u8, }

fn tint(ticket: usize) -> u8 {
    let copied_ticket = ticket;
    if (copied_ticket < usize(5)) {
        u8(94)
    } else {
        u8(132)
    }
}

fn main() -> unit {
    let vessel = Mosaic { ink: u8(57), frame: u8(199) };
    let entry = vessel@ink;
    let entry_copy = entry;
    let break_value = loop (phase = usize(1)) {
        if (phase < usize(7)) {
            let before_selected = vessel@ink;
            let ticket = phase;
            let from_call = tint(ticket);
            let replacement = from_call;
            let old = loan_write(vessel@ink) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@ink;
            let sibling = vessel@frame;
            let entry_still = entry_copy;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@ink;
    let post_sibling = vessel@frame;
    let entry_after = entry_copy;
    unit
}
