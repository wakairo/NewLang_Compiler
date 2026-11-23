fn stripe_bytes(count: usize) -> usize {
    if (count < usize(2)) {
        count * usize(18446744073709551615)
    } else {
        usize(0)
    }
}

fn main() -> unit {
    let key = usize(1);
    let input = if (key < usize(2)) { usize(1) } else { usize(2) };
    let bytes = stripe_bytes(input);
    let observed = bytes;
    observed;
    unit
}
