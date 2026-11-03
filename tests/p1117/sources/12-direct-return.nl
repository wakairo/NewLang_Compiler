fn return_selected(quantity: usize) -> u8 {
    return if (quantity < usize(260)) { u8(154) } else { u8(219) };
}
fn main() -> unit {
    let below = return_selected(usize(259));
    let equal = return_selected(usize(260));
    let above = return_selected(usize(261));
    return unit;
}
