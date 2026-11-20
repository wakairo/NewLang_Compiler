fn circular_offset(position: usize) -> usize {
    position % usize(18446744073709551615)
}

fn main() -> unit {
    let offset = circular_offset(usize(18446744073709551614));
    let prior = offset - usize(1);
    unit
}
