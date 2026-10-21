struct Canopy { apex: u8, base: u8 }
fn main() -> unit {
    let specimen = Canopy { apex: u8(4), base: u8(12) };
    let err = specimen@missing;
    unit
}
