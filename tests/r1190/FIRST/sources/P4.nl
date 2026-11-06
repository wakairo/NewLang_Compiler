struct Ribbon { tone: u8, seal: u8, }

fn main() -> unit {
    let vessel = Ribbon { seal: u8(179), tone: u8(39) };
    let entry = vessel@tone;
    let entry_copy = entry;
    let break_value = loop (phase = usize(6)) {
        if (phase < usize(13)) {
            let before_selected = vessel@tone;
            let from_call = signal(phase);
            let replacement = from_call;
            let old = loan_write(vessel@tone) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@tone;
            let sibling = vessel@seal;
            let entry_still = entry_copy;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@tone;
    let post_sibling = vessel@seal;
    let entry_after = entry_copy;
    unit
}

fn signal(mark: usize) -> u8 {
    let formal_one = mark;
    let formal_two = formal_one;
    if (formal_two == usize(10)) {
        u8(117)
    } else {
        u8(82)
    }
}
