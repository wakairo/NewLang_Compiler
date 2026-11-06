struct Keel { port: u8, star: u8, }

fn level(notch: usize) -> u8 {
    if (notch < usize(5)) {
        u8(62)
    } else {
        u8(91)
    }
}

fn main() -> unit {
    let vessel = Keel { star: u8(47), port: u8(167) };
    let entry = vessel@star;
    let entry_copy = entry;
    let kernel = usize(3);
    let seed_one = kernel;
    let seed_two = seed_one;
    let seed_three = seed_two;
    let break_value = loop (phase = seed_three) {
        if (phase < usize(7)) {
            let before_selected = vessel@star;
            let from_call = level(phase);
            let replacement = from_call;
            let old = loan_write(vessel@star) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@star;
            let sibling = vessel@port;
            let entry_still = entry_copy;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@star;
    let post_sibling = vessel@port;
    let entry_after = entry_copy;
    unit
}
