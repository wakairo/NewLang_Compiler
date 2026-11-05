struct Mica { facet: u8, vein: u8 }

fn main() -> unit {
    let original = Mica { facet: u8(61), vein: u8(131) };
    let finished = loop (phase = usize(0)) {
        if (phase < usize(3)) {
            let prewrite = original@facet;
            let sibling_before = original@vein;
            let selected = if (phase < usize(1)) {
                u8(61)
            } else {
                u8(97)
            };
            let copied = selected;
            let old = loan_write(original@facet) { |writer|
                replace(writer, copied)
            };
            let old_copy = old;
            let current = original@facet;
            let sibling = original@vein;
            continue(phase + usize(1));
        } else {
            break phase;
        }
    };
    let current_final = original@facet;
    let sibling_final = original@vein;
    unit
}
