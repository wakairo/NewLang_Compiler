fn main() -> unit {
    let original = u8(7);
    let p = loan_read(original) { |r| ptr_from_ref(r) };
    let a = addr(p);
    let point = uintptr(a);
    let bad = u8(point);
    unit
}
