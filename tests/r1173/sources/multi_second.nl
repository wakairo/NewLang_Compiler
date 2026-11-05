struct Latch { pulse: u8, tag: u8, }
fn main() -> unit {
    let record = Latch { pulse: u8(157), tag: u8(43) };
    let result = loop (i = usize(2)) {
        if (i < usize(10)) {
            let before = record@tag;
            let old = loan_write(record@tag) { |writer|
                replace(writer, u8(7))
            };
            let after = record@tag;
            let sibling = record@pulse;
            continue(i + usize(2));
        } else {
            break i;
        }
    };
    let final_selected = record@tag;
    let final_sibling = record@pulse;
    unit
}
