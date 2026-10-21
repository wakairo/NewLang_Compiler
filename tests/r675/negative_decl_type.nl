struct Clasp { one: u8, two: usize }
fn main() -> unit {
    let value = Clasp { one: u8(9), two: usize(4) };
    let first = value@one;
    first;
    unit
}
