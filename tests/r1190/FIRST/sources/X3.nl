struct Vela { rim: u8, glow: u8, }

fn shine(notch: usize) -> u8 {
    let formal_one = notch;
    let formal_two = formal_one;
    let formal_three = formal_two;
    if (formal_three < usize(4)) {
        u8(98)
    } else {
        u8(136)
    }
}

fn main() -> unit {
    let vessel = Vela { glow: u8(45), rim: u8(187) };
    let entry = vessel@glow;
    let entry_copy = entry;
    let break_value = loop (phase = usize(1)) {
        if (phase < usize(7)) {
            let before_selected = vessel@glow;
            let from_call = shine(phase);
            let replacement = from_call;
            let old = loan_write(vessel@glow) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@glow;
            let sibling = vessel@rim;
            let entry_still = entry_copy;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@glow;
    let post_sibling = vessel@rim;
    let entry_after = entry_copy;
    unit
}
