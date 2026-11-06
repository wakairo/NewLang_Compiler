struct Cairn { peg: u8, flare: u8, }

fn spectrum(tick: usize) -> u8 {
    if (tick < usize(7)) {
        u8(61)
    } else {
        u8(89)
    }
}

fn main() -> unit {
    let vessel = Cairn { flare: u8(25), peg: u8(163) };
    let entry = vessel@flare;
    let entry_copy = entry;
    let break_value = loop (phase = usize(4)) {
        if (phase < usize(13)) {
            let before_selected = vessel@flare;
            let bridge_one = phase;
            let bridge_two = bridge_one;
            let from_call = spectrum(bridge_two);
            let replacement = from_call;
            let old = loan_write(vessel@flare) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@flare;
            let sibling = vessel@peg;
            let entry_still = entry_copy;
            continue(phase + usize(3));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@flare;
    let post_sibling = vessel@peg;
    let entry_after = entry_copy;
    unit
}
