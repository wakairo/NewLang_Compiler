fn main() -> unit {
    let first = overflow(batch(usize(173)));
    let second = overflow(batch(usize(173)));
    unit
}

fn overflow(units: usize) -> usize {
    units % usize(13)
}

fn batch(units: usize) -> usize {
    units / usize(6)
}
