fn stripe_bytes(count: usize) -> usize {
    if (count < usize(1024)) {
        count * usize(64)
    } else {
        usize(0)
    }
}

fn main() -> unit {
    let key = usize(1);
    let input = if (key < usize(2)) { usize(1024) } else { usize(1023) };
    let bytes = stripe_bytes(input);
    let observed = bytes;
    observed;
    unit
}
