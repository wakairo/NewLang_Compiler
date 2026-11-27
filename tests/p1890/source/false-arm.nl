fn select_octet(input: usize) -> u8 {
    if (input < usize(256)) { u8(203) } else { u8(input) }
}

fn main() -> unit {
    let original = usize(256);
    let selected = select_octet(original);
    let observed = selected;
    return unit;
}
