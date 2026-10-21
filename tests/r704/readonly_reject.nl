struct QuietRegister { stamp:u8, spare:u8 }
fn main()->unit {
    let quiet = QuietRegister { spare:u8(14), stamp:u8(29) };
    loan_read(quiet@stamp) { |reader| replace(reader,u8(99)) };
    unit
}
