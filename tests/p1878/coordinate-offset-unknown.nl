fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r| ptr_from_ref(r) };
    let original = addr(p);
    let coordinate = uintptr(original);
    let displacement = usize(4);
    let bad = coordinate + displacement;
    unit
}
