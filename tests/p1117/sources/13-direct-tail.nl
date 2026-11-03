fn tail_selected(quantity: usize) -> u8 {
    if (quantity == usize(1025)) { u8(136) } else { u8(217) }
}
fn main() -> unit {
    let truncated = tail_selected(usize(1));
    let exact = tail_selected(usize(1025));
    let other = tail_selected(usize(1026));
    return unit;
}
