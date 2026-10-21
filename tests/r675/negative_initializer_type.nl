struct Kestrel { upper: u8, lower: u8 }
fn main() -> unit {
    let bird = Kestrel { upper: usize(11), lower: u8(17) };
    let first = bird@upper;
    first;
    unit
}
