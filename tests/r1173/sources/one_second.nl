struct Latch { pulse: u8, tag: u8, }
fn main() -> unit {
    let record = Latch { pulse: u8(64), tag: u8(18) };
    let result = loop (i = usize(3)) {
        if (i < usize(4)) {
            let before = record@tag;
            let old = loan_write(record@tag) { |writer|
                replace(writer, u8(203))
            };
            let after = record@tag;
            let sibling = record@pulse;
            continue(i + usize(1));
        } else {
            break i;
        }
    };
    let final_selected = record@tag;
    let final_sibling = record@pulse;
    unit
}
