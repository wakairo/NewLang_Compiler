fn main() -> unit {
    let result = overflow(fault(usize(81)));
    unit
}

fn overflow(units: usize) -> usize {
    units % usize(13)
}

fn fault(units: usize) -> usize {
    units % usize(0)
}
