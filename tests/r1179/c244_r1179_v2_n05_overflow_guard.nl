struct Pebble { rim: u8, nub: u8 }

fn main() -> unit {
    let original = Pebble { rim: u8(14), nub: u8(87) };
    let finished = loop (phase = usize(1)) {
        if (phase < usize(2)) {
            let selected = if (phase < usize(1)) {
                u8(43)
            } else {
                u8(71)
            };
            let copied = selected;
            let old = loan_write(original@rim) { |writer|
                replace(writer, copied)
            };
            continue(phase + usize(18446744073709551615));
        } else {
            break phase;
        }
    };
    unit
}
