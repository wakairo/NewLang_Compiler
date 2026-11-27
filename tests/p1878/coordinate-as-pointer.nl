fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r| ptr_from_ref(r) };
    let original = addr(p);
    let coordinate = uintptr(original);
    loan_read_ptr(coordinate) { |s| unit };
    unit
}
