fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r| ptr_from_ref(r) };
    let original = addr(p);
    let coordinate = uintptr(original);
    let residue = coordinate % usize(1);
    let zero = usize(0);
    let matches = residue == zero;
    unit
}
