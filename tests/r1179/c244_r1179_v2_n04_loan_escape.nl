struct Pebble { rim: u8, nub: u8 }

fn main() -> unit {
    let original = Pebble { rim: u8(14), nub: u8(87) };
    let finished = loop (phase = usize(0)) {
        if (phase < usize(2)) {
            let selected = if (phase < usize(1)) {
                u8(43)
            } else {
                u8(71)
            };
            let copied = selected;
            let escaped = loan_write(original@rim) { |writer|
                writer
            };
            let old = replace(escaped, copied);
            continue(phase + usize(1));
        } else {
            break phase;
        }
    };
    unit
}
