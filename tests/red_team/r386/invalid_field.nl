struct Pair { left: u8, right: u8, }
fn main() -> unit {
    let p = Pair { left: u8(7), right: u8(8), };
    loan_write(p@missing) { |w|
        replace(w, u8(10))
    };
    unit
}
