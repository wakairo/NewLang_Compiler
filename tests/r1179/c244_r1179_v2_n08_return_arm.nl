struct Pebble { rim: u8, nub: u8 }

fn main() -> unit {
    let original = Pebble { rim: u8(14), nub: u8(87) };
    let finished = loop (phase = usize(0)) {
        if (phase < usize(2)) {
            let selected = if (phase < usize(1)) {
                let return_marker = u8(173);
                return unit;
            } else {
                u8(71)
            };
            let copied = selected;
            let old = loan_write(original@rim) { |writer|
                replace(writer, copied)
            };
            continue(phase + usize(1));
        } else {
            break phase;
        }
    };
    unit
}
