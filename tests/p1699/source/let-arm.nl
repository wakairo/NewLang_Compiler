fn stripe_bytes(count: usize) -> usize {
    if (count < usize(1024)) {
        let size = count * usize(64);
        size
    } else {
        usize(0)
    }
}

fn main() -> unit {
    let key = usize(1);
    let input = if (key < usize(2)) { usize(1023) } else { usize(1024) };
    let bytes = stripe_bytes(input);
    let observed = bytes;
    observed;
    unit
}
