fn stripe_bytes(count: usize) -> usize {
    if (count < usize(1024)) {
        let advanced = count + usize(64);
        advanced
    } else {
        count
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
