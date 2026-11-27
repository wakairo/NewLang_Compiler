fn select_octet(input: usize) -> u8 {
    if (input < usize(256)) { let replacement = usize(256); u8(replacement) } else { u8(203) }
}

fn main() -> unit {
    let original = usize(255);
    let selected = select_octet(original);
    let observed = selected;
    return unit;
}
