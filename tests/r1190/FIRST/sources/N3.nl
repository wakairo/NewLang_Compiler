struct Tidal { wake: u8, crest: u8, }

fn sieve(notch: usize) -> u8 {
    if (notch == usize(4)) {
        u8(121)
    } else {
        u8(84)
    }
}

fn main() -> unit {
    let vessel = Tidal { crest: u8(53), wake: u8(191) };
    let entry = vessel@crest;
    let entry_copy = entry;
    let break_value = loop (phase = usize(4)) {
        if (phase < usize(7)) {
            let before_selected = vessel@crest;
            let from_call = sieve(phase);
            let replacement = from_call;
            let old = loan_write(vessel@crest) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@crest;
            let sibling = vessel@wake;
            let entry_still = entry_copy;
            continue(u8(6));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@crest;
    let post_sibling = vessel@wake;
    let entry_after = entry_copy;
    unit
}
