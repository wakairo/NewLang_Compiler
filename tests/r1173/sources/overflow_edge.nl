struct Latch { pulse: u8, tag: u8, }
fn main() -> unit {
    let record = Latch { pulse: u8(29), tag: u8(151) };
    let result = loop (i = usize(18446744073709551614)) {
        if (i < usize(18446744073709551615)) {
            let before = record@pulse;
            let old = loan_write(record@pulse) { |writer|
                replace(writer, u8(197))
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
