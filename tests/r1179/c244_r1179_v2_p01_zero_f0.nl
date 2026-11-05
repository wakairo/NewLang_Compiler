struct Basin { ember: u8, quartz: u8 }

fn main() -> unit {
    let original = Basin { ember: u8(17), quartz: u8(91) };
    let finished = loop (phase = usize(5)) {
        if (phase < usize(5)) {
            let prewrite = original@ember;
            let sibling_before = original@quartz;
            let selected = if (phase < usize(6)) {
                u8(31)
            } else {
                u8(73)
            };
            let copied = selected;
            let old = loan_write(original@ember) { |writer|
                replace(writer, copied)
            };
            let old_copy = old;
            let current = original@ember;
            let sibling = original@quartz;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let current_final = original@ember;
    let sibling_final = original@quartz;
    unit
}
