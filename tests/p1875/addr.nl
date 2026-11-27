fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r| ptr_from_ref(r) };
    let y = u8(9);
    let q = loan_read(y) { |s| ptr_from_ref(s) };
    let p_copy = p;
    let x_addr = addr(p);
    let copy_addr = addr(p_copy);
    let y_addr = addr(q);
    let saved_addr = x_addr;
    let same_eq = saved_addr == copy_addr;
    let same_ne = saved_addr != copy_addr;
    let distinct_eq = x_addr == y_addr;
    let distinct_ne = x_addr != y_addr;
    unit
}
