fn main() -> unit {
    let original = u8(7);
    let p = loan_read(original) { |r| ptr_from_ref(r) };
    let bad = usize(p);
    unit
}
