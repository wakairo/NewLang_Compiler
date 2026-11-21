fn main() -> unit {
    let result = overflow(usize(137) / usize(6));
    unit
}

fn overflow(units: usize) -> usize {
    units % usize(13)
}
