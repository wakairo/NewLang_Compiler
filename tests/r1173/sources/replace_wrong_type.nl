struct Latch { pulse: u8, tag: u8, }
fn main() -> unit {
    let record = Latch { pulse: u8(29), tag: u8(151) };
    let result = loop (i = usize(1)) {
        if (i < usize(8)) {
            let before = record@pulse;
            let old = loan_write(record@pulse) { |writer|
                replace(writer, usize(197))
            };
            let after = record@pulse;
            let sibling = record@tag;
            continue(i + usize(3));
        } else {
            break i;
        }
    };
    let final_selected = record@pulse;
    let final_sibling = record@tag;
    unit
}
