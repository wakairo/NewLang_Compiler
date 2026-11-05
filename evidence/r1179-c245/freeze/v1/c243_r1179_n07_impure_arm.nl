struct Pebble { rim: u8, nub: u8 }

fn main() -> u8 {
    let original = Pebble { rim: u8(14), nub: u8(87) };
    let finished = loop (phase = usize(0), memory = u8(199)) {
        if (phase < usize(2)) {
            let selected = if (phase < usize(1)) {
                loan_write(original@nub) { |side_writer|
                    replace(side_writer, u8(67));
                    u8(43)
                }
            } else {
                u8(71)
            };
            let copied = selected;
            let old = loan_write(original@rim) { |writer|
                replace(writer, copied)
            };
            continue(phase + usize(1), old);
        } else {
            break memory;
        }
    };
    finished
}
