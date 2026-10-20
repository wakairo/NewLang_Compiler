struct Lattice { ridge: u8, ember: u8 }
fn main() -> unit {
    let tile = Lattice { ridge: u8(5), ember: u8(8) };
    let old = loan_write(tile@ridge) { |w| replace(w, usize(3)) };
    old;
    unit
}
