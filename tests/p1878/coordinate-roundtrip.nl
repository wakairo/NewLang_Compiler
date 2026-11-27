fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r| ptr_from_ref(r) };
    let original = addr(p);
    let coordinate = uintptr(original);
    let returned = addr(coordinate);
    let matches = returned == original;
    unit
}
