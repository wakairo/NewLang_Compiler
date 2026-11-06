struct Slate { glyph: u8, bead: u8, }

fn main() -> unit {
    let vessel = Slate { bead: u8(181), glyph: u8(59) };
    let entry = vessel@glyph;
    let entry_copy = entry;
    let kernel = usize(42);
    let seed_echo = kernel;
    let break_value = loop (phase = seed_echo) {
        if (phase < usize(42)) {
            let before_selected = vessel@glyph;
            let from_call = shade(phase);
            let replacement = from_call;
            let old = loan_write(vessel@glyph) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@glyph;
            let sibling = vessel@bead;
            let entry_still = entry_copy;
            continue(phase + usize(1));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@glyph;
    let post_sibling = vessel@bead;
    let entry_after = entry_copy;
    unit
}

fn shade(ticket: usize) -> u8 {
    if (ticket == usize(42)) {
        u8(67)
    } else {
        u8(104)
    }
}
