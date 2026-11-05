struct Latch { pulse: u8, tag: u8, }
fn main() -> unit {
    let record = Latch { pulse: u8(101), tag: u8(222) };
    let result = loop (i = usize(0)) {
        if (i < usize(3)) {
            let before = record@pulse;
            let old = loan_write(record@pulse) { |writer|
                replace(writer, u8(101))
            };
            let after = record@pulse;
            let sibling = record@tag;
            continue(i + usize(1));
        } else {
            break i;
        }
    };
    let final_selected = record@pulse;
    let final_sibling = record@tag;
    unit
}
