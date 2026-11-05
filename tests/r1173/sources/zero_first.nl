struct Latch { pulse: u8, tag: u8, }
fn main() -> unit {
    let record = Latch { pulse: u8(37), tag: u8(149) };
    let result = loop (i = usize(9)) {
        if (i < usize(5)) {
            let before = record@pulse;
            let old = loan_write(record@pulse) { |writer|
                replace(writer, u8(211))
            };
            let after = record@pulse;
            let sibling = record@tag;
            continue(i + usize(2));
        } else {
            break i;
        }
    };
    let final_selected = record@pulse;
    let final_sibling = record@tag;
    unit
}
