struct Pair { left: u8, right: u8, }
fn main() -> unit {
    let p = Pair { left: u8(9), right: u8(10), };
    let escaped = loan_write(p@left) { |w| w };
    escaped;
    unit
}
